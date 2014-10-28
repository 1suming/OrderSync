
#include <stdint.h>
#include <stdlib.h>
#include<stdio.h>
#include <timerlist.h>

CTimerObject::~CTimerObject(void) 
{

}

void CTimerObject::TimerNotify(void)
{
    abort ();
	delete this;
}

void CTimerObject::AttachTimer(class CTimerList *lst)
{
	if(lst->timeout > 0)
		objexp = GET_TIMESTAMP() + lst->timeout * TIMESTAMP_PRECISION;
	ListMoveTail(lst->tlist);
}

int CTimerList::CheckExpired(int64_t now) 
{
	int n = 0;
	if(now==0) 
	{
		now = GET_TIMESTAMP();
	}
	while(!tlist.ListEmpty()) 
	{
		CTimerObject *tobj = tlist.NextOwner();
        if(tobj->objexp > now) break;
        tobj->ListDel();
		tobj->TimerNotify();
		n++;
	}
	return n;
}

CTimerList *CTimerUnit::GetTimerList(int to)
{
	CTimerList *tl;

	for(tl = next; tl; tl=tl->next) 
	{
		if(tl->timeout == to)
			return tl;
	}
	tl = new CTimerList(to);
	tl->next = next;
	next = tl;
	return tl;
}

CTimerUnit::CTimerUnit(void) : pending(0), next(NULL)
{
}

CTimerUnit::~CTimerUnit(void) 
{
	while(next) 
	{
		CTimerList *tl = next;
		next = tl->next;
		delete tl;
	}
};

int CTimerUnit::ExpireMicroSeconds(int msec) 
{
	int64_t exp;
	int64_t now;
	CTimerList *tl;

	now = GET_TIMESTAMP();
#if TIMESTAMP_PRECISION >= 1000
	exp = now + msec*(TIMESTAMP_PRECISION/1000);
#else
	exp = now + msec*TIMESTAMP_PRECISION/1000;
#endif

	for(tl = next; tl; tl=tl->next) 
	{
		if(tl->tlist.ListEmpty())
			continue;
		CTimerObject *o = tl->tlist.NextOwner();
		if(o->objexp < exp)
			exp = o->objexp;
	}
	
	exp -= now;
	if(exp <= 0)
		return 0;
#if TIMESTAMP_PRECISION > 1000
	msec = exp / (TIMESTAMP_PRECISION/1000);
#else
	msec = exp * 1000/TIMESTAMP_PRECISION;
#endif
	return msec;
}

int CTimerUnit::CheckPending(void) 
{
	int n = 0;
	while(!pending.tlist.ListEmpty()) 
	{
		CTimerObject *tobj = pending.tlist.NextOwner();
		tobj->ListDel();
		tobj->TimerNotify();
		n++;
	}
	return n;
}

int CTimerUnit::CheckExpired(int64_t now)
{
	if(now==0)
		now = GET_TIMESTAMP();

	int n = CheckPending();
	CTimerList *tl;
	for(tl = next; tl; tl=tl->next)
	{
		n += tl->CheckExpired(now);
	}
	return n;
}

