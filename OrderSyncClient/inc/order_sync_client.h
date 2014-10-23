#ifndef __ORDER_SYNC_CLIENT_H_
#define __ORDER_SYNC_CLIENT_H_

#include <stdint.h>

class fetcher_t;
class CTcpClient;
class CMysqlHelper;

typedef class CTcpClient 	tcp_client_t;
typedef class CMysqlHelper 	mysql_helper_t;

class order_sync_client_t {
public:
	order_sync_client_t(fetcher_t* f, tcp_client_t* c, mysql_helper_t* m)
		:_f(f),
		_c(c),
		_m(m)
	{
	}
	~order_sync_client_t();
public:
	int run()
public:
	long get_ord_date(uint64_t id); /* 返回订单创建时间 */
private:
	fetcher_t 		*_f;
	tcp_client_t 	*_c;
	mysql_helper_t	*_m;
};

#endif 
