#ifndef _HTTP_GLOBAL_H_
#define _HTTP_GLOBAL_H_

#include <malloc.h>
#include <unistd.h>
#include <string>
#include <stdint.h>
#include <defs.h>
#include <zlib.h>

#include <map>
using std::map;
#include <vector>
using std::vector;

using std::string;

HTTP_SVR_NS_BEGIN

class TGlobal
{
public: //method
    static void ShowVersion(void);
    static void ShowUsage(void);
    static int DaemonInit(const char* conf_file);
    static void DaemonStart(void);
	static int split_str(const char* ps_str , char* ps_sp , vector<std::string> &v_ret);
	static string binary2char(char c);
	static int RandomSvid(vector<int>& v);
	static int LoopSvid(vector<int>& v);
	static bool InWhiteList(vector<int>& v, int value);
public: //property
    static volatile int*    _module_close;
    static volatile int     _stopped;
    static int              _accesslog;
    static const char*      _progname;
    static const char*      _version;
    static std::string      _config_file;
public:
    static char            _addr[64];  
    static uint16_t        _port;  
    static int             _backlog;
    static int             _maxpoller;
    static int             _webtimeout;
    static int             _helpertimeout;
    static std::string     _key;
	static int             _svid;
	static std::string     _redisIp;
	static int             _redisPort;
	static int             _randCount;
	static int             _udpSwitch;
	static std::string	   _udpIp;	
	static int 			   _udpPort;
	static std::string	   _ip;

	static int			   _timerConnect;//Connect ³¬Ê±Ê±¼ä
	static int             _loopNum;

	static int             _debugLogSwitch;
		
private: //no impl
    TGlobal (void);
    ~TGlobal (void);
    TGlobal (const TGlobal&);
    const TGlobal& operator= (const TGlobal&);
};

HTTP_SVR_NS_END

#endif // #define _HTTP_GLOBAL_H_

