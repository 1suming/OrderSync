#ifndef __LISTENER_H__
#define __LISTENER_H__

#include <poller.h>
#include <noncopyable.h>

class CTimerList;
class CPollerUnit;

HTTP_SVR_NS_BEGIN

class CDecoderUnit;

class CListener : public CPollerObject, private noncopyable
{
public:
	CListener (const char* addr, uint16_t port, int acceptcnt);
	virtual ~CListener (void);
    
	virtual int InputNotify(void);
	int Attach (CPollerUnit*, CTimerList*, CTimerList*, int);
    void complete (CDecoderUnit* decoderunit);

private: //method
    CDecoderUnit* create_decoderunit (int);
    int proc_accept (struct sockaddr_in* peer, socklen_t* peerSize);
    int proc_request (struct sockaddr_in* peer);

private: //property
    CPollerUnit*    _pollerunit;
    CTimerList*     _webtimerlist;
    CTimerList*     _helpertimerlist;
    int             _accept_cnt;
    int             _newfd_cnt;
    int*            _fd_array;
	struct sockaddr_in*  _peer_array;
    int             _accept_num;
	uint16_t        _bindPort;
	char            _bindAddr[128];
	unsigned int    _flag;
}; 

HTTP_SVR_NS_END
#endif

