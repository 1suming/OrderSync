#include "order_sync_client.h"
#include "fetcher.h"
#include "IniFile.h"

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
};

bool 	is_daemon = true;
conf_t 	g_conf; 

static void
_init()
{
	char *f;

	f = "etc/server.ini";

	read_profile_string("server", "ip", g_conf.server_ip, sizeof g_conf.server_ip, "", f);
	g_conf.server_port = read_profile_int("server", "port", 0, f);

	read_profile_string("redis", "ip", g_conf.redis_ip, sizeof g_conf.redis_ip, "", f);
	g_conf.redis_port = read_profile_int("redis", "port", 0, f);
	read_profile_string("redis", "key", g_conf.redis_key, sizeof g_conf.redis_key, "", f);

	read_profile_string("mysql", "ip", g_conf.mysql_ip, sizeof g_conf.mysql_ip, "", f);
	g_conf.mysql_port = read_profile_int("mysql", "port", 0, f);
	read_profile_string("mysql", "db", g_conf.mysql_db, sizeof g_conf.mysql_db, "", f);
	read_profile_string("mysql", "user", g_conf.mysql_username, sizeof g_conf.mysql_username, "", f);
	read_profile_string("mysql", "password", g_conf.mysql_password, sizeof g_conf.mysql_password, "", f);
}

int 
main(int argc, char** argv)
{
	order_sync_client_t *instance;
	fetcher_t			*f;
	tcp_client_t		*c;




	instance = new order_sync_client_t(f, c);

	if (instance) {
		instance->run();
	}

	return 0;
}

