#include "cfg_ipc_server.h"
#include <stdio.h>

#define SERVER_MAX_FDS		20

int dispatch_client_msg(const char *msg)
{
	printf("server recv msg: %s\n", msg);

	return 1;
}

static cfg_fdevents server_ev = {
	.maxfds = SERVER_MAX_FDS,
	.type = FDEVENT_HANDLER_SELECT,
	.handler_client_msg = dispatch_client_msg,
};

int main()
{
	if (cfg_ipc_server_start(&server_ev) < 0)
		fprintf(stderr, "cfg_ipc_server_start failed\n");

	return 0;
}

