#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <poller.h>
#include <timerlist.h>
#include <decode_unit.h>
#include <listener.h>
#include <memcheck.h>
#include <log.h>
#include <net.h>
#include <client_unit.h>
#include <helper_unit.h>

HTTP_SVR_NS_BEGIN

CListener::CListener (const char* bindIp, uint16_t port, int acceptcnt) : 
CPollerObject (NULL, -1),
_pollerunit (NULL),
_webtimerlist (NULL),
_helpertimerlist (NULL),
_accept_cnt (acceptcnt),
_newfd_cnt (0),
_fd_array (NULL),
_peer_array(NULL),
_flag (0)
{
    strncpy (_bindAddr, bindIp, sizeof(_bindAddr) - 1);
    _bindPort = port;
    //calloc fd array
    _fd_array = (int*) CALLOC (acceptcnt, sizeof(int));
    assert (NULL != _fd_array);
    memset (_fd_array, -1, acceptcnt * sizeof(int));
	_peer_array=(struct sockaddr_in *) CALLOC (acceptcnt,sizeof(struct sockaddr_in));
    assert (NULL != _peer_array);	
}

CListener::~CListener()
{
    //free fd array
    FREE_CLEAR(_fd_array);
	FREE_CLEAR(_peer_array);
}

CDecoderUnit* CListener::create_decoderunit (int fd)
{
    CDecoderUnit* decoderunit = NULL;
    NEW (CDecoderUnit(this), decoderunit);
    if (NULL == decoderunit)
    {
        log_error ("create CDecoderUnit object failed.");
        return NULL;
    }
	decoderunit->set_netfd(fd); // 关联netfd
    decoderunit->pollerunit (_pollerunit);
    decoderunit->set_web_timer (_webtimerlist);				//web超时时间(Client)
    decoderunit->set_helper_timer (_helpertimerlist);		//helper超时时间(Alloc)
    return decoderunit;
}

int CListener::Attach (CPollerUnit* unit,
                       CTimerList* webtimerlist,
                       CTimerList* helpertimerlist,
                       int blog)
{
	if((netfd = CNet::tcp_bind(_bindAddr, _bindPort, blog)) == -1)
	{
		log_error ("bind addr[%s], port[%d] failed.", _bindAddr, _bindPort);
        return -1;
	}
    _pollerunit      = unit;
    _webtimerlist    = webtimerlist;
    _helpertimerlist = helpertimerlist;
	EnableInput ();  // 设置读事件
	return AttachPoller(unit); // 将 netfd 关联 epoll_fd
}

int CListener::proc_request (struct sockaddr_in* peer)
{
    for (int i = 0; i < _newfd_cnt; ++i)
    {
        if (_fd_array[i] == -1)
        {
            continue;
        }
        //create CDecoderUnit object
        CDecoderUnit* pDecoderunit = NULL;
        pDecoderunit = create_decoderunit (_fd_array[i]);
        if (NULL == pDecoderunit)
        {
            log_error ("create CDecoderUnit object failed, address[%s:%d], fd[%d]", _bindAddr, _bindPort, netfd);

            ::close (_fd_array[i]);
            _fd_array[i] = -1;

            continue;
        }
		pDecoderunit->set_ip(peer[i].sin_addr.s_addr);
		log_debug("client ip=%s", inet_ntoa(peer[i].sin_addr));
			
        if(pDecoderunit->listener_input() < 0) // init clientunit gameunit
        {
            ::close(_fd_array[i]);
            _fd_array[i] = -1;

            DELETE (pDecoderunit);
            pDecoderunit = NULL;
        }
		pDecoderunit->get_web_unit()->set_netfd(_fd_array[i]);
		log_debug("*STEP: accept new tcp connection, address[%s:%d], fd[%d]", _bindAddr, _bindPort, _fd_array[i]);
    }

    return 0;
}

int CListener::proc_accept (struct sockaddr_in* peer, socklen_t* peerSize)
{
    int newfd = -1;

    memset (_fd_array, -1, _accept_cnt * sizeof(int));
    _newfd_cnt  = 0;

    for (int i = 0; i < _accept_cnt; ++i)
    {
		newfd = ::accept (netfd, (struct sockaddr*)&peer[i], peerSize);
		log_debug ("proc_accept: accept new connection fd[%d]", newfd);
		if (newfd < 0)
		{
			if (errno == EINTR)
	     	{
	        	continue;
	    	}

	    	if (errno == EAGAIN)
	    	{
	       	 	return -1;
	   	 	}

			log_error ("*STEP: accept new connection failed, address[%s:%d], fd[%d], msg[%m]", _bindAddr, _bindPort, netfd);
	    }
	        
	    _fd_array[i] = newfd;
	    _newfd_cnt++;
	}

    return 0;
}

int CListener::InputNotify (void)
{
    struct sockaddr_in  *peer =_peer_array;
    socklen_t           peerSize;
    int                 ret;

    peerSize = sizeof (struct sockaddr_in);
	while (true)
	{
	    ret = proc_accept (peer, &peerSize);
		
	    if (_newfd_cnt <= 0)
	    {
	        log_error ("invoke proc_accept failed, new fd count = %d", _newfd_cnt);
	        return POLLER_SUCC; 
	    }

	    if (_newfd_cnt > 0)
	    {
	        proc_request (peer);
	    }

	    if (ret < 0)
	    {
	        return POLLER_SUCC;
	    }
	}
}

void CListener::complete (CDecoderUnit* decoderunit)
{
	if(decoderunit->get_conn_type() == CONN_CLINET)//如果是客户端，上报用户退出
	{
		decoderunit->ProcessUserClose();
	}
	
	int webfd   = 0;
    if (NULL != decoderunit->_webUnit)
    {
        webfd = decoderunit->_webUnit->get_netfd ();
    }

    log_debug("*STEP: handle HTTP request completed, listenfd[%d], webfd[%d]", netfd, webfd);
    
    DELETE (decoderunit);
}

HTTP_SVR_NS_END

