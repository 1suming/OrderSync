#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <iterator>
#include <map>
#include <netdb.h>
#include "watchdog.h"
#include "defs.h"

#include "clib_log.h"
extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;
static inline long long GetTimeNowInMsec(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec/1000;
}



Watchdog::Watchdog()
{
	_host = "127.0.0.1";
	_port = 7900;
	_fd = -1;
}

Watchdog::Watchdog(string host, int port)
{
	_host = host;
	_port = port;
	_fd = -1;
}

Watchdog::~Watchdog()
{
	/* if (_fd < -1) { */
	if (_fd > -1) {
		close(_fd);
		_fd = -1;
	}
}

int Watchdog::init(string host, int port)
{
	_host = host;
	_port = port;
	init();
}

int Watchdog::init()
{
	unsigned long inaddr = 0;
	struct hostent *host;
	
	_fd = socket(AF_INET ,SOCK_DGRAM, 0);
	if(_fd == -1) {
		cout << "Failed to socket because of " << strerror(errno) << endl;
		return -1;
	}
	
	memset(&_server_addr,0, sizeof(_server_addr));
	_server_addr.sin_family = AF_INET;
	_server_addr.sin_port = htons(_port);
	inaddr = inet_addr(_host.c_str());
	if (inaddr == INADDR_NONE) {
		// 线程非安全
		host = gethostbyname(_host.c_str());
		if (host == NULL) {
			cout << "Failed to gethostbyname because of " << strerror(errno) << endl;
			close(_fd);
			return -1;
		}
		memcpy((char*)&_server_addr.sin_addr, host->h_addr, host->h_length);
	} 
	else
		_server_addr.sin_addr.s_addr = inaddr;
	
	_server_addr_len = sizeof(_server_addr);
	
	return 0;
}

int Watchdog::write(const char* buf, int length)
{
	int ret;
	// cout << data << endl;
	ret = sendto(_fd, buf, length, 0, (struct sockaddr *)&_server_addr, _server_addr_len);
	if(ret < 0) {
		//cout << "Failed to sendto because of " << strerror(errno) << endl;
		g_pErrorLog->logMsg("Failed to sendto because of %s",  strerror(errno));
		return -1;
	}
	return 0;
}

int Watchdog::writeUserLeaveHall(int uid, string ip, int sid)
{
	char msg[1000] = {0};
	int logtimer = GetTimeNowInMsec()/1000;
	snprintf(msg, sizeof(msg), "%d#%d#%d#%s:%d\r\n",UDP_SERVER_CLIENT_CLOSE_LOG, logtimer, uid, ip.c_str(), sid);	

	string info ;
	info.append(msg);
	write(info.c_str(), info.length());
	return 0;
}

int Watchdog::writeHallUserCount(int count, const string& ip, int port)
{
	char msg[1000] = {0};
	int logtimer = GetTimeNowInMsec()/1000;
	snprintf(msg, sizeof(msg), "%d#%d#%s:%d#%d\r\n",UDP_SERVER_USER_COUNT, logtimer, ip.c_str(), port, count);	

	string info ;
	info.append(msg);
	write(info.c_str(), info.length());
	return 0;
}

int Watchdog::writeCommonLog(string info)
{
	write(info.c_str(), info.length());
	return 0;
}

