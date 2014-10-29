#ifndef __REDIS_HELPER_H_
#define __REDIS_HELPER_H_

#include <string>
#include "hiredis.h"

using namespace std;

class redis_helper_t {
public:
	redis_helper_t(const string& host, 
				 unsigned short port, 
				 unsigned short second = 5)
		:reply(0), 
		host_(host), 
		port_(port)
	{
		timeout.tv_sec = second;
		timeout.tv_usec = 0;
		
		context = redisConnectWithTimeout(host_.c_str(), port_, timeout);
	}

	~redis_helper_t() 
	{
		if(reply) freeReply();
		if(context) redisFree(context); 
	}
	
public:

	int Connect(const char* host, 
				const unsigned short& port, 
				const unsigned short second = 5);
	int Connect();

	int Set(const string& key, const string& value);
	string& Get(const string& key, string& value);

	int Enqueue(const string& queue, const string& value);
	string& Dequeue(const string& queue, string& value);

	int Push(const string& stack, const string& value);
	string& Pop(const string& stack, string& value);

	bool IsAlived() { return ping(); }
	
private:
	redisContext	*context;
	redisReply		*reply;
	struct timeval 	timeout;

	string 			host_;
	unsigned short 	port_;

	void freeReply() 
	{ 
		if(reply) {
			freeReplyObject(reply);
			reply = NULL;
		}
	}

	redis_helper_t(const redis_helper_t& rhs) {}
	redis_helper_t& operator=(const redis_helper_t& rsh) { return *this; }

	bool ping();
};

#endif

