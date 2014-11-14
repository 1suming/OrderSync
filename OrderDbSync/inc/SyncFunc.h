#ifndef __SYNC_FUNC_H_
#define __SYNC_FUNC_H_

#include "ConfigDef.h"

namespace Helper 
{
class CRedisHelper;
class CMysqlHelper;
}

using Helper::CRedisHelper;
using Helper::CMysqlHelper;

int
sync_order(CMysqlHelper* mysql, CRedisHelper* redis);


#endif
