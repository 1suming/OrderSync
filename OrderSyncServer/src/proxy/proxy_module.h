#ifndef __QZ_PROXY_MODULE_H__
#define __QZ_PROXY_MODULE_H__

#include <global.h>
#include <noncopyable.h>
#include <module.h>
#include "RedisServer.h"
class CIncoming;
HTTP_SVR_NS_BEGIN
class CProxyModule : public IModule, private noncopyable
{
public:
    CProxyModule (void);
    virtual ~CProxyModule (void);
    virtual int open (int argc, char** argv);
    virtual int close (void);
    virtual int run (void* parameter);
    virtual int exception (void);

	CRedisServer* m_pRedisServer;
};

HTTP_SVR_NS_END
#endif

