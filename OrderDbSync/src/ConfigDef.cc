#include "IniFile.h"
#include "ConfigDef.h"
#include "Log.h"

#include <stdio.h>
#include <string.h>

int
Load_Mysql_Conf(const char * file, pEMysqlConf pConf)
{
	read_profile_string("mysql", "host", pConf->host, sizeof(pConf->host), "", file);
	pConf->port = read_profile_int("mysql", "port", 0, file);
	read_profile_string("mysql", "db", pConf->db, sizeof(pConf->db), "", file);
	read_profile_string("mysql", "user", pConf->user, sizeof(pConf->user), "", file);
	read_profile_string("mysql", "passwd", pConf->passwd, sizeof(pConf->passwd), "", file);

	return 0;
}

int
Load_Redis_Conf(const char * file, pERedisConf pConf)
{
	read_profile_string("redis", "host", pConf->host, sizeof(pConf->host), "", file);
	pConf->port = read_profile_int("redis", "port", 0, file);
	pConf->timeout = read_profile_int("redis", "timeout", 0, file);

	return 0;
}

int
Load_Queue_Conf(const char* file, pEQueueConf pConf)
{
	char queue_name[50];

	for (int i = 1; i <= pConf->nums; i++)
	{
		snprintf(queue_name, sizeof(queue_name), "%s%d", "queue", i);
	
		read_profile_string("queue", queue_name, *(pConf->names + i - 1), 50, "", file);

		memset(queue_name, 0, sizeof(queue_name));
	}
	
	return 0;
}

int
Load_Server_Conf(const char* file, pEServerConf pConf)
{
	read_profile_string("server", "version", pConf->version, sizeof(pConf->version), "", file);
	pConf->sleep_msec = read_profile_int("server", "delay", 0, file);

	return 0;
}

int
load_table_conf(const char* file, table_conf_t* ptable)
{

}
