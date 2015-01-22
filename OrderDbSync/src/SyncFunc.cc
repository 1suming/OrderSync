#include "SyncFunc.h"
#include "RedisHelper.h"
#include "MysqlHelper.h"
#include "json/json.h"
#include "Log.h"
#include "ConfigDef.h"

#include <string>

using std::string;
using namespace Helper;
using namespace Json;

extern table_conf_t table_conf;
extern EMysqlConf 	mysql_conf;

static string
sync_get_table_name(const long& mtime)
{
	struct tm 	now;
	char		date[128];
	time_t		t;

	log_debug("mtime: %lu", mtime);

	if (mtime == 0) { // insert
		t = time(NULL);
		localtime_r(&t, &now); 
		snprintf(date, sizeof date, table_conf.new_name, 
							now.tm_year + 1900, now.tm_mon + 1);
	} else { // update
		localtime_r((const time_t *)&mtime, &now); 
		snprintf(date, sizeof date, table_conf.new_name, 
							now.tm_year + 1900, now.tm_mon + 1;
	}
	
	return string(date);
}

static int
__repair_sql(string& sql, uint64_t pid)
{
	size_t 	pos, pos2;
	char 	temp[64];
	char	*dm = ")";

	pos = sql.find(dm);
	if (pos != string::npos) {
		sql.replace(pos, strlen(dm), ",pid)");
	} else {
		return -1;
	}

	pos = sql.find(dm);
	if (pos != string::npos) {
		pos2 = sql.find(dm, pos + 1);
		if (pos2 != string::npos) {
			snprintf(temp, sizeof temp, ",%lu)", pid);
			sql.replace(pos2, strlen(dm), temp);
		} else {
			return -1;
		}
	} else {
		return -1;
	}

	return 0;
}

int
sync_order(CMysqlHelper* mysql, CRedisHelper* redis)
{
	string 			table;
	string 			sql;
	string 			data;
	long 			mtime;
	Value			json;
	Reader			reader;
	size_t			pos;
	int				urows;
	int				type;
	uint64_t		id;

	if (!redis->IsActived()) {
		redis->Connect();
		if (!redis->IsActived()) {
			log_error("redis can't connect.");
			return -1;
		}
	} else {
		redis->Dequeue("ORDER_Q", data);
	}
	
	if (!data.empty()) {
		log_debug("DATA: %s", data.c_str()); 
	} else {
		usleep(100000);
		return 0;
	}

	if (!reader.parse(data, json)) {
		log_error("json parse failed.");
		return -1;
	}

	sql = json["sql"].asString();
	type = json["type"].asInt();
	mtime = type == 0 ? json["stime"].asInt64() : json["mtime"].asInt64();

	table = sync_get_table_name(mtime);
	if (table.empty()) {
		log_error("get table failed.");
		return -1;
	}
	log_debug("TABLE: %s", table.c_str());

	pos = sql.find(table_conf.old_name);
	if (pos != string::npos) {
#ifdef __PARTITION__
#warning("__PARTITION__ defined")
		sql.replace(pos, 19, table); // 分表处理方式 paycenter_order_0001 固定长度为20
#else 
#warning("__PARTITION__ not defined")
		sql.replace(pos, strlen(table_conf.old_name), table); //未分表处理方式
#endif
	}
#ifndef __PARTITION__
	if (type == 0) { // insert
		id = json["id"].asUInt64();
		__repair_sql(sql, id); // add pid 
	}
#endif
	log_debug("SQL: %s", sql.c_str());

	if (!mysql->IsConnected()) {
		mysql->Connect();
		mysql->UseDB(mysql_conf.db);
	} 

	urows = mysql->ExecuteNonQuery(sql); 
	if (urows == -1) {
		log_error("DB Error: %d %s", mysql->GetErrNo(), mysql->GetErrMsg());
	}

	return 0;
}



