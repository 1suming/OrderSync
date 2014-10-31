#ifndef __SYNC_FUNC_H_
#define __SYNC_FUNC_H_

#include "ConfigDef.h"


namespace Helper 
{
class CRedisHelper;
class CMysqlHelper;
}


using namespace Helper;

int sync_game_log(CRedisHelper* rh, CMysqlHelper* mysql);
int sync_user_bankruptcy_log(CRedisHelper* rh, CMysqlHelper* mysql);
int sync_user_score_log(CRedisHelper* rh, CMysqlHelper* mysql);
int sync_user_money_log(CRedisHelper* rh, CMysqlHelper* mysql);

int sync_new_table(CMysqlHelper* mysql);

extern pEQueueConf queue_conf;
extern EMysqlConf mysql_conf;
extern ERedisConf redis_conf;
extern EServerConf server_conf;
extern char Today[9];

#endif
