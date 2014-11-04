#include "TcpClient.h"
#include "log.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

static inline int
_set_nonblock(int fd)
{
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}


CTcpClient::CTcpClient(string host, unsigned short port)
	:host_(host),
	 port_(port),
	 IsConnected(false)
{
    Socket();
}

CTcpClient::~CTcpClient()
{
	Close();
}

int
CTcpClient::Send(const char* buffer, int size)
{
	int iLen; //发送的总字节数
    int cLen;//当次发送的字节数

	iLen = cLen = 0;

	if (IsConnected) {
		while(iLen < size) {
			cLen = send(socketfd_, buffer + iLen, size - iLen, 0); 

			if (cLen == -1) {
				if (errno == EAGAIN || errno == EINTR) {
					continue;
				} else {
					Disconneced();
					Close(); 
					return 0;
				}
			} else if (cLen == 0) { // 连接关闭
				Disconneced();
				Close(); 
				return 0;
			} 

			iLen += cLen;
		}
	}

	return iLen;
}

int
CTcpClient::Recv(char* buffer, int size)
{
	int rlen;

	if(!IsConnected) return 0;
	
    rlen = recv(socketfd_, buffer, size, 0);

    return rlen;
}

//private

int
CTcpClient::Socket()
{
	log_debug("--------CTcpClient::Socket begin --------");
	socketfd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd_ == -1){
		log_debug("--------CTcpClient::Socket end -1 --------");
		return -1;
	} else {
		log_debug("--------CTcpClient::Socket end 0 --------");
		return 0;
	}
}

int
CTcpClient::Connect()
{
	log_debug("--------CTcpClient::Connect begin --------");
	int result = 0;

	if (IsConnected) return 0;

	struct sockaddr_in remote;

    remote.sin_addr.s_addr = inet_addr(host_.c_str());
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port_);

	if(this->socketfd_ > 0) {
		result = connect(socketfd_, (struct sockaddr*)&remote, sizeof(struct sockaddr));

		IsConnected = result == 0 ? true : false;

		if (IsConnected) SetNonblock();
	}

	log_debug("--------CTcpClient::Connect end --------");
    return result;
}

int
CTcpClient::Reconnect()
{
	int ret;

	log_debug("--------CTcpClient::Reconnect begin --------");
	ret = Socket();

	if (ret == -1) {
		log_error("Reconnect invoke socket() failed.");
		return -1;
	}

	log_debug("--------CTcpClient::Reconnect end --------");
	return Connect();
}


int
CTcpClient::Close()
{
    int result = ::close(socketfd_);

    return result;
}

int
CTcpClient::SetSendTimeout(unsigned int sec)
{
	struct timeval timeout = {sec, 0};

	return setsockopt(socketfd_, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
}

int
CTcpClient::SetRecvTimeout(unsigned int sec)
{
	struct timeval timeout = {sec, 0};

	return setsockopt(socketfd_, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
}

int
CTcpClient::SetNonblock()
{
	return _set_nonblock(socketfd_);
}

int
CTcpClient::check()
{
	int 	rlen;
	char 	rbuff[4];

	rlen = Recv(rbuff, 4);

	if (rlen == -1) {
		if (errno == EAGAIN || errno == EINTR || errno == EINPROGRESS) {
			return 1;
		} else {
			return 0;
		}
	} else if (rlen == 0) {
		return 0;
	}

	return rlen;
}
