#include "SyncFunc.h"
#include "RedisHelper.h"
#include "MysqlHelper.h"
#include "json/json.h"
#include "Log.h"

#include <string>

using std::string;
using namespace Helper;
using namespace Json;

static string
sync_get_table_name(const unsigned long& mtime)
{
	struct tm 	now;
	char		date[32];
	time_t		t;

	log_debug("mtime: %lu", mtime);

	if (mtime == 0) { // insert
		t = time(NULL);
		localtime_r(&t, &now); 
		snprintf(date, 32, "payadmin_order_%4d%02d", 
							now.tm_year + 1900, now.tm_mon ? now.tm_mon + 1 : 12);
	} else { // update
		localtime_r((const time_t *)&mtime, &now); 
		snprintf(date, 32, "payadmin_order_%4d%02d", 
							now.tm_year + 1900, now.tm_mon ? now.tm_mon + 1 : 12);
	}
	
	return string(date);
}

int
sync_order(CMysqlHelper* mysql, CRedisHelper* redis)
{
	string 			table;
	string 			sql;
	string 			data;
	unsigned long 	mtime;
	Value			json;
	Reader			reader;
	size_t			pos;
	int				urows;

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
	mtime = json["mtime"].asUInt64();

	table = sync_get_table_name(mtime);
	if (table.empty()) {
		log_error("get table failed.");
		// return -1;
	}
	log_debug("TABLE: %s", table.c_str());

	pos = sql.find("paycenter_order");
	if (pos != string::npos) {
		sql.replace(pos, strlen("paycenter_order"), table);
	}

	log_debug("SQL: %s", sql.c_str());

	urows = mysql->ExecuteNonQuery(sql);

	if (urows == -1) {
		log_error("DB Error: %d %s", mysql->GetErrNo(), mysql->GetErrMsg());
	}
	return 0;
}



