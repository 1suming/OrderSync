#include "watchdog.h"
#include "Json/json.h"
#include "clib_log.h"
#include "mempool.h"
#include "CHelper_pool.h"
#include "defs.h"
#include "client_unit.h"
#include "decode_unit.h"

#include <stdio.h>
#include <sys/un.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <memcheck.h>
#include <log.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <cache.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <helper_unit.h>
#include <memcheck.h>
#include <sstream>
#include <MarkupSTL.h>
#include <time.h>
#include <sys/time.h>
#include <string>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;

using namespace comm::sockcommu;
using namespace Json;
using std::string;

#define MAX_ERR_RSP_MSG_LEN 4096

extern CMemPool* _webMp;
extern Watchdog* LogFile;

HTTP_SVR_NS_BEGIN

extern CHelperPool*	_helperpool;

inline int GetAllocFromSid(short sid)
{
	return int(sid/200);
}

static const int RANDOM_COUNT = 100;
static int m_nRandCount = 0;

int GetRand()
{
	if(m_nRandCount > 1000000000)
		m_nRandCount = 0;
	srand((int)time(NULL)+m_nRandCount++);
	return rand()%RANDOM_COUNT;
}


CClientUnit::CClientUnit(CDecoderUnit* decoderunit, int fd, unsigned long flow): 
	CPollerObject (decoderunit->pollerunit(), fd),
	_api(0),
	_send_error_times(0),
	_stage (CONN_IDLE),
	_decodeStatus(DECODE_WAIT_HEAD),
	_decoderunit (decoderunit),
	_uid(0),
	_login_flag(0),
	_r(*_webMp),
	_w(*_webMp),
	_flow(flow)
{
}

CClientUnit::~CClientUnit (void)
{
	_w.skip(_w.data_len());
	_r.skip(_r.data_len());
}

int CClientUnit::Attach (void)
{
    EnableInput ();
    
	if (AttachPoller() == -1) // ???? netfd epoll
	{
        log_error ("invoke CPollerObject::AttachPoller() failed.");
		return -1;
	}
	_stage = CONN_IDLE;
    AttachTimer(_decoderunit->get_web_timer());
    return 0;
}

int CClientUnit::recv (void) // InputNotify
{    
    int ret = proc_pkg();
    g_pDebugLog->logMsg("recv ret [%d]", ret);
    switch (ret)
    {
        default:
        case DECODE_FATAL_ERROR:
            DisableInput ();
            g_pDebugLog->logMsg ("decode fatal error netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
            _stage = CONN_FATAL_ERROR;
            return -1;

        case DECODE_DATA_ERROR:
            DisableInput ();
            g_pDebugLog->logMsg ("decode error, netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
            _stage = CONN_DATA_ERROR;
            break;
		case DECODE_WAIT_HEAD:
		case DECODE_WAIT_CONTENT:
			_stage = CONN_DATA_RECVING;
			g_pDebugLog->logMsg ("recving data, netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
			break; 
        case DECODE_DONE:
            _stage = CONN_RECV_DONE;
            g_pDebugLog->logMsg ("decode done, netfd[%d], stage[%d]", netfd, _stage);
            break;     

        case DECODE_DISCONNECT_BY_USER:
            DisableInput ();
            g_pDebugLog->logMsg ("disconnect by user, netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
            _stage = CONN_DISCONNECT;
            break;
    }

	return 0;
}

int CClientUnit::send (void)
{
    log_error("CClientUnit response bufLen:[%d] netfd[%d]", _w.data_len(), netfd);	
	if (_w.data_len() != 0)
	{		
		log_error("send packet before, length[%d]", _w.data_len());
		int ret = ::send (netfd, _w.data(), _w.data_len(), 0);
		log_error("sent packet length[%d] netfd[%d]", ret, netfd);
		if(-1 == ret)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
			{
				log_error("errno:[%d]", errno);
				//¼ÓÈësend´íÎó´ÎÊýÏÞÖÆ
				_send_error_times++;
				if(_send_error_times >= 50)
				{
					g_pErrorLog->logMsg("%s|CClientUnit::send, send error more,%d|%d", __FUNCTION__, _uid, _api);
					DisableInput ();
					DisableOutput ();
					ApplyEvents ();
					_w.skip( _w.data_len() );
					_r.skip( _r.data_len() );
					_stage = CONN_FATAL_ERROR;
					return -1;
				}	
				EnableOutput ();
				ApplyEvents ();
				_stage = CONN_DATA_SENDING;
				return CONN_DATA_SENDING;
			}
		
			log_error ("sending package to client failed, ret[%d], errno[%d]",  ret, errno);	
			DisableInput ();
			DisableOutput ();
			ApplyEvents ();
			_w.skip( _w.data_len() );
			_r.skip( _r.data_len() );
			_stage = CONN_FATAL_ERROR;
			return -1;
		}

		if(ret == (int)_w.data_len())
		{
			log_debug("send complete");
			DisableOutput();
			ApplyEvents ();
			_w.skip(ret);	
			_stage = CONN_SEND_DONE;
			_send_error_times = 0;
			return ret;
		}
		else if (ret < (int)_w.data_len())
		{
			log_debug("had sent part of data");
			EnableOutput ();
			ApplyEvents ();
			_w.skip(ret);
			_stage = CONN_DATA_SENDING;
			_send_error_times = 0;
			return ret;
		}
	}

	DisableOutput();
	ApplyEvents ();	
	_stage = CONN_FATAL_ERROR;
	log_error("send process failure");
	return -1;
}

int CClientUnit::InputNotify (void)
{
    update_timer();
    recv();
     g_pDebugLog->logMsg("InputNotify");
    return _decoderunit->web_input ();
}

int CClientUnit::OutputNotify (void)
{
    update_timer();
    send();
    return _decoderunit->web_output();
}

int CClientUnit::HangupNotify (void)
{
    update_timer();
    log_error("*STEP: web connection is hangup, netfd[%d] stage[%d]", netfd, _stage);
    return _decoderunit->web_hangup();
}

void CClientUnit::TimerNotify(void)
{
    log_warning("*STEP: web client timer expired, fd[%d] timeout, stage[%d]", netfd, _stage);
	_decoderunit->web_timer();
    return;
}

void CClientUnit::update_timer(void)
{
    DisableTimer();	
    AttachTimer(_decoderunit->get_web_timer());
}

void CClientUnit::add_rsp_buf(const char* data, unsigned int len)
{
	_w.append(data, len);
}

TDecodeStatus CClientUnit::proc_pkg () // recv
{
	int     curr_recv_len   = 0;
	char    curr_recv_buf[MAX_WEB_RECV_LEN] = {'\0'};

	curr_recv_len = ::recv (netfd, curr_recv_buf, MAX_WEB_RECV_LEN, 0);
	g_pDebugLog->logMsg("*STEP: receiving HTTP request, length[%d]", curr_recv_len);
	if(-1 == curr_recv_len)
	{
		if(errno != EAGAIN && errno != EINTR && errno != EINPROGRESS)
		{
			_decodeStatus = DECODE_FATAL_ERROR;
			log_warning ("recv failed from fd[%d], msg[%s]", netfd, strerror(errno));
			return _decodeStatus;
		}
		else
		{
			return _decodeStatus;
		}
	}

	if(0 == curr_recv_len)
	{
		_decodeStatus = DECODE_DISCONNECT_BY_USER;
		g_pDebugLog->logMsg("%s||Connect disconnect by client, uid:[%d]", __FUNCTION__, _uid);
		return _decodeStatus;
	}

	if(curr_recv_len==23 && curr_recv_buf[0]=='<' && curr_recv_buf[1]=='p')		
	{	
		_decoderunit->set_conn_type(CONN_OTHER);
		std::string policy = "<policy-file-request/>";			
		for(int i=0; i<23; ++i)			
		{				
			if(curr_recv_buf[i] != policy[i])				
			{					
				_decodeStatus = DECODE_FATAL_ERROR;	
				return _decodeStatus;
			}
		}
		std::string resPolicy ="<cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>\0";
		_w.append(resPolicy.c_str(), resPolicy.size());
		send();	
		_decodeStatus = DECODE_FATAL_ERROR;					
		return _decodeStatus;
	}
	g_pDebugLog->logMsg("AAAAAAAAAAAAA");
	_r.append(curr_recv_buf, curr_recv_len);
	g_pDebugLog->logMsg("BBBBBBBBBBBBB");
	while(_r.data_len() > 0)
	{
		g_pDebugLog->logMsg("CCCCCCCCCCCC");
		int inputRet = HandleInput(_r.data(), _r.data_len()); /* 合法性检查 */
		g_pDebugLog->logMsg("DDDDDDDDDDDDDD  [%d]",inputRet);
		if(inputRet < 0) {
			_decodeStatus = DECODE_FATAL_ERROR;
			return _decodeStatus;
		} else if(inputRet == 0) {
			_decodeStatus = DECODE_WAIT_CONTENT;
			return _decodeStatus;
		}
		
		int handleInputBufRet = HandleInputBuf(_r.data() , inputRet);   
		g_pDebugLog->logMsg("eeeeeeeeeee  [%d]",handleInputBufRet);         
		if( handleInputBufRet < 0 )           
		{ 
			_decodeStatus = DECODE_FATAL_ERROR;                 
			return _decodeStatus;            
		}              
		_r.skip(inputRet);
	}
	_decodeStatus = DECODE_WAIT_HEAD;
	return _decodeStatus;
}

int 
CClientUnit::HandleInput(const char* data,  int len) /* 数据包合法性检查 */
{
	int 			headLen, pkglen;
	CEncryptDecrypt ed;
	
	headLen = sizeof(struct TPkgHeader);
	g_pDebugLog->logMsg("ffffffffffff  [%d]",headLen);  
	g_pDebugLog->logMsg("ggggggggg  [%d]",len);
	if(len < headLen) {
		g_pDebugLog->logMsg("hhhhhhh  [%d]",len);
		return 0;
	}
	
	TPkgHeader *pHeader = (struct TPkgHeader*)data;
	if(pHeader->flag[0]!='B' || pHeader->flag[1]!='Y') {
		g_pErrorLog->logMsg("%s||Invalid packet, uid:[%d]", __FUNCTION__, _uid);
		return -1;
	}
	
	pkglen = sizeof(short) + ntohs(pHeader->length);//×ª»»³É´ó¶Ë
	g_pErrorLog->logMsg("client packet body length:[%d], cmd:[%d]", ntohs(pHeader->length), ntohs(pHeader->cmd));
	if(pkglen < 0 || pkglen > 8*1000) {
		g_pErrorLog->logMsg("%s||Invalid packet, uid:[%d], pkglen:[%d]", __FUNCTION__, _uid, pkglen);
		return -1;
	}

	if(len < pkglen) return 0; /* 接收到数据包头数据，但是包还没有接受完整 */

	return pkglen;
}


bool CClientUnit::CheckCmd(int cmd)
{
	for(int i=0; i<(int)_helperpool->m_cmdlist.size(); i++)
	{
		if(cmd == _helperpool->m_cmdlist[i])
			return true;
	}
	return false;
}

int 
CClientUnit::HandleInputBuf(const char *pData, int len)
{
	NETInputPacket 	reqPacket;
	int 			cmd = 0, ret = 0, decode = 0;
	string 			ReqMsg;
	CEncryptDecrypt	ed;

	reqPacket.Copy(pData, len);
	decode = ed.DecryptBuffer(&reqPacket);

	if (decode == -1) {
		log_error("decode failed.");
		return -1;
	}

	cmd = reqPacket.GetCmdType();

	g_pErrorLog->logMsg("%s||HandleInputBuf cmd:[0x%x]",__FUNCTION__, cmd);
	
	/* 协议命令处理 */
	switch (cmd) {
		case CLIENT_CMD_DATA_REQ:
			ret = cmd_data_handler(&reqPacket);
			break;
		default:
			ret = -1; // 命令字不合法
			g_pErrorLog->logMsg("cmd %d is invailed.", cmd);
			break;
	}

	return ret; /* 结束 */
}


int CClientUnit::ResetHelperUnit()
{	
	CMarkupSTL  markup;
    if(!markup.Load("../conf/server.xml"))
    {       
        log_error("Load server.xml failed.");
        return -1;
    }

    if(!markup.FindElem("SYSTEM"))
    {
        log_error("Can not FindElem [SYSTEM] in server.xml failed.");
        return -1;
    }

	if (!markup.IntoElem())    
	{        
		log_error ("IntoElem [SYSTEM] failed.");       
		return -1;    
	}
	
	if(markup.FindElem("Node"))
	{
		map<int, vector<int> >::iterator iterLevel = _helperpool->m_levelmap.begin();
		for(; iterLevel!=_helperpool->m_levelmap.end(); iterLevel++)
		{
			vector<int>& v = iterLevel->second;
			v.clear();
		}
		_helperpool->m_levelmap.clear();
		
		_helperpool->m_svidlist.clear();

		map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.begin();
		for(; iterHelper!=_helperpool->m_helpermap.end(); iterHelper++)
		{
			CHelperUnit* pHelperUnit = iterHelper->second;
			if(pHelperUnit != NULL)
			{
				delete pHelperUnit;
				pHelperUnit = NULL;
			}
		}

		_helperpool->m_helpermap.clear();
			
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem failed.");       
			return -1;    
		}

		if(!markup.FindElem("ServerList"))
		{
			log_error ("IntoElem [ServerList] failed.");     
			return -1; 
		}
		
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem [ServerList] failed.");       
			return -1;    
		}
		
		while(markup.FindElem("Server"))
		{
			int svid =  atoi(markup.GetAttrib("svid").c_str());
			int level = atoi(markup.GetAttrib("level").c_str());
			string ip = markup.GetAttrib("ip");
			int port = atoi(markup.GetAttrib("port").c_str());
			CHelperUnit *pHelperUnit = NULL;
			map<int, CHelperUnit*>::iterator iter = _helperpool->m_helpermap.find(svid);
			if(iter != _helperpool->m_helpermap.end())
			{
				pHelperUnit = iter->second;
			}
			else
			{
				pHelperUnit = new CHelperUnit(_decoderunit->pollerunit());
				if(pHelperUnit == NULL)
				{
					log_error("New CHelpUnit error");
					return -1;
				}	
				_helperpool->m_helpermap[svid] = pHelperUnit;
			}
			 
			if(pHelperUnit == NULL)
			{
				log_boot("pHelperUnit NULL");
				return -1;
			}	


			_helperpool->m_svidlist.push_back(svid);

			pHelperUnit->addr = ip;
			pHelperUnit->port = port;

			vector<int>& v = _helperpool->m_levelmap[level];
			v.push_back(svid);
			log_error("alloc server id:[%d], level:[%d], ip:[%s], port:[%d]", svid, level, ip.c_str(), port);
		}


		if (!markup.OutOfElem())    
		{        
			log_error ("OutOfElem [CmdList] failed.");       
			return -1;    
		}
	
		if(!markup.FindElem("CmdList"))
		{
			log_error("Can not FindElem [CmdList] in server.xml failed.");
			return -1;
		}
		
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem [CmdList] failed.");       
			return -1;    
		}

		_helperpool->m_cmdlist.clear();
		while(markup.FindElem("Cmd"))
		{
			int cmd = atoi(markup.GetAttrib("value").c_str());
			log_error("cmd:[%d]", cmd);
			_helperpool->m_cmdlist.push_back(cmd);
		}
		
		if (!markup.OutOfElem())    
		{        
			log_error ("OutOfElem [ServerList] failed.");       
			return -1;    
		}
	}
	return 0;
}

int CClientUnit::ResetIpMap()
{
	CMarkupSTL  markup;
    if(!markup.Load("../conf/server.xml"))
    {       
        log_error("Load server.xml failed.");
        return -1;
    }

    if(!markup.FindElem("SYSTEM"))
    {
        log_error("Can not FindElem [SYSTEM] in server.xml failed.");
        return -1;
    }

	if (!markup.IntoElem())    
	{        
		log_error ("IntoElem [SYSTEM] failed.");       
		return -1;    
	}
	
	if(markup.FindElem("IPMap"))
	{
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem [IPMap] failed.");       
			return -1;    
		}

		while(markup.FindElem("IP"))
		{
			string eth0 = markup.GetAttrib("eth0");
			string eth1 = markup.GetAttrib("eth1");
			log_error("%s||eth0:[%s], eth1:[%s]", __FUNCTION__, eth0.c_str(), eth1.c_str());
			_helperpool->m_ipmap[eth0] = eth1;
		}
	}
	return 0;
}

CHelperUnit* CClientUnit::GetRandomHelper(short level)
{
	CHelperUnit* pHelperUnit = NULL;
	map<int, vector<int> >::iterator iter = _helperpool->m_levelmap.find(level);
	if(iter != _helperpool->m_levelmap.end())
	{
		vector<int>& v = iter->second;
		if((int)v.size() > 0)
		{
			int svid = TGlobal::RandomSvid(v);
			map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
			if(iterHelper != _helperpool->m_helpermap.end())
			{
				pHelperUnit = iterHelper->second;	
			}
			else
			{
				g_pErrorLog->logMsg("%s||Can not find helper, uid:[%d], api:[%d], svid:[%d]", 
					__FUNCTION__, _uid, _api, svid); //找不到alloc helper
			}
		}
	}
	return pHelperUnit;
}

int CClientUnit::SendPacketToHelperUnit(CHelperUnit* pHelperUnit, char *pData, int nSize)
{
	NETOutputPacket transPacket;
	transPacket.Begin(CLIENT_PACKET2);
	transPacket.WriteInt(_uid);
	transPacket.WriteInt(TGlobal::_svid);
	transPacket.WriteInt(_decoderunit->get_ip());
	transPacket.WriteShort(_api);
	transPacket.WriteBinary(pData, nSize);
	transPacket.End();

	pHelperUnit->append_pkg(transPacket.packet_buf(), transPacket.packet_size());
	if(pHelperUnit->send_to_logic(_decoderunit->get_helper_timer()) < 0) {
		g_pErrorLog->logMsg("%s||Send to AllocServer failed, ip:[%s], port:[%d], uid:[%d], api:[%d]",
			__FUNCTION__, pHelperUnit->addr.c_str(), pHelperUnit->port, _uid, _api);
		return -1;
	}
	return 0;
}

void CClientUnit::SendIPSetPacket(CGameUnit* pGameUnit, NETInputPacket &reqPacket, int cmd)
{
	if(NULL!=_decoderunit && CLIENT_COMMAND_LOGIN==cmd)
	{
		CEncryptDecrypt encryptdecrypt;
		encryptdecrypt.DecryptBuffer(&reqPacket);
		int nTableId = reqPacket.ReadInt();
		int nUserId = reqPacket.ReadInt();

		g_pDebugLog->logMsg("nTableId [%d], nUserId [%d]", nTableId, nUserId);

		NETOutputPacket outPkg;
		outPkg.Begin(SERVER_CMD_SET_IP);
		outPkg.WriteInt(nTableId);
		outPkg.WriteInt(nUserId); 
		outPkg.WriteInt(ntohl(_decoderunit->get_ip())); 
		outPkg.End();
		encryptdecrypt.EncryptBuffer(&outPkg);
 		pGameUnit->append_pkg(outPkg.packet_buf(), outPkg.packet_size());
	}
}	

int CClientUnit::ProcessOpenDebug(NETInputPacket *pPacket)
{	
	CEncryptDecrypt encryptdecrypt;
	encryptdecrypt.DecryptBuffer(pPacket);
	string strFlag = pPacket->ReadString();
	if(strFlag != "!@#$%^&*()")//¼òµ¥ÑéÖ¤ÏÂ
		return -1;
	TGlobal::_debugLogSwitch = pPacket->ReadInt();
	log_error("%s|_debugLogSwitch:[%d]", __FUNCTION__, TGlobal::_debugLogSwitch);
	return 0;
}

int
CClientUnit::client_cmd_req_handler(NETInputPacket* pack)
{
	int 			ret = 0, ReqType = 0;
	string 			ReqMsg, ReqJson;
	NETOutputPacket out;
	NETInputPacket 	*in;
	Reader			reader;
	Value			value;
	FastWriter		writer;
	CHelperUnit*	h;
	CEncryptDecrypt	ed;	

	in = pack;
	ed.DecryptBuffer(in); /* 解码 */

	ReqType = in->ReadInt();
	ReqMsg = in->ReadString(); /* json format */

	/* parse json and add flow feild */
	if (reader.parse(ReqMsg, value)) {
		value["flow"] = (UInt64)this->_flow;
		value["time"] = (unsigned int)time(NULL);

		ReqJson = writer.write(value);

		out.Begin(INTER_CMD_REQ);
		out.WriteInt(ReqType);
		out.WriteString(ReqJson);
		out.End();

		ed.EncryptBuffer(&out); /* 编码 */

		/* 获取后台helper，并发送 */
		h = this->_get_job_worker();

		if (h) {
			h->append_pkg(out.packet_buf(), out.packet_size());
			ret = h->send_to_logic(_decoderunit->get_helper_timer());
		} else {
			g_pErrorLog->logMsg("JobWorker Not Found.");
			return -1;
		}
	} else {
		g_pErrorLog->logMsg("ReqMsg Parsed Failed.");
		ret = -1;
	}

	return ret;
}

CHelperUnit*
CClientUnit::_get_job_worker()
{
	map<int, vector<int> >::iterator 	l_iter;
	map<int, CHelperUnit*>::iterator 	h_iter;
	int									job_id;
	CHelperUnit							*h = NULL;
	
	l_iter = _helperpool->m_levelmap.find(JobWorkerType);

	if (l_iter != _helperpool->m_levelmap.end()) {
		vector<int>& workers = l_iter->second;

		if (workers.size() > 0) {
			job_id = workers[0]; /* 暂时只有一个worker这里直接赋值 */

			h_iter = _helperpool->m_helpermap.find(job_id);

			if (h_iter != _helperpool->m_helpermap.end()) {
				h = h_iter->second;
			} else {
				g_pErrorLog->logMsg("helper null.");
			}
		}
	}

	return h;
}

int
CClientUnit::cmd_login_handler(int id)
{
	log_debug("-------- CClientUnit::cmd_login_handle begin --------");

	_helperpool->m_objmap[id] = _decoderunit;

	log_debug("-------- CClientUnit::cmd_login_handle end --------");

	return 0;
}

int
CClientUnit::cmd_data_handler(NETInputPacket* pack)
{
	string 			data;
	uint64_t 		event_id;
	NETOutputPacket out;
	int				ret;
	CEncryptDecrypt ed;
	int				client_id;

	log_debug("-------- CClientUnit::cmd_data_handler begin --------");
	client_id = pack->ReadInt();
	event_id = pack->ReadULong();
	data = pack->ReadString();

	cmd_login_handler(client_id);

	log_debug("event_id: %"PRIu64, event_id);
	log_debug("DATA: %s", data.c_str());

	__data_flow(data);
	
	out.Begin(SERVER_CMD_DATA_REP);
	out.WriteULong(event_id);
	out.End();
	ed.EncryptBuffer(&out);

	this->add_rsp_buf(out.packet_buf(), out.packet_size());
	ret = this->send();

	log_debug("-------- CClientUnit::cmd_data_handler end --------");

	return ret;
}

int
CClientUnit::__data_flow(const string& data)
{
	Value 								value;
	Reader 								reader;
	time_t								mtime;
	uint64_t 							oid;
	string 								sql;
	int 								rsz;
	int									sid;
	map<int, redis_helper_t*>::iterator itr;
	redis_helper_t						*redis;

	log_debug("-------- CClientUnit::__data_flow begin --------");
	if (!reader.parse(data, value)) {
		log_error("data[%s] parse failed.", data.c_str());
		return -1;
	}

	oid = value["id"].asUInt64();
	mtime = value["mtime"].asUInt64();
	sql = value["sql"].asString();

	rsz = _helperpool->m_redis_map.size();

	sid = oid % rsz;
	
	if (sid == 0) sid = rsz;

	log_debug("sid: %d", sid);

	itr = _helperpool->m_redis_map.find(sid);
	if (itr != _helperpool->m_redis_map.end()) {
		redis = itr->second;
	} else { /* 未找到 redis */
		log_error("order[%"PRIu64"] not found redis.", oid);
		return -1;
	}

	redis->Enqueue("ORDER_Q", data);

	log_debug("-------- CClientUnit::__data_flow end --------");
	return 0;
}

HTTP_SVR_NS_END

