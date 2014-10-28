#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

/*
	Any bugs reports to rocenting(#)gmail.com
 */

#include <iostream>			// std::cout
#include <string.h> 		// std::string
#include <map> 				// std::map
#include <netinet/in.h> 	// struct sockaddr_in
#include "ICHAT_PacketBase.h"
#include <sys/time.h>

using namespace std;

/*
	datacenter namespace
 */

class Watchdog {
public:
	Watchdog();
	Watchdog(string host, int port);
	~Watchdog();
	int init();
	int init(string host, int port);
	int write(const char* buf, int length);

	//命令字的处理时间
	int writeUserLeaveHall(int uid, string ip, int sid);

	int writeHallUserCount(int count, const string &ip, int port);

	int writeCommonLog(string info);
	
private:
	string				_host;
	int					_port;
	int					_fd;
	struct sockaddr_in	_server_addr;
	int					_server_addr_len;
};

#endif
