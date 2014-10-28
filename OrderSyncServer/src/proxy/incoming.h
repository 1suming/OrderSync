#ifndef __INCOMING_H__
#define __INCOMING_H__

#include <global.h>
#include <noncopyable.h>
#include <stdint.h>
#include <poller.h>
#include <timerlist.h>

HTTP_SVR_NS_BEGIN

class CListener;
class CIncoming : private noncopyable {
public: //method
    CIncoming (void);
    virtual ~CIncoming (void);
    static CIncoming* Instance (void);
    static void Destroy (void);
    int open (void);
    int run (void);
	int InitHelperUnit();
private:
	int _active_helper(const int _level, const int _svid);

private: //property
    CPollerUnit*    _pollerunit;
    CListener*      _listener;
    CTimerUnit*     _timerunit;
    CTimerList*     _webtimerlist;    
    CTimerList*     _helpertimerlist;    
    int             _epwaittimeout;
	CTimerList*		_realtimerlist;
};

HTTP_SVR_NS_END

#endif //__INCOMING_H__

