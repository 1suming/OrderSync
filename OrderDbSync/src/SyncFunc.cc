#include "SyncFunc.h"
#include "RedisHelper.h"
#include "MysqlHelper.h"
#include "json/json.h"
#include "Log.h"

#include <iostream>
#include <stdio.h>

char Today[9] = {0};

//format: yyyymmdd
static string 
GetDate()
{
	struct tm* ptr;
	time_t t;

	time(&t);

	ptr = localtime(&t);

	char buff[9] = {0};
	
	strftime(buff, 9, "%Y%m%d", ptr);

	return buff; 
}


typedef struct {
	unsigned int 	tid;
	unsigned int	basechip;
	unsigned int 	time;
	short 			type;
	char 			log[8000];	
	char			uuid[37];
	unsigned short	clear_times;
} EGameLog, *pEGameLog;

typedef struct
{
	unsigned int 	mid;
	unsigned int 	tid;
	int 			basechip;
	int 			amount;
	unsigned int 	time;
}EUserBankruptcy, *pEUserBankruptcy;

typedef struct
{
	unsigned int 	mid;
	unsigned int 	king;
	unsigned int	queen;
	unsigned int	people;
	unsigned int	slave;
}EUserScore, *pEUserScore;

typedef struct
{
	unsigned int		mid;
	unsigned int		tid;
	unsigned int		money;
	int					commission;
	unsigned int		time;
	unsigned int		tax;
	unsigned int		base_chip;
	char				uuid[37];
}EUserMoneyLog, *pEUserMoneyLog;

using namespace Helper;

static const char* const DELIMITER_STR = "\\r\\n";

static const char* const GAME_LOG_QUEUE = "GAME_RD_QUEUE";
static const char* const USER_BANKRUPTCY_QUEUE = "USER_BANKRUPTCY_QUEUE";
static const char* const USER_MONEY_LOG_QUEUE = "USER_MONEY_QUEUE";
static const char* const USER_SCORE_LOG_QUEUE = "USER_SCORE_QUEUE";

static string& 
prepare_string(string& src, const string& delimiter)
{
	int pos = string::npos;
	
	
	while(pos = src.find(delimiter) != string::npos)
	{
		src.replace(pos, delimiter.size(), "\r\n");
	}
	
	return src;
}

static EGameLog& 
Json2EGameLog(const string& json, EGameLog& glog)
{
	Json::Value value;
	Json::Reader reader;

	if (reader.parse(json, value)) {
		snprintf(glog.log, sizeof(glog.log), "%s", value["m_s_log"].asCString());
		snprintf(glog.uuid, sizeof(glog.uuid), "%s", value["m_s_uuid"].asCString());

		glog.tid = value["m_n_id"].asUInt();
		glog.time = value["m_n_unix_time"].asUInt();
		glog.type = short(value["m_n_type"].asInt());
		glog.basechip = value["m_n_base_chip"].asUInt();
		glog.clear_times = value["m_n_clear_times"].asUInt();

	}

	return glog;
}

static EUserBankruptcy& 
Json2EUserBankruptcy(const string& json, EUserBankruptcy& ubank)
{
	Json::Value value;
	Json::Reader reader;

	if (reader.parse(json, value))
	{
		ubank.tid = value["m_n_tid"].asUInt();
		ubank.mid = value["m_n_mid"].asUInt();
		ubank.basechip = value["m_n_base_chip"].asInt();
		ubank.amount = value["m_n_amount"].asInt();
		ubank.time = value["m_n_unix_time"].asUInt();
	}
	
	return ubank; 
}

static
EUserScore& 
Json2EUserScore(const string& json, EUserScore& uscore)
{
	Json::Value value;
	Json::Reader reader;

	if (reader.parse(json, value))
	{
		uscore.mid = value["m_n_mid"].asUInt();
		uscore.king = value["m_n_king"].asUInt();
		uscore.queen = value["m_n_queen"].asUInt();
		uscore.people = value["m_n_people"].asUInt();
		uscore.slave = value["m_n_slave"].asUInt();
	}

	return uscore;
}

static
EUserMoneyLog&
Json2EUserMoneyLog(const string& json, EUserMoneyLog& umoney)
{
	Json::Value value;
	Json::Reader reader;

	if (reader.parse(json, value))
	{
		umoney.tid = value["m_n_tid"].asUInt();
		umoney.mid = value["m_n_id"].asUInt();
		umoney.money = value["m_n_money"].asInt() < 0 ? abs(value["m_n_money"].asInt()) : value["m_n_money"].asUInt();
		umoney.commission = value["m_n_turn_money"].asInt();
		umoney.time = value["m_n_unix_time"].asUInt();
		umoney.tax = value["m_n_tax"].asUInt();
		umoney.base_chip = value["m_n_base_chip"].asUInt();

		snprintf(umoney.uuid, sizeof(umoney.uuid), "%s", value["m_s_uuid"].asCString());
	}

	return umoney;
}


static
int 
EGameLog2DB(CMysqlHelper* mysql, EGameLog& glog)
{
	int ret = 0;

	if(!mysql->IsConnected())
	{
		mysql->Connect();

		log_debug("Connect Ret:%d\n", mysql->GetErrNo());
		log_debug("Connect Ret:%s\n", mysql->GetErrMsg());
		
		mysql->UseDB(mysql_conf.db);
	}

//		std::string sql_1 = "insert into tb_game_log_";
//		std::string sql_2 = "";
//		std::string sql_template = sql_1 + Today + sql_2;

	string sql_template = "insert into ks_logtable_%s(tid, basechip, content, time, type, lid, totalscore) values(%u, %u, '%s', %u, %d, '%s', %d)";

	char sql[5000] = {0};

	snprintf(sql, sizeof(sql), sql_template.c_str(), Today, glog.tid, glog.basechip, glog.log, glog.time, glog.type, glog.uuid, glog.clear_times);

	log_debug("SQL: %s\n", sql);

	ret = mysql->ExecuteNonQuery(sql);

	return ret;
}

static
int 
EUserBankruptcy2DB(CMysqlHelper* mysql, EUserBankruptcy& ubank)
{
	if(!mysql->IsConnected())
	{
		mysql->Connect();
		
		log_debug("Connect Ret:%d\n", mysql->GetErrNo());
		log_debug("Connect Ret:%s\n", mysql->GetErrMsg());
		
		mysql->UseDB(mysql_conf.db);
	}


	int ret = 0;

	std::string sql_template = "insert into ks_logbankrupt(tid, mid, basechip, time, amount) values(%u, %u, %d, %u, %d)";

	char sql[5000] = {0};

	snprintf(sql, sizeof(sql), sql_template.c_str(), ubank.tid, ubank.mid, ubank.basechip, ubank.time, ubank.amount);

	log_debug("SQL: %s\n", sql);

	ret = mysql->ExecuteNonQuery(sql);

	return ret;
}

static
int
EUserScore2DB(CMysqlHelper* mysql, EUserScore& uscore)
{
	if(!mysql->IsConnected())
	{
		mysql->Connect();
		log_debug("Connect Ret:%d\n", mysql->GetErrNo());
		log_debug("Connect Ret:%s\n", mysql->GetErrMsg());		
		mysql->UseDB(mysql_conf.db);
	}

	int ret = 0;

	string sql_template = "call p_upd_user_playlog(%u, %u, %u, %u, %u)";

	char sql[200] = {0};

	snprintf(sql, sizeof(sql), sql_template.c_str(), uscore.mid, uscore.king, uscore.queen, uscore.people, uscore.slave);

	log_debug("SQL: %s\n", sql);

	ret = mysql->ExecuteNonQuery(sql);

	return ret;
}

static
int
EUserMoney2DB(CMysqlHelper* mysql, EUserMoneyLog& umoney)
{
	int ret = 0;

	if(!mysql->IsConnected())
	{
		mysql->Connect();
		log_debug("Connect Ret:%d\n", mysql->GetErrNo());
		log_debug("Connect Ret:%s\n", mysql->GetErrMsg());
		mysql->UseDB(mysql_conf.db);
	}	

	string sql_template = "insert into ks_logmember_%s(lid, tid, uid, curmoney, turnmoney, tax, basechip, time) values('%s', %u, %u, %u, %d, %u, %u, %u)";

	char sql[1024] = {0};

	snprintf(sql, sizeof(sql), sql_template.c_str(), Today, umoney.uuid, umoney.tid, 
		umoney.mid, umoney.money, umoney.commission, umoney.tax, umoney.base_chip, umoney.time);

	log_debug("SQL: %s\n", sql);

	ret = mysql->ExecuteNonQuery(sql);

	return ret;
}


static
bool
IsNewDay()
{
	string day = GetDate();

	if (strcmp(day.c_str(), Today) != 0)
	{
		memcpy(Today, day.data(), day.size());
		return true;
	}
	
	return false;
}

static
int
NewGameLogTB(CMysqlHelper* mysql, const string& table)
{
	if(!mysql->IsConnected())
	{
		mysql->Connect();
		mysql->UseDB(mysql_conf.db);
	}

	string sql_1 = "call p_add_ks_logtable('" ;
	string sql_2 = "')";

	string sql =  sql_1 +  table + sql_2; 

	log_debug("SQL: %s\n", sql.c_str());

	int ret =  mysql->ExecuteNonQuery(sql);

	log_debug("ret: %s\n", mysql->GetErrMsg());
	log_debug("ret: %d\n", ret);

	return ret;
}

static
int
NewMoneyLogTB(CMysqlHelper* mysql, const string& table)
{
	if(!mysql->IsConnected())
	{
		mysql->Connect();
		mysql->UseDB(mysql_conf.db);
	}

	string sql_1 = "call p_add_ks_money_log('";
	string sql_2 = "')";

	string sql = sql_1 + table + sql_2;

	log_debug("SQL: %s\n", sql.c_str());
	
	int ret =  mysql->ExecuteNonQuery(sql);

	log_debug("ret: %s\n", mysql->GetErrMsg());
	log_debug("ret: %d\n", ret);

	return ret;
}

int
sync_game_log(CRedisHelper* rh, CMysqlHelper* mysql)
{
/*	if (IsNewDay())
	{
		int ret = NewGameLogTB(mysql, Today);

		if (ret)
		{
			if (1050 == ret)
			{
				log_debug("%s\n", mysql->GetErrMsg());
			}
			else
			{
				log_debug("create table game_log failed.\n");
			
				return 0;	
			}

		}
	}
*/
	string value;

	rh->Dequeue(queue_conf->names[0], value);

	if (value.size() == 0)
	{
		return 0;
	}

	log_debug("JSON: %s\n", value.c_str());
	
	EGameLog gl = {0};
	
	gl = Json2EGameLog(value, gl);

	//log_debug("gl.log: %s\n", gl.log);

	int ret = EGameLog2DB(mysql, gl);

	log_debug("ret:%d\n", mysql->GetErrNo());
		
	return 0;
}

int
sync_user_bankruptcy_log(CRedisHelper* rh, CMysqlHelper* mysql)
{
	string value;

	rh->Dequeue(queue_conf->names[1], value);

	if (value.size() == 0)
	{
		return 0;		
	}

	log_debug("JSON: %s\n", value.c_str());


	EUserBankruptcy ub = {0};

	ub = Json2EUserBankruptcy(value, ub);

	int ret = EUserBankruptcy2DB(mysql, ub);

	log_debug("ret:%d\n", mysql->GetErrNo());
	
	return 0;
}

int
sync_user_score_log(CRedisHelper* rh, CMysqlHelper* mysql)
{
	string value;

	rh->Dequeue(queue_conf->names[2], value);

	if (value.size() == 0)
	{
		return 0;
	}

	log_debug("JSON: %s\n", value.c_str());


	EUserScore us = {0};

	us = Json2EUserScore(value, us);

	int ret = EUserScore2DB(mysql, us);

	log_debug("ret:%d\n", mysql->GetErrNo());
	
	return 0;
}

int
sync_user_money_log(CRedisHelper* rh, CMysqlHelper* mysql)
{
/*	if (IsNewDay())
	{
		int ret = NewMoneyLogTB(mysql, Today);

		if (ret)
		{
			if (1050 == ret)
			{
				log_debug("%s\n", mysql->GetErrMsg());
			}
			else
			{
				log_debug("create table game_log failed.\n");
			
				return 0;	
			}

		}
	}

*/
	string value;

	rh->Dequeue(queue_conf->names[3], value);

	if (value.size() == 0)
	{
		return 0;
	}

	log_debug("JSON: %s\n", value.c_str());


	EUserMoneyLog um = {0};

	um = Json2EUserMoneyLog(value, um);

	int ret = EUserMoney2DB(mysql, um);

	log_debug("ret:%d\n", mysql->GetErrNo());
	
	return 0;
}

int sync_new_table(CMysqlHelper* mysql)
{
	int ret = 0;

	if (IsNewDay())
	{
		 ret = NewGameLogTB(mysql, Today);

		 if (ret)
		 {
			 if (1050 == ret) //表存在
			 {
				 log_debug("%s\n", mysql->GetErrMsg());
			 }
			 else
			 {
				 log_debug("create table game_log failed.\n");
				 log_debug("ErrMsg:%s\n", mysql->GetErrMsg());
			 }
		 
		 }

		 ret = NewMoneyLogTB(mysql, Today);
		 
		 if (ret)
		 {
			 if (1050 == ret)
			 {
				 log_debug("%s\n", mysql->GetErrMsg());
			 }
			 else
			 {
				 log_debug("create table money_log failed.\n");
				 log_debug("ErrMsg:%s\n", mysql->GetErrMsg());
			 }
		 
		 }		 


	}

	return ret;
}

