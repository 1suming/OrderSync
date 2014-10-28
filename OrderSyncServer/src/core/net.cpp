#include <log.h>
#include <net.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>

int CNet::init_unix_addr(struct sockaddr_un *addr, const char *path)
{
/*
    bzero (addr, sizeof (struct sockaddr_un));
    addr->sun_family = AF_LOCAL;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path)-1);
    socklen_t addrlen = SUN_LEN(addr);
    addr->sun_path[0] = '\0';
*/

    bzero (addr, sizeof (struct sockaddr_un));
    addr->sun_family = AF_LOCAL;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path)-1);
    socklen_t addrlen = sizeof(struct sockaddr_un);

    return addrlen;
}

int CNet::unix_connect(int* netfd, const char* path, int block, int* out_errno)
{
    *netfd = SOCKET_INVALID;
    struct sockaddr_un unaddr;
    socklen_t addrlen;

    if ((*netfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        if (NULL != out_errno)
        {
            *out_errno = errno;
        }

        return SOCKET_CREATE_FAILED;
    }

    if (!block)
    {
        fcntl (*netfd, F_SETFL, O_RDWR|O_NONBLOCK);
    }

    addrlen = init_unix_addr(&unaddr, path);

    int ret = ::connect (*netfd, (struct sockaddr *)&unaddr, addrlen);

    if (NULL != out_errno)
    {
        *out_errno = errno;
    }

    return ret;

}

int CNet::init_udp_unix(const char* szPath, int bFlag)
{
    int sock_fd;    
    int sock_opt = 0;    
    struct sockaddr_un sa;    
    
    if ((sock_fd = ::socket(PF_UNIX, SOCK_DGRAM, 0)) == -1) 
        return -1;

    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));    

    if(bFlag)
        fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    bzero (&sa, sizeof (struct sockaddr_un));
    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path, szPath, sizeof(sa.sun_path)-1);
    socklen_t addrlen = SUN_LEN(&sa);
    sa.sun_path[0] = '\0';

    if(::bind(sock_fd, (struct sockaddr *) &sa, addrlen ) == -1 ) 
        return -1;   

    return sock_fd;
}

int CNet::wait_udp_unix(int sock_fd, int nTimeout)
{
    struct timeval timeout;

    fd_set fd_r;

    FD_ZERO(&fd_r);    
    FD_SET(sock_fd, &fd_r);

    if(nTimeout > 0)
    { 
        timeout.tv_sec = nTimeout;
        timeout.tv_usec = 0; 
        select(sock_fd+1, &fd_r, NULL, NULL, &timeout);
    }
    else
    {
        select(sock_fd+1, &fd_r, NULL, NULL, NULL);
    }

    if(!FD_ISSET(sock_fd, &fd_r))
        return -1;

    return 0;
}

int CNet::tcp_connect(int* netfd, const char* address, int port,int block, int timeout)
{    
	*netfd = SOCKET_INVALID;    
	if ((*netfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)    
	{        
		return SOCKET_CREATE_FAILED;    
	}	

	sockaddr_in mysock;	bzero((char*)&mysock,sizeof(mysock));	
	mysock.sin_family      = AF_INET;	
	mysock.sin_port        = htons(port);	
	mysock.sin_addr.s_addr = inet_addr(address); 	
	socklen_t addrlen = sizeof(struct sockaddr_in);    
/*
	int ret = ::connect (*netfd, (struct sockaddr *)&mysock, addrlen);	
	if (!block)    
	{        
		fcntl (*netfd, F_SETFL, O_RDWR|O_NONBLOCK);    
	}    

*/

//改成非阻塞connect  10毫秒的超时
		
	fcntl (*netfd, F_SETFL, O_RDWR|O_NONBLOCK);

	
	int ret = -1;
	do{
		ret = ::connect (*netfd, (struct sockaddr *)&mysock, addrlen);
		if(ret == 0 )
			break;

		if(ret == -1 && errno != EINPROGRESS)
		{
			log_error("socket connect errno, %d", errno);
			break;
		}

		struct pollfd   wfd[1];
	    wfd[0].fd     = *netfd;
	    wfd[0].events = POLLOUT;

		if( poll(wfd, 1, timeout) <= 0)		
			break;
		
		int error=-1;
		socklen_t len = sizeof(error);
		if(getsockopt(*netfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len) < 0)
			break;
			
		if(error != 0)
		{
			log_error ("socket connect error, %d", error);
			break;
		}
			
		ret = 0;		
	}while(0);
	return ret;
}

int CNet::tcp_bind(const char *addr, uint16_t port, int backlog)
{
	struct sockaddr_in inaddr;
	int reuse_addr = 1;
	int netfd;

	bzero (&inaddr, sizeof (struct sockaddr_in));
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons (port);

	char *end = strchr((char *)addr, ':');
	if(end)
	{
		char *p = (char *)alloca(end-addr+1);
		memcpy(p, addr, end-addr);
		p[end-addr] = '\0';
		addr = p;
	}
	if(strcmp(addr, "*")!=0 &&
			inet_pton(AF_INET, addr, &inaddr.sin_addr) <= 0)
	{
		log_boot ("invalid address %s:%d", addr, port);
		return -1;
	}

	if((netfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		log_boot ("make tcp socket error, %m");
		return -1;
	}

	setsockopt (netfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof (reuse_addr));
    setsockopt (netfd, SOL_TCP, TCP_NODELAY, &reuse_addr, sizeof (reuse_addr));
    reuse_addr = 60;
    setsockopt (netfd, SOL_TCP, TCP_DEFER_ACCEPT, &reuse_addr, sizeof (reuse_addr));
	
    if(::bind(netfd, (struct sockaddr *)&inaddr, sizeof(struct sockaddr)) == -1)
	{
		log_boot ("bind tcp %s:%u failed, %m", addr, port);
		close (netfd);
		return -1;
	}

	if(::listen(netfd, backlog) == -1)
	{
		log_boot ("listen tcp %s:%u failed, %m", addr, port);
		close (netfd);
		return -1;
	}

	log_boot ("listen on tcp %s:%u", addr, port);
	return netfd;
}

int CNet::udp_bind(const char *addr, uint16_t port, int rbufsz, int wbufsz)
{
	struct sockaddr_in inaddr;
	int netfd;

	bzero (&inaddr, sizeof (struct sockaddr_in));
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons (port);

	char *end = strchr((char *)addr, ':');
	if(end)
	{
		char *p = (char *)alloca(end-addr+1);
		memcpy(p, addr, end-addr);
		p[end-addr] = '\0';
		addr = p;
	}
    
	if(strcmp(addr, "*")!=0 &&
			inet_pton(AF_INET, addr, &inaddr.sin_addr) <= 0)
	{
		log_boot ("invalid address %s:%d", addr, port);
		return -1;
	}

	if ((netfd = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
		log_boot ("make udp socket error, %m");
		return -1;
	}

	if (::bind (netfd, (struct sockaddr *)&inaddr, sizeof(struct sockaddr)) == -1) {
		log_boot ("bind udp %s:%u failed, %m", addr, port);
		close (netfd);
		return -1;
	}

	if(rbufsz > 0)
	    setsockopt(netfd, SOL_SOCKET, SO_RCVBUF, &rbufsz, sizeof(rbufsz));
	
    if(wbufsz > 0)
	    setsockopt(netfd, SOL_SOCKET, SO_SNDBUF, &wbufsz, sizeof(wbufsz));

	log_boot ("listen on udp %s:%u", addr, port);
	
    return netfd;
}

int CNet::unix_bind(const char *path, int backlog, int flag)
{
	struct sockaddr_un unaddr;
	int netfd;
	int sock_opt = 0;    
	socklen_t addrlen = CNet::init_unix_addr(&unaddr, path);
	
    	if ((netfd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)  {
		log_boot ("%s", "make unix socket error, %m");
		return -1;
	}


    	setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));    

       if(flag)
        	fcntl(netfd, F_SETFL, O_NONBLOCK);

	if (::bind (netfd, (struct sockaddr *)&unaddr, addrlen) == -1) {
		log_boot ("bind unix %s failed, %m", path);
		close (netfd);
		return -1;
	}

	if (listen (netfd, backlog) == -1) {
		log_boot ("listen unix %s failed, %m", path);
		close (netfd);
		return -1;
	}

	log_boot ("listen on unix %s, fd=%d", path, netfd);
	
    	return netfd;
}

