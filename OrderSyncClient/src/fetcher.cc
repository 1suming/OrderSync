#include "fetcher.h"
#include "TcpClient.h"
#include "RedisHelper.h"

string 
fetcher_t::fetch(const char* q)
{
	string v;

	if (_r->IsAlived()) {
		_r->Dequeue(q, v);
	}

	return v;
}

