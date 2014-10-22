#ifndef __ORDER_SYNC_CLIENT_H_
#define __ORDER_SYNC_CLIENT_H_

class fetcher_t;
class CTcpClient;

typedef class CTcpClient tcp_client_t;

class order_sync_client_t {
public:
	order_sync_client_t(fetcher_t* f, tcp_client_t* c)
		:_f(f),
		_c(c)
	{
	}
	~order_sync_client_t();
public:
	int run();
private:
	fetcher_t 		*_f;
	tcp_client_t 	*_c;
};

#endif 
