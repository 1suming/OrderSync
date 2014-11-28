#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <decode_unit.h>
#include <client_unit.h>
#include <helper_unit.h>
#include <game_unit.h>
#include <listener.h>
#include <memcheck.h>
#include <log.h>
#include <sys/types.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <timestamp.h>
#include <CHelper_pool.h>
#include <sstream>
#include <limits.h>
#include "ICHAT_PacketBase.h"

#include "watchdog.h"
extern Watchdog *LogFile;

#include "clib_log.h"
extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;

unsigned long g_iFlow;

#define __ADD__(v, c) __sync_add_and_fetch(&v, c)
#define __FLOW__() g_iFlow == ULLONG_MAX ? 1 : __ADD__(g_iFlow, 1)

HTTP_SVR_NS_BEGIN

void 
HexDumpImp0(const void *pdata, unsigned int len, int num)
{
	if(pdata == 0 || len == 0)
		return;
	
	int cnt  = 0;
	int n    = 0;
	int cnt2 = 0;
	
	const char *data = (const char *)pdata;
	cout<<"Address               Hexadecimal values                  Printable\n";
	cout<<"-------  -----------------------------------------------  -------------\n";
	cout<<"num:[" << num << "]";
	cout << "\n";
	
	unsigned char buffer[20];
	unsigned int rpos = 0;

	while ( 1 )
	{
		if(len <= rpos)
			break;

		if(len >= rpos + 16) {
			memcpy(buffer, data + rpos, 16);
			rpos += 16;
			cnt   = 16;
		} else {
			memcpy(buffer, data + rpos, len - rpos);
			cnt  = len - rpos;
			rpos = len;
		}

		if(cnt <= 0) return;

		cout << setw(7) << ( int ) rpos << "  ";

		cnt2 = 0;
		for ( n = 0; n < 16; n++ ) {
			cnt2 = cnt2 + 1;
			if ( cnt2 <= cnt )
				cout << hex << setw(2) << setfill ( '0' ) <<  (unsigned int)buffer[n];
			else
				cout << "  ";

			cout << " ";
		}

		cout << setfill ( ' ' );
		cout << " ";

		cnt2 = 0;
		for ( n = 0; n < 16; n++ ) {
			cnt2 = cnt2 + 1;
			if ( cnt2 <= cnt ) {
				if ( buffer[n] < 32 || 126 < buffer[n] )
					cout << '.';
				else
					cout << buffer[n];
			}
		}

		cout << "\n";
		cout << dec;
	}
}


extern CHelperPool*	_helperpool;

CDecoderUnit::CDecoderUnit(CListener* owner): 
	_pollerUnit(NULL),
	_webTimer (NULL),
	_helperTimer (NULL),	
	_webUnit (NULL),
	_gameUnit (NULL),
	_listener (owner),
	_flag(0),
	_fd(0),
	_conn_type(CONN_OTHER)
{
}

CDecoderUnit::~CDecoderUnit (void)
{	
	if(_flag > 0) {
		std::map<int, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.find(_flag);
		if(iter != _helperpool->m_objmap.end())
		{
			_helperpool->m_objmap.erase(iter);
		}
		else
		{
			log_error("%s||Cannot find uid:[%d]", __FUNCTION__, _flag);
		}
	}
	
	log_debug("delete object from map, length[%d]", (int)_helperpool->m_objmap.size());	
	DELETE (_webUnit);
	DELETE (_gameUnit);
}

int CDecoderUnit::listener_input (void) // called by CListener::proc_request 
{
	NEW(CClientUnit(this, _fd), _webUnit);
	if (NULL == _webUnit) {
		log_error ("create CClient object failed, errno[%d], %m", errno);
		return -1;
	}

	NEW(CGameUnit(this), _gameUnit);
	if(NULL == _gameUnit) {
		log_error ("create helper object failed, errno[%d], %m", errno);
		return -1;
	}

	//set user id
	_webUnit->set_netfd(_fd);
	if (_webUnit->Attach() == -1) // 关联 epoll 事件
	{
		log_error ("Invoke CClient::Attach() failed");
		return -1;
	}
	return 0;
}

int CDecoderUnit::web_input (void)
{
	int webfd   = _webUnit->get_netfd ();
	int stage   = _webUnit->get_state ();

	switch (stage)
	{
		case CONN_DATA_ERROR:
			log_warning("web input decode error, webfd[%d], stage[%d]", webfd, stage);
			_listener->complete (this);
			return POLLER_COMPLETE;

		case CONN_DISCONNECT:
			//log_notice("web input disconnect by user, webfd[%d], stage[%d]", webfd, stage);
			_listener->complete(this);
			return POLLER_COMPLETE;

		case CONN_DATA_RECVING:
		case CONN_IDLE:
			return POLLER_SUCC;

		case CONN_RECV_DONE:			
			return POLLER_SUCC;

		case CONN_FATAL_ERROR:
			log_error("web input fatal error, webfd[%d], stage[%d]", webfd, stage);
			_listener->complete (this);
			return POLLER_COMPLETE;

		default:
			log_error("web input unknow status, webfd[%d], stage[%d]", webfd, stage);
			_listener->complete (this);
			return POLLER_COMPLETE;
	}
}


int CDecoderUnit::ProcessUserClose()
{
	if(_flag <= 0)
		return 0;
	g_pDebugLog->logMsg("%s||Connect close, uid:[%d]", __FUNCTION__, _flag);
	NETOutputPacket packet;
	packet.Begin(CLIENT_CLOSE_PACKET);
	packet.WriteInt(_flag);
	packet.WriteInt(TGlobal::_svid);
	packet.End();


	if(TGlobal::_udpSwitch)
		LogFile->writeUserLeaveHall(_flag, TGlobal::_ip, TGlobal::_port);


	g_pDebugLog->logMsg("_gameUnit->tid [%d]", _gameUnit->tid);
	
	int svid = 0;
    if ( _gameUnit->tid > 0 )
    {
        svid = ((_gameUnit->tid)>>16)/200;
    }
    else
    {
        svid = TGlobal::LoopSvid(_helperpool->m_svidlist);
    }
	g_pDebugLog->logMsg("svid [%d]", svid);
	CHelperUnit *pHelperUnit = NULL;
	map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
	if(iterHelper != _helperpool->m_helpermap.end())
	{
		pHelperUnit = iterHelper->second;
		if(pHelperUnit != NULL)
		{
			pHelperUnit->append_pkg(packet.packet_buf(), packet.packet_size());
			pHelperUnit->send_to_logic(_helperTimer);
		}
	}
	else
	{
		g_pErrorLog->logMsg("%s||Cannot find helper, svid:[%d]", __FUNCTION__, svid);
	}	

	return 0;
}
 
int CDecoderUnit::web_output (void)
{
	int stage = _webUnit->get_state();
	switch (stage)
	{
		case CONN_SEND_DONE:
			log_debug ("reponse data completed, netfd[%d]", _webUnit->get_netfd());
		    return POLLER_COMPLETE;
		case CONN_DATA_SENDING:
		case CONN_APPEND_SENDING:
		case CONN_APPEND_DONE:
			if (_webUnit->send() < 0)
			{
				_listener->complete (this);
				return POLLER_COMPLETE;
			}

			stage = _webUnit->get_state ();
			switch (stage)
			{
				case CONN_SEND_DONE:
					log_debug ("response data complete, netfd[%d]", _webUnit->get_netfd());
					return POLLER_COMPLETE;

				case CONN_DATA_SENDING:
				case CONN_APPEND_SENDING:
				case CONN_APPEND_DONE:
					log_debug ("responsing data, netfd[%d]", _webUnit->get_netfd());
					return POLLER_COMPLETE;

				default:
					log_debug ("response data to client failed, netfd[%d]", _webUnit->get_netfd());
					_listener->complete (this);
					return POLLER_COMPLETE;
			}

		default:
			log_debug ("response data to client failed, netfd[%d],stage[%d]", _webUnit->get_netfd(), stage);
			_listener->complete (this);
			return POLLER_COMPLETE;
	}
}
 
int CDecoderUnit::web_hangup(void)
{
	_listener->complete(this);
	return POLLER_COMPLETE;    
} 

int CDecoderUnit::web_timer(void)
{
	_listener->complete(this);
	return POLLER_COMPLETE;
}

void CDecoderUnit::complete (void)
{
	return _listener->complete (this);
}
HTTP_SVR_NS_END

