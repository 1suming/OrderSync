#include "global.h"
#include "incoming.h"
#include "log.h"
#include "proxy_module.h"
#include "cache.h"
#include "CHelper_pool.h"
#include "clib_log.h"
#include "watchdog.h"
#include "RealTimer.h"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

using namespace comm::sockcommu;

MODULE_INSTANCE_EXPORT(__http_svr_ns__::CProxyModule, qzhttp_proxy, 2_0_0)
MODULE_DESTROY_EXPORT(__http_svr_ns__::CProxyModule, qzhttp_proxy, 2_0_0)

CMemPool* _webMp;
CMemPool* _helpMp;

clib_log* g_pErrorLog;
clib_log* g_pDebugLog;

Watchdog *LogFile = NULL;


HTTP_SVR_NS_BEGIN

CHelperPool* 	_helperpool;
CRealTimer* 	_RealTimer;//定时上报定时
server_stat_t	*gSvrStat;
server_switch_t	*gSvrSwitch;

CLevelCountTimer*	_LevelCountTimer;	//获取场次人数的定时器

CProxyModule::CProxyModule (void)
{
}

CProxyModule::~CProxyModule (void)
{
}

int CProxyModule::open (int argc, char** argv)
{
	 TGlobal::DaemonStart();
	_webMp = new CMemPool();
	_helpMp = new CMemPool(); 
	_helperpool = new CHelperPool();

	_RealTimer = new CRealTimer();
	assert(_RealTimer != NULL);

	//_LevelCountTimer = new CLevelCountTimer();
	//assert(_LevelCountTimer != NULL);

	gSvrStat = new server_stat_t;
	assert(gSvrStat != NULL);

	gSvrSwitch = new server_switch_t;
	assert(gSvrSwitch != NULL);

    if(TGlobal::DaemonInit("../conf/config.xml") < 0)
		return -1;
	if((string)argv[1]!="-p" || (string)argv[3]!="-s" || (string)argv[5]!="-h")
	{
		log_boot("param format error %s,%s,%s", argv[1], argv[3], argv[5]);
		return -1;
	}
	TGlobal::_port = atoi(argv[2]);
	TGlobal::_svid = atoi(argv[4]);
	string strHallIp = argv[6];

	
	TGlobal::_ip = strHallIp;

	if(TGlobal::_udpSwitch)
	{
		LogFile = new Watchdog(TGlobal::_udpIp, TGlobal::_udpPort);
		assert(LogFile!=NULL);
		LogFile->init();
	}
	
	m_pRedisServer = new CRedisServer();
	if(m_pRedisServer == NULL)
	{
		return -1;
	}

	m_pRedisServer->Connect(TGlobal::_redisIp, TGlobal::_redisPort);
	m_pRedisServer->SetInfo(strHallIp, TGlobal::_port, TGlobal::_svid);

	char szDebugName[100] = {0};
	snprintf(szDebugName, sizeof(szDebugName), "HallServer.debug.%u", getpid());
	g_pDebugLog = new clib_log();    
	g_pDebugLog->set_file(szDebugName);    
	g_pDebugLog->set_level(5);    
	g_pDebugLog->set_maxfile(5);    
	g_pDebugLog->set_maxsize(10240000);    
	g_pDebugLog->set_timeformat(CLIB_LOG_TFORMAT_0);

	char szErrorName[100] = {0};
	snprintf(szErrorName, sizeof(szErrorName), "HallServer.error.%u", getpid());
	g_pErrorLog = new clib_log();    
	g_pErrorLog->set_file(szErrorName); 
	g_pErrorLog->set_level(5);    
	g_pErrorLog->set_maxfile(5);    
	g_pErrorLog->set_maxsize(10240000);    
	g_pErrorLog->set_timeformat(CLIB_LOG_TFORMAT_0);
	
	if(TGlobal::_port<=0 || TGlobal::_svid<=0)
	{
		log_boot("param value error");
		return -1;
	}
    if(CIncoming::Instance()->open() < 0)
    {
        log_boot("Init incoming instance failed");
        return -1;
    }
    return 0;
}

int CProxyModule::close (void)
{
    CIncoming::Destroy ();
    log_boot ("%s v%s: stopped", TGlobal::_progname, TGlobal::_version);
    return 0;
}

int CProxyModule::run (void* parameter)
{
    TGlobal::_module_close = (int*)(parameter);
    //process network event
    CIncoming::Instance()->run ();
    return 0;
}

int CProxyModule::exception (void)
{
    return 0;
}


HTTP_SVR_NS_END
	
using namespace __http_svr_ns__;


int main (int argc, char** argv)
{
	CProxyModule cpm;
	int iRet = cpm.open(argc, argv);
	
	if (iRet != 0)
	{
        log_boot("CProxyModule open failed. iRet : [%d]", iRet);
		return 0;
	}
	
    log_boot("Proxy open is done!!");
	cpm.run((void*)&(TGlobal::_stopped));
}

