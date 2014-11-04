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
sync_get_table_name(const unsigned long& time)
{
	struct tm 	now;
	char		date[32];

	localtime_r((const time_t*)&time, &now);

	snprintf(date, 32, "payadmin_order_%4d%02d", now.tm_year, now.tm_mon);
	
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

	if (!redis->IsActived()) {
		redis->Connect();
		if (!redis->IsActived()) {
			log_error("redis can't connect.");
			return -1;
		}
	} else {
		redis->Enqueue("ORDER_Q", data);
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
		return -1;
	}

	pos = sql.find("paycenter_order");
	if (pos != string::npos) {
		sql.replace(pos, table.size(), table);
	}
	mysql->ExecuteQuery(sql);

	return 0;
}



