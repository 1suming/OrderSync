#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <log.h>
#include <game_unit.h>
#include <net.h>
#include <memcheck.h>
#include <extend_http.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <socket.h>
#include <CHelper_pool.h>
#include <sstream>
#include <client_unit.h>
#include "ICHAT_PacketBase.h"
#include "EncryptDecrypt.h"

#include "clib_log.h"
extern clib_log* g_pErrorLog;
extern clib_log* g_pDebugLog;

HTTP_SVR_NS_BEGIN
extern CHelperPool* _helperpool;

class TPkgHeader;
class CClientUnit; 

CGameUnit::CGameUnit(CDecoderUnit* decoderunit) : CPollerObject (decoderunit->pollerunit()),
		_stage (CONN_IDLE),
		_decoderunit(decoderunit),
		_send_error_times(0),
		_r(*_helpMp),
		_w(*_helpMp)
{
	addr = "0.0.0.0";
	port = 0;
	tid = 0;
}

CGameUnit::~CGameUnit ()
{
	reset_helper();
}

int CGameUnit::send_to_logic (CTimerList* lst)
{
	_helperTimer = lst;
	EnableInput ();
	if (connect() < 0)
	{
		reset_helper();
		log_debug("Connect logic server failed!");
		return -1;
	}
	AttachTimer(_helperTimer);
	return send_to_cgi();
}

int CGameUnit::reConnect(void)
{
	int ret = -1;
	log_debug("*STEP: PROXY CNet::reConnect before");
	ret = CNet::tcp_connect(&netfd, addr.c_str(), port, 0, TGlobal::_timerConnect);
	log_debug("*STEP: PROXY CNet::reConnect ret :[%d], netfd:[%d]", ret, netfd);
	if (ret < 0)
	{
		if (ret == SOCKET_CREATE_FAILED)
		{
			log_error("*STEP: helper create socket failed, errno[%d], msg[%s]", errno, strerror(errno));
			return -1;
		}
		if(errno != EINPROGRESS)
		{
			log_error("*STEP: PROXY connect to logic failed, errno[%d], msg[%s]",errno , strerror(errno));
			return -1;
		}
		_stage = CONN_CONNECTING;
		log_debug("*STEP: PROXY connecting to logic, unix fd[%d]", netfd);
		goto exit;
	}
	_stage = CONN_CONNECTED;

exit:
	return AttachPoller();
}

int CGameUnit::connect (void)
{
	int ret = -1;

	log_debug("*STEP: PROXY CNet::tcp_connect before");

	log_debug("addr [%s]",addr.c_str());

	if (_stage == CONN_IDLE)
	{
		ret = CNet::tcp_connect(&netfd, addr.c_str(), port, 0, TGlobal::_timerConnect);
	}
	else
	{
		if (netfd < 0)
		{
			ret = CNet::tcp_connect(&netfd,addr.c_str(), port, 0, TGlobal::_timerConnect);
		}
		else
		{
			ret = 0;
		}
	}

	log_debug("*STEP: PROXY CNet::tcp_connect ret :[%d], netfd:[%d]", ret, netfd);

	if (ret < 0)
	{	
		log_error("Connect GameServer, uid:[%d], tid[%d], ip:[%s], port:[%d]", GetClientUid(), tid, addr.c_str(), port);
		return -1;
	}
	_stage = CONN_CONNECTED;
exit:
	return AttachPoller();
}

int CGameUnit::send_to_cgi(void)
{
	int ret = 0;
	int reConnectCount = 0;
	log_debug("_w data size:[%d]", _w.data_len());
logic:		
	if (_w.data_len() != 0)
	{
		ret = ::send (netfd, _w.data(), _w.data_len(), 0);
		log_debug("sent packet size:[%d]", ret);
		if(-1 == ret)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
			{
				//加入发送错误次数判断
				_send_error_times++;
				if(_send_error_times >= 50)
				{
					if(_decoderunit != NULL)
					{
						CClientUnit* clt = _decoderunit->get_web_unit();
						if(clt != NULL)
						{
							g_pErrorLog->logMsg("%s|CGameUnit::send_to_cgi Send error more,%d|%d|", __FUNCTION__, clt->get_uid(), clt->_api);
						}	
					}
					reset_helper();
					_stage = CONN_FATAL_ERROR;
					return -1;
				}
				EnableOutput ();
				ApplyEvents ();		
				return CONN_DATA_SENDING;
			}
		
			log_error("*STEP: sending package to logic server failed, ret code[%d], errno[%d]", ret, errno);	
			reset_helper();
			_stage = CONN_FATAL_ERROR;
			return -1;
		}

		if(ret == (int)_w.data_len())
		{
			_w.skip(ret);
			DisableOutput ();
			ApplyEvents();
			_stage = CONN_SEND_DONE;
			//重置发送错误次数
			_send_error_times = 0;
			log_debug ("*STEP: sent package to logic server, netfd[%d] packet size[%d]", netfd, ret);
			return ret;
		}
		else if (ret < (int)_w.data_len())
		{
			EnableOutput ();
			ApplyEvents ();	
			_w.skip(ret);
			//重置发送错误次数
			_send_error_times = 0;
			_stage = CONN_DATA_SENDING;
			return ret;
		}
		DisableInput ();
		DisableOutput ();
		ApplyEvents();
		_stage = CONN_FATAL_ERROR;
		log_debug ("*STEP: sending package to logic server in exception, netfd[%d]", netfd);
		return ret;
	}
	return ret;
}


int CGameUnit::recv_from_cgi(void)
{
	char _curRecvBuf[MAX_HELPER_RECV_BUF] = {0};
	int ret = ::recv (netfd, _curRecvBuf, MAX_HELPER_RECV_BUF, 0);	
	log_error("*STEP: recv from logic server fd[%d] recv len[%d] this time", netfd, ret);	
	if (-1 == ret)
	{
		log_debug("*STEP: ret = -1");		
		if((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
		{
			_stage = CONN_DATA_RECVING;
			return 0;
		}		
		_stage = CONN_FATAL_ERROR;
		log_error ("*STEP: recv from logic server failed, unix fd[%d]", netfd);
		return -1;
	}

	if (0 == ret)
	{
		_stage = CONN_DISCONNECT;
		log_debug ("*STEP:logic server close, unix fd[%d]", netfd);
		return -1;
	}
		
	_r.append(_curRecvBuf, ret);
	if(process_pkg() < 0)
		return -1;
	_stage = CONN_RECV_DONE;
	log_debug("*STEP: recving from logic server complete, unix fd[%d] current recv len[%d]", netfd, ret);
	return ret;
} 

int CGameUnit::InputNotify (void)
{
	update_timer();
	int ret = recv_from_cgi();
	if (ret < 0)
	{
		log_error("call recv failed, helper client netfd[%d]", netfd); 
		reset_helper();
		_decoderunit->complete();
		return POLLER_COMPLETE;
	}
	return POLLER_SUCC;
}

int CGameUnit::OutputNotify (void)
{
	update_timer();
	if(send_to_cgi() < 0
)
	{
		reset_helper();
		_decoderunit->complete();
	}
	return POLLER_SUCC;
}

int CGameUnit::HangupNotify (void)
{
	log_warning("CHelperUnit object hangup notify: fd[%d]", netfd);
	update_timer();
	process_pkg();
	reset_helper();
	_decoderunit->complete();
	return POLLER_COMPLETE;
}

void CGameUnit::TimerNotify (void)
{
	log_warning ("*STEP: timer expired, unix fd[%d], timeout.", netfd);
	reset_helper();
	_decoderunit->complete();
}

void CGameUnit::update_timer(void)
{
	DisableTimer();
	AttachTimer(_helperTimer);
}

int CGameUnit::append_pkg(const char* buf, unsigned int len)
{
	_w.append(buf, len);
	return 0;
}

int CGameUnit::process_pkg(void)
{
	int headLen  = sizeof(struct TPkgHeader);
	int totalLen = _r.data_len();
	log_error("forward packets to the client start headLen[%d],totalLen[%d]", headLen, totalLen);
	while( totalLen > 0)
	{
		if (totalLen < headLen)
			break;
		TPkgHeader *pHeader = (struct TPkgHeader*)(_r.data());
		if(pHeader == NULL)//空指针判断，做容错处理
		{
			log_error("CGameUnit::process_pkg pHeader==NULL");
			return -1;
		}	
		int contenLen = ntohs(pHeader->length);
		log_error("helper packet body length:[%d], cmd:[%#x]", ntohs(pHeader->length), ntohs(pHeader->cmd));
		if(contenLen < 0 || contenLen>10*1024)
		{
			log_error("CGameUnit::process_pkg contenLen invalid");
			return -1;
		}
		int pkglen = sizeof(short) + contenLen;
		if (totalLen < pkglen)
			 break;
		NETInputPacket tranPacket;
		tranPacket.Copy(_r.data(), pkglen);
		int cmd = tranPacket.GetCmdType();
		CClientUnit *clt = _decoderunit->get_web_unit();
		if(clt != NULL)
		{
			if(TGlobal::_debugLogSwitch && clt->get_uid()>0)
			{
				g_pDebugLog->logMsg("%s|0x%x|%d|%d|", __FUNCTION__, cmd, clt->get_uid(), clt->_api);
			}
			
			if(clt->get_state() != CONN_FATAL_ERROR)
			{
				clt->add_rsp_buf(tranPacket.packet_buf(), tranPacket.packet_size());
				if(clt->send() < 0)
					return -1;
			}
			else
			{
				log_error("CGameUnit::process_pkg clt->get_state()==CONN_FATAL_ERROR");
				return -1;
			}
		}
		_r.skip(pkglen);
		totalLen -= pkglen;
		if(cmd==SERVER_COMMAND_LOGOUT_SUCCESS)//用户正常退出房间，不断开clientunit连接
		{
			reset_helper();
			break;
		}
		else if(cmd == SERVER_COMMAND_CHANGE_GAMESERVER)
		{
			CEncryptDecrypt Encry;
			Encry.DecryptBuffer(&tranPacket);
			int uid = tranPacket.ReadInt();
			string _ip = tranPacket.ReadString();	
			int _port = tranPacket.ReadInt();
			reset_helper();
			//reset_helper()会重置ip port 一定要在后面赋值
			addr = _ip;
			port = _port;
			break;
		}		
	}
	log_error("forward packets to the client end");
	return 0;
}

void CGameUnit::reset_helper(void)
{
	log_debug("reset helper object");
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );
	DisableInput();
	DisableOutput();
	ApplyEvents();
	CPollerObject::DetachPoller();
	if(netfd > 0)
	{
		::close(netfd);
	}
	netfd  = -1;
	_stage = CONN_IDLE;
	tid = 0;
	addr = "0.0.0.0";
	port = 0;
}

int CGameUnit::GetClientUid()
{
	if( NULL == _decoderunit)
	{
		return 0;
	}

	CClientUnit* pClient = _decoderunit->get_web_unit();
	if( NULL == pClient)
	{
		return 0;
	}
	return pClient->get_uid();
}

HTTP_SVR_NS_END

