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

	if (_r->IsActived()) {
		_r->Dequeue(q, v);
	}

	return v;
}

