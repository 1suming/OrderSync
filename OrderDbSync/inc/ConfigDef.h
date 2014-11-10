#ifndef __CONFIG_DEF_H_
#define __CONFIG_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char 			host[16];
	unsigned short 	port;
	char			db[50];
	char			user[50];
	char			passwd[50];
} EMysqlConf, *pEMysqlConf;

typedef struct {
	char			host[16];
	unsigned short 	port;
	unsigned short 	timeout;
} ERedisConf, *pERedisConf;

typedef struct {
	unsigned short	nums;
	char**			names;
} EQueueConf, *pEQueueConf;

typedef struct {
	char 			version[10];
	unsigned int	sleep_msec; 
} EServerConf, *pEServerConf;

typedef struct table_conf_s table_conf_t;
struct table_conf_s {
	char old_name[128];
	char new_name[128];
};

int
Load_Mysql_Conf(const char* file, pEMysqlConf pConf);

int
Load_Redis_Conf(const char* file, pERedisConf pConf);

int
Load_Queue_Conf(const char* file, pEQueueConf pConf);

int
Load_Server_Conf(const char* file, pEServerConf pConf);

int
load_table_conf(const char* file, table_conf_t* ptable);


#ifdef __cplusplus
}
#endif
	
#endif

