
#pragma once

#include <timerlist.h>
#include <global.h>

#include "CHelper_pool.h"
#include "ICHAT_PacketBase.h"
#include "EncryptDecrypt.h"


class CTimerObject;


HTTP_SVR_NS_BEGIN



class CRealTimer : public CTimerObject
{
public:

	CRealTimer(){}
	~CRealTimer(){}

	CTimerList* realTimerlist;
	
	virtual void TimerNotify(void);	 
};



class CLevelCountTimer: public CTimerObject
{
public:
	CLevelCountTimer()	{}
	~CLevelCountTimer()	{}

	virtual void TimerNotify();

	inline void SetTimerList(CTimerList *pTimerList)
	{
		m_TimerList = pTimerList;
	}

	inline void SetHelperTimerList(CTimerList *pTimerList)
	{
		m_HelperList = pTimerList;	
	}

	inline void StartTimer()
	{
		if( NULL != m_TimerList)
		{
			AttachTimer(m_TimerList);	
		}
	}

	void SendGetLevelCount();

private:
	CTimerList*	m_TimerList;
	CTimerList*	m_HelperList;
};


HTTP_SVR_NS_END




