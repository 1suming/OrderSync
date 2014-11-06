#ifndef __CONF_H_
#define __CONF_H_

typedef struct conf_s conf_t;
struct conf_s {
	char 			server_ip[16];
	unsigned short 	server_port;
	char 			redis_ip[16];
	unsigned short 	redis_port;
	char 			redis_key[64];
	char 			mysql_ip[16];
	unsigned short 	mysql_port;
	char 			mysql_username[64];
	char 			mysql_password[64];
	char 			mysql_db[64];
	int				client_id;
};

#endif


