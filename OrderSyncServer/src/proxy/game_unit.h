#ifndef __GAME_UNIT_H__
#define __GAME_UNIT_H__

#include <zlib.h>
#include <poller.h>
#include <timerlist.h>
#include <noncopyable.h>
#include <decode_unit.h>
#include <cache.h>
#include <memcheck.h>
#include <client_unit.h>
#include <vector>
using std::vector;

#define MAX_HELPER_RECV_BUF 65536
using namespace comm::sockcommu;

extern CMemPool *	_helpMp;

HTTP_SVR_NS_BEGIN
class TPkgHeader;
class CDecoderUnit;
class CClientUnit;
class CGameUnit :	public CPollerObject,	private CTimerObject, private noncopyable
{
public: //method
	CGameUnit(CDecoderUnit* decoderunit);
	virtual ~CGameUnit(void);
    
    int send_to_logic (CTimerList* lst); 
    int append_pkg(const char* buf, unsigned int len);
	int connect (void);
	
  	inline void set_netfd (int fd) 
	{ 
		netfd = fd; 
	}

	inline int get_netfd (void) 
	{ 
		return netfd; 
	}
public:
	std::string addr;
	int			port;
	int         tid;

private:	
    virtual int InputNotify (void);
	virtual int OutputNotify (void);
	virtual int HangupNotify (void);
	virtual void TimerNotify (void);
	
	int reConnect(void);
    int send_to_cgi(void);
	int recv_from_cgi(void);
	int process_pkg(void);
	void update_timer(void);

	int GetClientUid();
		
	
public:
	void reset_helper(void);

private:
	CConnState      _stage;
	CTimerList*     _helperTimer;
	CDecoderUnit*   _decoderunit;
	short 			_send_error_times;

	//连接类型: TCP_SOCKET\UDP_SOCKET\UNIX_SOCKET
	int         _type;
	//对端信息: 
	CSocketAddr _addr;
	//读请求cache
	CRawCache   _r;
	//写回复cache
	CRawCache   _w;
};

HTTP_SVR_NS_END
#endif

