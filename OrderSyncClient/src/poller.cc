#include "poller.h"
#include "TcpClient.h"

#include <sys/epoll.h>

poller_t::poller_t():efd(0)
{
	events = (struct epoll_event*)malloc(sizeof *events);
}

poller_t::~poller_t()
{
	free(events);
}

int
poller_t::attach(tcp_client_t* c)
{
	struct epoll_event 	ev;
	int 				fd;

	fd = c->get_fd();
	ev.events = c->get_event();
	ev.data.ptr = c;

	return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
}

int
poller_t::apply_events(tcp_client_t* c)
{
	struct epoll_event 	ev;
	int 				fd;

	fd = c->get_fd();
	ev.events = c->get_event();
	ev.data.ptr = c;

	return epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev);
}

int
poller_t::detach(tcp_client_t* c)
{
	struct epoll_event 	ev;
	int 				fd;

	fd = c->get_fd();

	return epoll_ctl(efd, EPOLL_CTL_DEL, fd, &ev);
}

int
poller_t::create()
{
	efd = epoll_create(__MAX_EPOLL);

	return efd;
}

int
poller_t::wait(int timeout)
{
	return epoll_wait(efd, events, __MAX_EPOLL, timeout);
}

