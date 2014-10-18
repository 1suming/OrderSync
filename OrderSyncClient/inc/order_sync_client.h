#ifndef __ORDER_SYNC_CLIENT_H_
#define __ORDER_SYNC_CLIENT_H_

class fetcher_t;
class tcp_client_t;


class order_sync_client_t {
public:
	order_sync_client(fetcher_t* f, tcp_client_t* c)
		:_f(f),
		_c(c)
	{
	}
	~order_sync_client() { delete _f; delete _c; }
public:
	int run();
private:
	fetcher_t 		*_f;
	tcp_client_t 	*_c;
};

#endif __ORDER_SYNC_CLIENT_H_
