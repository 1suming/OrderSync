#ifndef __HELPER_CLIENT_H__
#define __HELPER_CLIENT_H__

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
#include <game_unit.h>

#define MAX_HELPER_RECV_BUF 65536
using namespace comm::sockcommu;

extern CMemPool*	_helpMp;

HTTP_SVR_NS_BEGIN
class TPkgHeader;
class CHelperUnit : public CPollerObject,	
					private CTimerObject, 
					private noncopyable
{
public: //method
	CHelperUnit(CPollerUnit* unit);
	virtual ~CHelperUnit (void);
    
    int send_to_logic (CTimerList* lst); 
    int append_pkg(const char* buf, unsigned int len);
	int connect (void);

	int send_to_logic();
	
  	inline void set_netfd (int fd) 
	{ 
		netfd = fd; 
	}

	inline int get_netfd (void) 
	{ 
		return netfd; 
	}
private:
	string IpMap(string& ip);

	CClientUnit* GetClientUintByUid(const int &uid, CDecoderUnit **pDecoder);
	CClientUnit* _get_client_by_id(const unsigned long& flow, CDecoderUnit** ppDecoder);

	int _worker_stat_chk(NETInputPacket* pack); 	/* Notify完成任务 */
	int _pay_res(NETInputPacket* pack); 	/* 返回支付请求结果 */

public:
	std::string addr;
	int			port;

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
	void reset_helper(void);
private:
	void SendClientClose(int uid);

private:
	CConnState      _stage;
	CTimerList*     _helperTimer;
    char            _curRecvBuf[MAX_HELPER_RECV_BUF];
	short 			_send_error_times;

	//��������: TCP_SOCKET\UDP_SOCKET\UNIX_SOCKET
	int         _type;
	//�Զ���Ϣ: 
	CSocketAddr _addr;
	//������cache
	CRawCache   _r;
	//д�ظ�cache
	CRawCache   _w;
};

HTTP_SVR_NS_END
#endif

