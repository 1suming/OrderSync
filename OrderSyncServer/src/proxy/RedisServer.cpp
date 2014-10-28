#include "RedisServer.h"
#include "Json/json.h"


CRedisServer::CRedisServer()
{
	m_strIp = "";
	m_nPort = 0;
	m_pRedisConn = NULL;
	m_pRedisReply = NULL;
}

CRedisServer::~CRedisServer()
{
}

int CRedisServer::Connect(string& ip, int port)
{
	m_strIp = ip;
	m_nPort = port;
	m_pRedisConn = redisConnect(ip.c_str(), port);
	if (m_pRedisConn->err)
	{
		//ACE_DEBUG((LM_ERROR, ACE_TEXT("Connect Redis Server failed, ip:%s, port:%d\r\n"), ip.c_str(), port));
		return -1;
	}
	else
	{
		//ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Connect Redis Server success, ip:%s, port:%d\n"), ip.c_str(), port));
	}
	
	return 0;
}

int CRedisServer::Reconnect()
{
	redisFree(m_pRedisConn);
	m_pRedisConn = redisConnect(m_strIp.c_str(), m_nPort);
	if (m_pRedisConn->err)
	{
		return -1;
	}
	return 0;
}

int CRedisServer::SetExpire(char *key, int expire)
{
	m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn, "EXPIRE %s %d", key, expire));
	if (m_pRedisReply != NULL)
	{
		freeReplyObject(m_pRedisReply);
	}
	else
	{
		//ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SetExpire m_pRedisReply=NULL\r\n")));
	}
	return 0;
}

int CRedisServer::SetTimeout(struct timeval tv)
{
	if(m_pRedisConn!=NULL)
	{
		int ret = redisSetTimeout(m_pRedisConn,tv);
		return ret;
	}
	return REDIS_ERR;
}

redisReply * CRedisServer::RedisCommand(const char * command)
{
	m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn, command));
	if(m_pRedisReply == NULL) // lose connection , reconnect .
	{
		int ret = Reconnect();
		if(ret != 0)
		{
			return NULL;
		}
		else
		{
			m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn,command));
			if(m_pRedisReply == NULL)
			{
				return NULL;
			}
			else
			{
				return m_pRedisReply;
			}
		}
	}
	return m_pRedisReply;
}


int CRedisServer::SetInfo(string ip,int port,int sid)
{
	int ret = -3;
	char szMainKey[32] = {0};
	char szKey[32] = {0};
	
	snprintf(szMainKey, sizeof(szMainKey), "HALLSERVER");
	snprintf(szKey, sizeof(szMainKey), "HALLSERVER%d", sid);
	
	
	char Info[300] = {0};
	
	snprintf(Info,sizeof(Info),"%s,%d,%d",ip.c_str(),port,sid);
	

	Json::Value value;
	value["IP"] = ip.c_str();
	value["PORT"] = port;
	value["SVID"] = sid;

	Json::FastWriter writer;
	string strValue = writer.write(value);
	m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn, "HSET %s %s %s", szMainKey, szKey, strValue.c_str()));
	if(m_pRedisReply != NULL)
	{
	//	value = m_pRedisReply->str;
		freeReplyObject(m_pRedisReply);
	}
	
	return 0;
}

