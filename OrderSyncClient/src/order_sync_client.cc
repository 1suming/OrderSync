#include "order_sync_client.h"
#include "packet.h"
#include "log.h"
#include "fetcher.h"
#include "TcpClient.h"
#include "MysqlHelper.h"
#include "json/json.h"

using namespace Json;

order_sync_client_t::
~order_sync_client_t()
{
	delete _f;
	delete _c;
	delete _m;
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

	while (1) {
		if (_f) {
			json = _f->fetch("");

			if (!json.empty()) {
				if (!reader.parse(json, value)) {
					log_error("json: %s parse failed.", json.c_str());
					continue;
				}

				type = value["type"].asInt();
				id = value["id"].asUInt64();
				if (type == 1) { /* update order 需要获取创建时间，同步端需要依赖这个字段 */
					value["time"] = (UInt64)get_ord_date(id); 
				} 

				out.begin(0x0001);
				out.write_string(json);
				out.end();

				if (_c) {
				send:
					r = _c->Send(out.get_data(), out.get_len());

					if (r == 0) {
						if (_c->Reconnect() > 0) {
							goto send;
						} else {
							log_error("connected server failed");
							log_error("order: %s", json.c_str());
							break;
						}
					}
				}
			}

			out.clean();
		}
	}

	return 0;
}

long
order_sync_client_t::get_ord_date(uint64_t id)
{
	char 			*sql;
	CMysqlResult 	*r;
	long			t;

	sql = "select pstarttime from paycenter_order where pid = %llu";

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



