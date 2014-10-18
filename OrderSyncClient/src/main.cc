#include "order_sync_client.h"
#include "fetcher.h"

int 
main(int argc, char** argv)
{
	order_sync_client_t *instance;
	fetcher_t			*f;
	tcp_client_t		*c;

	instance = new order_sync_client_t(f, c);

	if (instance) {
		instance->run();
	}

	return 0;
}

