#include "order_sync_client.h"
#include "packet.h"
#include "log.h"
#include "fetcher.h"
#include "TcpClient.h"
#include "MysqlHelper.h"
#include "json/json.h"
#include "conf.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace Json;

extern conf_t g_conf;

order_sync_client_t::
~order_sync_client_t()
{
	delete _f;delete _c;delete _m;
}

int
order_sync_client_t::run()
{
	packet_t 		out;
	string 			json;
	int				r;
	Reader			reader;
	Value 			value;
	int				type;
	unsigned long 	id;
	FastWriter		writer;

	while (1) {
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
				out.write_string(json);
				out.end();

				if (_c) {
					if (_c->check() == 0) {
						_c->Disconneced();
						_c->Close();
						log_error("Tcp connection closed.");
					}
				send:
					r = _c->Send(out.get_data(), out.get_len());

					log_debug("SEND: %d", r);

					if (r == 0) {
						_c->Disconneced();
						_c->Close();
						if (_c->Reconnect() == 0) {
							goto send;
						} else {
							log_error("connected server failed");
							log_error("order: %s", json.c_str());
							break;
						}
					}
				}
			} else {
				usleep(100); //如果没有记录就休眠
			}

			out.clean(); 
		}
	}

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



