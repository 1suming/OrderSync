#ifndef __POLLER_H_
#define __POLLER_H_

#ifndef __MAX_EPOLL
#define __MAX_EPOLL 32
#endif

class CTcpClient;

typedef CTcpClient tcp_client_t;

class poller_t {
public:
	poller_t();
	~poller_t();
public:
	int attach(tcp_client_t* c);
	int apply_events(tcp_client_t* c);
	int detach(tcp_client_t* c);

	int create();
	int wait(int timeout);
	struct epoll_event* get_events() { return events; }
private:
	int 				efd;
	struct epoll_event	*events;
};


#endif

