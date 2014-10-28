#include "order_sync_client.h"
#include "fetcher.h"
#include "IniFile.h"
#include "log.h"
#include "RedisHelper.h"
#include "TcpClient.h"
#include "MysqlHelper.h"
#include "conf.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

bool 	is_daemon = true;
conf_t 	g_conf; 

static void
_dump_conf()
{
	log_debug("[server_ip]: %.15s", g_conf.server_ip);
	log_debug("[server_port]: %d", g_conf.server_port);
	log_debug("[redis_ip]: %.15s", g_conf.redis_ip);
	log_debug("[redis_port]: %d", g_conf.redis_port);
	log_debug("[redis_key]: %.63s", g_conf.redis_key);
	log_debug("[mysql_ip]: %.15s", g_conf.mysql_ip);
	log_debug("[mysql_port]: %d", g_conf.mysql_port);
	log_debug("[mysql_db]: %.63s", g_conf.mysql_db);
	log_debug("[mysql_username]: %.63s", g_conf.mysql_username);
	log_debug("[mysql_password]: %.63s", g_conf.mysql_password);
}

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
	read_profile_string("mysql", "username", g_conf.mysql_username, sizeof g_conf.mysql_username, "", f);
	read_profile_string("mysql", "password", g_conf.mysql_password, sizeof g_conf.mysql_password, "", f);

	_dump_conf();
}

int 
main(int argc, char** argv)
{
	order_sync_client_t *instance;
	fetcher_t			*f;
	tcp_client_t		*c;
	redis_helper_t		*r;
	mysql_helper_t		*m;
	char 				log_prefix[64];
	pid_t				pid;

   	pid = getpid();
	snprintf(log_prefix, sizeof(log_prefix), "Log_%d", pid); 
    init_log(log_prefix, "./");

	_init();

#if 1

	r = new redis_helper_t(g_conf.redis_ip, g_conf.redis_port);
	if (r == NULL) {
		log_error("new reids failed.");
		return -1;
	}

	f = new fetcher_t(r); 
	if (f == NULL) {
		log_error("new fetcher failed.");
		return -1;
	}

	c = new tcp_client_t(g_conf.server_ip, g_conf.server_port);
	if (c == NULL) {
		log_error("new TcpClient failed.");
		return -1;
	} 
	if (c->Connect()) {
		log_error("connect server failed.");
		return -1;
	}


	m = new mysql_helper_t(g_conf.mysql_ip, 
						   g_conf.mysql_port, 
		                   g_conf.mysql_username, 
		                   g_conf.mysql_password);
	if (m == NULL) {
		log_error("new mysql failed.");
		return -1;
	}
	if (m->Connect()) {
		log_error("connect mysql failed.");
		return -1;
	}
	m->UseDB(g_conf.mysql_db);


	instance = new order_sync_client_t(f, c, m);
	if (instance) {
		instance->run();
	} else {
		log_error("new instance failed.");
		return -1;
	}
#endif

	return 0;
}

