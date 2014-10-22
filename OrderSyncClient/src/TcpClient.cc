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
					return -1;
				}
			} else if (cLen == 0) { // 连接关闭
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
    if(!IsConnected)
	 {
        return -1;
	 }

	 int iLen = 0;

    iLen = recv(socketfd_, buffer, size, MSG_WAITALL);
//	 int iLen = 0;
//  int cLen = 0;
//
//  if(0 == size)
//  {
//      cLen = recv(socketfd_, buffer, 260, 0);
//      iLen = cLen;
//  }
//  else
//  {
//      while (iLen < size)
//      {
// 	 cLen = recv(socketfd_, buffer + iLen, size - iLen, 0);
//
// 	 if (-1 == cLen)
// 	 {
// 	     return -1;
// 	 }
//
// 	 iLen += cLen;
//      }
//  }

    return iLen;
}

//private

int
CTcpClient::Socket()
{
    socketfd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd_ == -1){
        return -1;
	} else {
        return 0;
	}
}

int
CTcpClient::Connect()
{
    int result = 0;

	if (IsConnected) return 0;

	struct sockaddr_in remote;

    remote.sin_addr.s_addr = inet_addr(host_.c_str());
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port_);

	if(this->socketfd_ > 0) {
		result = connect(socketfd_, (struct sockaddr*)&remote, sizeof(struct sockaddr));

		IsConnected = result == 0 ? true : false;
	}

    return result;
}

int
CTcpClient::Reconnect()
{
	int ret;

	ret = Socket();

	if (ret == -1) {
		log_error("Reconnect invoke socket() failed.");
		return -1;
	}

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
