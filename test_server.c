#include "cfg_ipc_server.h"
#include <stdio.h>

int main()
{
	if (cfg_ipc_server_start() < 0)
		fprintf(stderr, "cfg_ipc_server_start failed\n");

	return 0;
}

