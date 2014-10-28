#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <log.h>
#include <helper_unit.h>
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
#include "watchdog.h"
#include "Json/json.h"
#include "defs.h"
#include "clib_log.h"

extern Watchdog *LogFile;

extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;

HTTP_SVR_NS_BEGIN
extern CHelperPool		*_helperpool;
extern server_stat_t 	*gSvrStat;
extern server_switch_t 	*gSvrSwitch;

using namespace Json;

class TPkgHeader;
class CClientUnit; 

CHelperUnit::CHelperUnit(CPollerUnit* unit) : CPollerObject (unit),
		_stage (CONN_IDLE),
		_send_error_times(0),
		_r(*_helpMp),
		_w(*_helpMp)
{
}

CHelperUnit::~CHelperUnit ()
{
	reset_helper();
}


string CHelperUnit::IpMap(string& ip)
{
	string retIp = "";
	map<string, string>::iterator iter = _helperpool->m_ipmap.find(ip);
	if(iter != _helperpool->m_ipmap.end())
	{
		retIp = iter->second;
	}
	return retIp;
}

int CHelperUnit::send_to_logic (CTimerList* lst)
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

int CHelperUnit::reConnect(void)
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

int CHelperUnit::connect (void)
{
	int ret = -1;
	log_debug("*STEP: PROXY CNet::tcp_connect before");

	if (_stage == CONN_IDLE)
	{
		ret = CNet::tcp_connect(&netfd, addr.c_str(), port, 0, TGlobal::_timerConnect);
	}
	else
	{
		if (netfd < 0) // 这里判断是否连接存在
		{
			ret = CNet::tcp_connect(&netfd, addr.c_str(), port, 0, TGlobal::_timerConnect);
		}
		else
		{
			ret = 0;
		}
	}

	log_debug("*STEP: PROXY CNet::tcp_connect ret :[%d], netfd:[%d]", ret, netfd);

	if (ret < 0) {	
		log_error("Connect Helper error, ip:[%s], port:[%d]", addr.c_str(), port);
		return -1;
	}
	_stage = CONN_CONNECTED;
	
exit:
	return AttachPoller();
}

int CHelperUnit::send_to_cgi(void)
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
				_send_error_times++;
				if(_send_error_times >= 50)
				{
					g_pErrorLog->logMsg("CHelperUnit::send_to_cgi, send error more");
					reset_helper();
					_stage = CONN_FATAL_ERROR;
					return -1;
				}
				EnableOutput ();
				ApplyEvents ();		
				return CONN_DATA_SENDING;
			}
		
			log_error("*STEP: sending package to logic server failed, ret code[%d], errno:[%d], ip:[%s], port:[%d]", ret, errno, addr.c_str(), port);	
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
			_stage = CONN_DATA_SENDING;
			//重置发送错误次数
			_send_error_times = 0;
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


int CHelperUnit::recv_from_cgi(void)
{
	int ret = ::recv (netfd, _curRecvBuf, MAX_HELPER_RECV_BUF, 0);	
	log_error("*STEP: recv from logic server fd[%d] recv len[%d] this time", netfd, ret);	
	if (-1 == ret) {
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

	if (0 == ret) {
		_stage = CONN_DISCONNECT;
		log_error("*STEP:logic server close, unix fd[%d]", netfd);
		reConnect();
		return -1;
	}
		
	_r.append(_curRecvBuf, ret);
	if(process_pkg() < 0)
		return -1;
	_stage = CONN_RECV_DONE;
	log_debug("*STEP: recving from logic server complete, unix fd[%d] current recv len[%d]", netfd, ret);
	return ret;
} 

int CHelperUnit::InputNotify (void)
{
	update_timer();
	int ret = recv_from_cgi();
	if (ret < 0) {
		log_error("call recv failed, helper client netfd[%d]", netfd); 
		reset_helper();
		return POLLER_COMPLETE;
	}
	return POLLER_SUCC;
}

int CHelperUnit::OutputNotify (void)
{
	update_timer();
	send_to_cgi();
	return POLLER_SUCC;
}

int CHelperUnit::HangupNotify (void)
{
	log_debug("CHelperUnit object hangup notify: fd[%d]", netfd);
	update_timer();
	process_pkg();
	reset_helper();
	return POLLER_COMPLETE;
}

void CHelperUnit::TimerNotify (void)
{
	log_debug ("*STEP: timer expired, unix fd[%d], timeout.", netfd);
	reset_helper();
}

void CHelperUnit::update_timer(void)
{
	DisableTimer();

	if (_helperTimer) {
		AttachTimer(_helperTimer); 
	}
}

int CHelperUnit::append_pkg(const char* buf, unsigned int len)
{
	_w.append(buf, len);
	return 0;
}

int CHelperUnit::process_pkg(void)
{
	int 			headLen = 0, totalLen = 0, pkglen = 0;
	CEncryptDecrypt ed;

	log_debug("--------------- CHelperUnit::process_pkg begin --------------");

	headLen  = sizeof(struct TPkgHeader);
	totalLen = _r.data_len();

	log_debug("forward packets to headLen [%d]:",headLen);
	log_debug("forward packets to totalLen [%d]:",totalLen);
	log_debug("forward packets to the client start headLen[%d],totalLen[%d]", headLen, totalLen);

	while (totalLen > 0) {
		if (totalLen < headLen) break;

		TPkgHeader *pHeader = (struct TPkgHeader*)(_r.data());
		if(pHeader == NULL)
		{
			log_info("pHeader==NULL");
			return -1;
		}
		int contenLen = ntohs(pHeader->length);
		log_error("helper packet body length:[%d], cmd:[%#x]", ntohs(pHeader->length), ntohs(pHeader->cmd));
		if(contenLen < 0 || contenLen > 10*1024) {
			return -1;
		}

		pkglen = sizeof(short) + contenLen;
		if (totalLen < pkglen) break;

		NETInputPacket tranPacket;
		tranPacket.Copy(_r.data(), pkglen);
		unsigned short nCmd = tranPacket.GetCmdType();
		ed.DecryptBuffer(&tranPacket); /* 瑙ｇ爜 */

		if(_helperpool == NULL) {
			log_error("_helperpool is null");
		} else {
			switch (nCmd) {
				case INTER_CMD_RES:
					_pay_res(&tranPacket);
					_r.skip(pkglen); /* cache 涓彲鑳芥湁澶氫釜鍖呴渶瑕佽缃甶ndex */
					totalLen -= pkglen;
					break;
				case INTER_CMD_WAITER_STAT_RES:
				case INTER_CMD_NOTIFY_STAT_RES:
					_worker_stat_chk(&tranPacket);
					_r.skip(pkglen);
					totalLen -= pkglen;
					break;
				default:
					log_error("cmd:[%X] is invalied.", nCmd);
					break;
			}
		}
	}
	return 0;
}

void CHelperUnit::SendClientClose(int uid)
{
	NETOutputPacket transPacket;
	transPacket.Begin(CLIENT_CLOSE_PACKET);
	transPacket.WriteInt(uid);
	transPacket.WriteInt(TGlobal::_svid);
	transPacket.End();
	
	if((int)_helperpool->m_svidlist.size() > 0)
	{
		int svid = TGlobal::LoopSvid(_helperpool->m_svidlist);
		CHelperUnit *pHelperUnit = NULL;
		map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
		if(iterHelper != _helperpool->m_helpermap.end())
		{
			pHelperUnit = iterHelper->second;
			if(pHelperUnit != NULL)
			{
				pHelperUnit->append_pkg(transPacket.packet_buf(), transPacket.packet_size());
				pHelperUnit->send_to_logic(_helperTimer);
			}
		}
		else
		{
			g_pErrorLog->logMsg("%s||Cannot find helper, svid:[%d]", __FUNCTION__, svid);
		}
	}
}

void CHelperUnit::reset_helper(void)
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
}

CClientUnit* 
CHelperUnit::GetClientUintByUid(const int &uid, CDecoderUnit **pDecoder)
{
	CClientUnit* clt = NULL;
/* 	std::map<int, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.find(uid);
	if( iter != _helperpool->m_objmap.end())
	{
		(*pDecoder) = iter->second;
		if(*pDecoder != NULL)
		{
			clt = (*pDecoder)->get_web_unit();
		}
	} */
	return clt;
}

CClientUnit*
CHelperUnit::_get_client_by_id(const unsigned long& flow, CDecoderUnit** ppDecoder)
{
	CClientUnit *clt = NULL;

//	std::map<unsigned long, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.find(flow);
//	if( iter != _helperpool->m_objmap.end()) {
//		(*ppDecoder) = iter->second;
//		if(*ppDecoder != NULL) {
//			clt = (*ppDecoder)->get_web_unit();
//		}
//	}

	return clt;
}

int
CHelperUnit::_pay_res(NETInputPacket* pack)
{
	int 			ret = 0;
	int				code = 0;
	string 			json, result;
	Reader			r;
	Value			v;
	unsigned long 	flow;
	CClientUnit		*c;
	CDecoderUnit	*d;
	NETOutputPacket	out;
	CEncryptDecrypt	ed;

	log_debug("-------- CHelperUnit::_pay_res begin --------");
	code = pack->ReadByte();
	json = pack->ReadString();
	log_debug("ErrCode: %d", code);
	log_debug("JSON: %s", json.c_str());

	if (r.parse(json, v)) {
		flow = v["flow"].asUInt64();
		result = v["result"].asString();

		out.Begin(SERVER_CMD_REP);
		out.WriteInt(0);
		out.WriteString(result);
		out.End();
	} else { /* error handle */
		out.Begin(SERVER_CMD_REP);
		out.WriteInt(1);
		out.WriteString("");
		out.End();

		log_debug("-------- CHelperUnit::_pay_res error --------");
		ret = -1;
	}

	ed.EncryptBuffer(&out);  /* 缂栫爜 */

	c = _get_client_by_id(flow, &d);

	if (c && c->get_state() != CONN_FATAL_ERROR) {
		c->add_rsp_buf(out.packet_buf(), out.packet_size()); 
		ret = c->send();
	}

	log_debug("-------- CHelperUnit::_pay_res end --------");
	return ret;
}

int 
CHelperUnit::_worker_stat_chk(NETInputPacket* pack)
{
	unsigned short	cmd;
	int 			len;
	unsigned int	ctime;
	  
	log_debug("-------- CHelperUnit::_worker_stat_chk begin --------");
	
		  
	cmd = pack->GetCmdType();
	len = pack->ReadInt();
	ctime = pack->ReadInt();

	log_debug("Cmd: [%#x]", cmd);

	switch (cmd) {
		case INTER_CMD_WAITER_STAT_RES:
			gSvrSwitch->_w_is_busyed = len > SVR_MAX_W_QLEN ? true : false;
			gSvrStat->_w_qlen = len;
			gSvrStat->_w_ctime = ctime;
			break;
		case INTER_CMD_NOTIFY_STAT_RES:
			gSvrSwitch->_n_is_busyed = len > SVR_MAX_N_QLEN ? true : false;
			gSvrStat->_n_qlen = len;
			gSvrStat->_n_ctime = ctime;
			break;
		default:
			break;
	}

	log_info("-------- Server_Stat begin --------");
	log_info("_w_qlen:  %d", gSvrStat->_w_qlen);
	log_info("_w_ctime: %d", gSvrStat->_w_ctime);
	log_info("_n_qlen:  %d", gSvrStat->_n_qlen);
	log_info("_w_ctime: %d", gSvrStat->_n_ctime);
	log_info("_w_is_busyed: %s", gSvrSwitch->_w_is_busyed ? "true" : "false");
	log_info("_n_is_busyed: %s", gSvrSwitch->_n_is_busyed ? "true" : "false");
	log_info("-------- Server_Stat end --------");
	
	log_debug("-------- CHelperUnit::_worker_stat_chk end --------");
	return 0;
}

int
CHelperUnit::send_to_logic()
{
	EnableInput ();
	if (connect() < 0) 
	{
		reset_helper();
		log_debug("Connect logic server failed!");
		return -1;
	}
	return send_to_cgi();	
}

HTTP_SVR_NS_END

