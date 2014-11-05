#ifndef __TCP_CLIENT_H_
#define __TCP_CLIENT_H_

#include <string>

using std::string;

#define READABLE 	0x001
#define WRITEABLE 	0x004
#define ET 			1 << 31

class CTcpClient {
public:
    CTcpClient(string host, unsigned short port);
    ~CTcpClient();

    /*
	        发送完size大小的缓冲内容才返回
	        返回-1异常
	        返回0 对方关闭连接
	 */
    int Send(const char* buffer, int size);

	 /*
	        接收完size大小的缓冲内容才返回
	        返回-1异常
	        返回0 对方关闭连接
	 */
    int Recv(char* buffer, int size);

    /*
	        连接远端地址
	        返回 -1异常 
	        返回 0 连接成功 
	 */
    int Connect();

    int Disconneced() { IsConnected = false; return 0; }

	int SetSendTimeout(unsigned int sec);

	int SetRecvTimeout(unsigned int sec);

	int SetNonblock();

	int Reconnect();

	int Close();

	int check();

	uint32_t get_event() { return event; }
	int	get_fd() { return socketfd_; }

	void readable(bool s) { s ? event |= READABLE : event &= ~READABLE; }
	void writeable(bool s) { s ? event |= WRITEABLE : event &= ~WRITEABLE; }
private:
    int				socketfd_;
    string			host_;
    unsigned short 	port_;
	bool 			IsConnected;
	uint32_t		event;
private:
	int Socket();
};

#endif
