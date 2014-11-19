#include "fetcher.h"
#include "TcpClient.h"
#include "RedisHelper.h"

fetcher_t::
~fetcher_t()
{
	delete _r;
}

string 
fetcher_t::fetch(const char* q)
{
	string v;

	FF:
	if (_r->IsActived()) {
		_r->Dequeue(q, v);
	} else { //redis 不正常
		sleep(1);
		_r->Connect();
		goto FF;
	}

	return v;
}

