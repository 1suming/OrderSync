#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <memcheck.h>
#include <stdlib.h>

//#include <poll_thread.h>
#include <poller.h>
#include <log.h>
#include <errno.h>

CPollerObject::CPollerObject (CPollerUnit *o, int fd) :
    netfd (fd),
    ownerUnit(o),
    newEvents (0),
    oldEvents (0),
    epslot(NULL)
{
}

CPollerObject::~CPollerObject ()
{
	if (ownerUnit && epslot)
		ownerUnit->FreeEpollSlot (epslot);

	if (netfd > 0)
    {
		close(netfd);
        log_debug ("netfd[%d] closed", netfd);
		netfd = 0;
	}
    else
    {
        log_debug ("netfd[%d], already closed.", netfd);
    }
}

int CPollerObject::AttachPoller (CPollerUnit *unit)
{
	if(unit)
    {
		if( ownerUnit==NULL)
			ownerUnit = unit;
		else
			return -1;
	}
	if(netfd < 0)
		return -1;
	if(epslot==NULL)
    {
		if (!(epslot = ownerUnit->AllocEpollSlot ()))
        {
			return -1;
        }

		epslot->poller = this;

		int flag = fcntl (netfd, F_GETFL);
		fcntl (netfd, F_SETFL, O_NONBLOCK | flag);
		struct epoll_event ev;
		ev.events = newEvents;
		ev.data.u64 = (++epslot->seq);
		ev.data.u64 = (ev.data.u64 << 32) + ownerUnit->GetSlotId (epslot); // ��poller object �������¼�

		if (ownerUnit->Epctl (EPOLL_CTL_ADD, netfd, &ev) == 0)
        {
			oldEvents = newEvents;
        }
		else
        {
			return -1;
		}
		return 0;

	}
	return ApplyEvents ();
}

uint64_t CPollerObject::GetPollerEventId()
{
	if(epslot == NULL)
	{
		return 0;
	}
	uint64_t id = epslot->seq;
	return ((id << 32) + ownerUnit->GetSlotId (epslot));
}

int CPollerObject::DetachPoller() 
{
	if(epslot) 
	{
		struct epoll_event ev;
		if (ownerUnit->Epctl (EPOLL_CTL_DEL, netfd, &ev) == 0)
			oldEvents = newEvents;
		else 
		{
			log_error("Epctl:%d %m", errno);
			//return -1;
		}
		ownerUnit->FreeEpollSlot(epslot);
		epslot = NULL;
	}
	return 0;
}

int CPollerObject::ApplyEvents ()
{
	if (epslot == NULL || oldEvents == newEvents)
		return 0;

	struct epoll_event ev;

	ev.events = newEvents;
	ev.data.u64 = (++epslot->seq);
	ev.data.u64 = (ev.data.u64 << 32) + ownerUnit->GetSlotId (epslot);
	if (ownerUnit->Epctl (EPOLL_CTL_MOD, netfd, &ev) == 0)
		oldEvents = newEvents;
	else
	{
		//log_warning("Epctl: %m");
		return -1;
	}

	return 0;
}

int CPollerObject::InputNotify(void)
{
	EnableInput(false);

    return POLLER_SUCC;
}

int CPollerObject::OutputNotify(void)
{
	EnableOutput(false);

    return POLLER_SUCC;
}

int CPollerObject::HangupNotify(void) 
{
    abort ();
	delete this;

    return POLLER_FAIL;
}

CPollerUnit::CPollerUnit(int mp)
{
	maxPollers   = mp;
	epfd         = -1;
	ep_events    = NULL;
	pollerTable  = NULL;
	freeSlotList = NULL;
	usedPollers  = 0;
}

CPollerUnit::~CPollerUnit() 
{
	for (int i = 0; i < maxPollers; i++) 
	{
		if (pollerTable[i].freeList)
			continue;
		//delete pollerTable[i].poller;
	}

	FREE_CLEAR(pollerTable);

	if (epfd != -1)
	{
		close (epfd);
		epfd = -1;
	}

	FREE_CLEAR(ep_events);
}

int CPollerUnit::SetMaxPollers(int mp)
{
	if(epfd >= 0)
    {
		return -1;
    }

	maxPollers = mp;

	return 0;
}

int CPollerUnit::InitializePollerUnit(void) //called by CIncoming::open
{
	pollerTable = (struct CEpollSlot *)CALLOC(maxPollers, sizeof (*pollerTable));

	if (!pollerTable)
	{
		log_error("calloc failed, num=%d, %m", maxPollers);
		return -1;
	}

	for (int i = 0; i < maxPollers - 1; i++)
	{
		pollerTable[i].freeList = &pollerTable[i+1];
	}

	pollerTable[maxPollers - 1].freeList = NULL;
	freeSlotList = &pollerTable[0];

	ep_events = (struct epoll_event *)CALLOC(maxPollers, sizeof (struct epoll_event));

	if (!ep_events)
	{
		log_error("malloc failed, %m");
		return -1;
	}

	if ((epfd = epoll_create (maxPollers)) == -1)
	{
		log_warning("epoll_create failed, %m");
		return -1;
	}
	return 0;
}

inline int CPollerUnit::VerifyEvents (struct epoll_event *ev)
{
	int idx = EPOLL_DATA_SLOT (ev);
	
	if ((idx >= maxPollers) || (EPOLL_DATA_SEQ (ev) != pollerTable[idx].seq))
	{
		return -1;
	}

	return 0;
}

void CPollerUnit::FreeEpollSlot (CEpollSlot *p)
{
	p->freeList = freeSlotList;
	p->poller = NULL;
	freeSlotList = p;
	usedPollers--;
	p->seq++;
}

struct CEpollSlot *CPollerUnit::AllocEpollSlot ()
{
	CEpollSlot *p = freeSlotList;

	if (0 == p) 
	{
		log_warning("no free epoll slot, usedPollers=%d", usedPollers);
		return NULL;
	}
	
	usedPollers++;
	freeSlotList = freeSlotList->freeList;
	p->freeList  = NULL;

	return p;
}

int CPollerUnit::Epctl (int op, int fd, struct epoll_event *events)
{
	if (epoll_ctl (epfd,  op, fd, events) == -1)
    {
		//log_warning("epoll_ctl error, epfd=%d, fd=%d", epfd, fd);

		return -1;
	}

	return 0;
}

void CPollerUnit::WaitPollerEvents(int timeout)
{
	nrEvents = epoll_wait (epfd, ep_events, maxPollers, timeout);
}

void CPollerUnit::ProcessPollerEvents(void)
{
    int ret_code = -1;
// 	log_debug ("events [%d]", nrEvents);
	for (int i = 0; i < nrEvents; i++)
	{
		if(VerifyEvents (ep_events+i) == -1)
		{
			log_notice("VerifyEvents failed, ep_events[%d].data.u64 = %llu", i, (unsigned long long)ep_events[i].data.u64);
			continue;
		}

		CEpollSlot *s = &pollerTable[EPOLL_DATA_SLOT(ep_events+i)];
		CPollerObject *p = s->poller;

		p->newEvents = p->oldEvents;

        if(ep_events[i].events & (EPOLLHUP | EPOLLERR)) {
            log_debug ("EPOLLHUP | EPOLLERR netfd[%d]", p->netfd);
			p->HangupNotify();
			continue;
		}

		if(ep_events[i].events & EPOLLIN) {
            log_debug ("EPOLLIN netfd[%d]", p->netfd);
			ret_code = p->InputNotify();
			log_debug ("ret_code [%d]", ret_code);
            //因为在InputNotify里面已经删除了自己，所以这里必须判断是否已经detach
            if (s->poller != p)
            {
                continue;
            }

            //再次检测返回值，如果不等于POLLER_SUCC，则表示已经delete掉自己，或者异常
            if (ret_code != POLLER_SUCC)
            {
                continue;
            }
        }

        if(ep_events[i].events & EPOLLOUT)
        {
            log_debug ("EPOLLOUT netfd[%d]", p->netfd);
			ret_code = p->OutputNotify();

            //因为在OutputNotify里面已经删除了自己，所以这里必须判断是否已经detach
            if (s->poller != p) continue;

            //再次检测返回值，如果不等于POLLER_SUCC，则表示已经delete掉自己，或者异常
            if (ret_code != POLLER_SUCC) continue;
        }

        //因为在InputNotify, OutputNotify退出后，已经检测指针是否已经detach，因此这里无需判断
   		//if(s->poller==p)
        //{
			p->ApplyEvents();
        //}
	}
}

