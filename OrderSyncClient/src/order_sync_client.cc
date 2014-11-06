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

using namespace Json;

extern conf_t g_conf;

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

	rbuff = (char*)malloc(1024);

	_c->readable(true);
	_c->writeable(true);
	_p->attach(_c);

	log_debug("event_id: %"PRIu64, event_id);

	while (1) {
		ev_nums = _p->wait(100);

		if (ev_nums <= 0) continue;

		events = _p->get_events();

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
					if (type == 1) { /* update order 需要获取创建时间，同步端需要依赖这个字段 */
						value["mtime"] = (UInt64)get_ord_date(id); 
						json = writer.write(value); 
					} 

					out.begin(0x0002);
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
								log_error("order: %s", json.c_str());
								break;
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
				}
			} // if (_f)
		}
	} // while

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

	r = _m->ExecuteQuery(sql);

	if (r) {
		if (r->HasNext()) {
			t = r->GetLong(0);
		} else {
			t = 0;
		}
	} else {
		log_error("mysql execute failed.");
		return -1;
	}

	return t;
}


