#include "OrderSyncClient.h"
#include "packet.h"

int
order_sync_client_t::run()
{
	packet_t 	out;
	string 		json;
	int			r;

	while (1) {
		if (_f) {
			json = _f->fetch("");

			if (!json.empty()) {
				out.begin(0x0001);
				out.write_string(json);
				out.end();

				if (_c) {
				send:
					r = _c->Send(out.get_data(), out.get_len());

					if (r == 0) {
						if (_c->Reconnect() == 0) {
							goto send;
						}
					}
				}
			}

			out.clean;
		}
	}

	return 0;
}




