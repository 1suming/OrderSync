#include "order_sync_client.h"
#include "packet.h"
#include "log.h"
#include "fetcher.h"
#include "TcpClient.h"
#include "MysqlHelper.h"
#include "poller.h"
#include "json/json.h"
#include "conf.h"
#define __STDC_FORMAT_MACROS // PRIu64
#include <inttypes.h>
#include <sys/epoll.h>
#include <errno.h>
//��ӵ�*.o�ļ��еı�ʾ��Ϣ ��Ϣ����Ϊ�ַ�������
#ident "\\n$@           ID : chenbo.o v1.3 (25/08/07)@$\\n" 


using namespace Json;

extern conf_t g_conf;

#define __FREE_TIME		1000 * 1000 // 1s ������ʱ����ʱ��
#define __EXEC_TIME 	10 * 1000 // 10ms  ������һ����¼��˯�ߵ�ʱ��
#define __CMD_DATA_SYNC 0x0002
#define __ERROR_TIMES 	200
#define __ERROR_TIME 	25 * 1000

#ifndef ALLOC
#define ALLOC(P, SZ) P = (__typeof__(P))malloc(SZ); if (P) memset(P, 0, SZ)
#endif

order_sync_client_t::
~order_sync_client_t()
{
	delete _f;delete _c;delete _m;delete _p;
}

int
order_sync_client_t::run()
{
	packet_t 			out, in;
	string 				json;
	int					r, rr;
	Reader				reader;
	Value 				value;
	int					type;
	unsigned long 		id;
	FastWriter			writer;
	struct epoll_event 	*events;
	int					ev_nums;
	char				*rbuff;
	long 				mtime;
	int 				r_errs;
	string 				table;

	ALLOC(rbuff, 1024);

	_c->readable(true);
	_c->writeable(true);
	_p->attach(_c);

	log_debug("event_id: %"PRIu64, event_id);

	while (1) {
		ev_nums = _p->wait(100);

		if (ev_nums <= 0) continue;

		events = _p->get_events();

		r_errs = 0;

		if (events[0].events & READABLE) {
			if (_c->check() == 0) {
				_c->Disconneced();
				_c->Close();
				_p->detach(_c);
				log_error("Tcp connection closed.");

				sleep(1);

				if (_c->Reconnect() == 0) {
					_c->readable(true);
					_c->writeable(true);
					_p->attach(_c);
				}
			}
		}

		if (events[0].events & WRITEABLE) {
			if (_f) {
				json = _f->fetch(g_conf.redis_key);

				if (!json.empty()) {
					if (!reader.parse(json, value)) {
						log_error("json: %s parse failed.", json.c_str());
						continue;
					}

					log_debug("JSON: %s", json.c_str());

					type = value["type"].asInt();
					id = value["id"].asUInt64();
					table = value["table"].asString();
					if (type == 1) { /* update order ��Ҫ��ȡ����ʱ�䣬ͬ������Ҫ��������ֶ� */
#ifdef __PARTITION__
#warning("__PARTITION__ defined")
						mtime = get_ord_date(table, id);
#else
#warning("__PARTITION__ not defined")
						mtime = get_ord_date(id);
#endif
						if (mtime <= 0) { // ��ȡ������ʱ�������
							log_error("ORDER: %s", json.c_str());
							continue;
						}
						value["mtime"] = (Int64)mtime; 
						json = writer.write(value); 
					} 

					out.begin(__CMD_DATA_SYNC);
					out.write_int(g_conf.client_id);
					out.write_uint64(event_id);
					out.write_string(json);
					out.end();

					if (_c) {
					send:
						r = _c->Send(out.get_data(), out.get_len());

						log_debug("SEND: %d", r);

						if (r == 0) {
							_c->Disconneced();
							_c->Close();
							if (_c->Reconnect() == 0) {
								_c->readable(true);
								_c->writeable(true);
								_p->attach(_c);

								goto send;
							} else {
								log_error("connected server failed");
								sleep(5);/*  ����ʧ�ܺ�ȴ�5s*/
								goto send;
							}
						}

						//response
						RR:
						rr = _c->Recv(rbuff, 1024);
						if (rr == 0) {
							_c->Disconneced();
							_c->Close();
							goto send;
						} else if (rr == -1) {
							if (errno == EAGAIN || errno == EINTR || errno == EINPROGRESS) {
								if (r_errs <= __ERROR_TIMES) {
									usleep(__ERROR_TIME); 
									r_errs++; /* ��ֹ���ղ�������������ѭ��*/
								} else {
									r_errs = 0;
									_c->Disconneced();
									_c->Close();
									goto send;
								}

								goto RR;
							} else {
								_c->Disconneced();
								_c->Close();
								goto send;
							}
						} else if (rr > 0) {
							log_debug("rr: %d", rr);
							int ret = in.parse_packet(rbuff, rr);
							log_debug("decode: %d", ret);
							if (ret == 0) {
								uint64_t eid = in.read_uint64();
								log_debug("eid: %"PRIu64, eid);
								if (event_id == eid) {
									log_debug("sync success.");
								} else {
									log_error("sync faild: %s", json.c_str());
								}
							} else {
								log_error("sync packet parse failed.");
							}
						}
					}

					out.clean();
					event_id++;
					usleep(__EXEC_TIME);
				} else {
					usleep(__FREE_TIME);
				}

			} // if (_f)
		}
	} // while

	free(rbuff);
	return 0;
}

long
order_sync_client_t::get_ord_date(uint64_t id)
{
	string 			sql;
	CMysqlResult 	*r;
	long			t;
	size_t			pos;
	char			temp[64];

	sql = "select pstarttime from paycenter_order where pid = :id";
	pos = sql.find(":id");
	if (pos != string::npos) {
		snprintf(temp, 64, "%"PRIu64, id);
		sql.replace(pos, strlen(":id"), temp);
		log_debug("sql: %s", sql.c_str());
	}

	if (!_m->IsConnected()) {
		_m->Connect();
		_m->UseDB(g_conf.mysql_db);
	}
	r = _m->ExecuteQuery(sql);

	if (r) {
		if (r->HasNext()) {
			t = r->GetLong(0);
		} else {
			t = 0;
		}

		delete r;
	} else {
		log_error("mysql execute failed: %lu", id);
		return -1;
	}

	return t;
}

long
order_sync_client_t::get_ord_date(string& table, uint64_t id)
{
	string 			sql;
	size_t			pos;
	char 			temp[64];
	CMysqlResult 	*r;
	long 			t;

	sql = "select pstarttime from " + table + " where pid = :id";
	pos = sql.find(":id");
	if (pos != string::npos) {
		snprintf(temp, 64, "%"PRIu64, id);
		sql.replace(pos, strlen(":id"), temp);
		log_debug("sql: %s", sql.c_str());
	}

	if (!_m->IsConnected()) {
		_m->Connect();
		_m->UseDB(g_conf.mysql_db);
	}
	r = _m->ExecuteQuery(sql);

	if (r) {
		if (r->HasNext()) {
			t = r->GetLong(0);
		} else {
			t = 0;
		}

		delete r;
	} else {
		log_error("mysql execute failed: %lu", id);
		return -1;
	}

	return t;
}

