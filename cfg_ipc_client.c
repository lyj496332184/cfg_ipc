#include "cfg_common.h"

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define MAX_MSG_LEN			1024

static void signal_init(void)
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGUSR1, &act, NULL);
}

static int client_conn(const char *server_name, const char *client_name)
{
	int fd, len;
	struct sockaddr_un un;
	mode_t old_mode;
	
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		DEBUG_ERR("create socket error:%s", strerror(errno));
		return -1;
	}

	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, client_name); 
	len = offsetof(struct sockaddr_un, sun_path) + strlen(client_name);

	unlink(client_name);
	
	if (bind(fd, (struct sockaddr *)&un, len) < 0)
	{
		DEBUG_ERR("bind error:%s", strerror(errno));
		goto errout;
	}

	old_mode = umask(0);
	if (chmod(un.sun_path, 0777) < 0)
	{
		umask(old_mode);
		DEBUG_ERR("chmod %s error\n", un.sun_path);
		goto errout;
	}
	
	umask(old_mode);
	
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, server_name); 
	len = offsetof(struct sockaddr_un, sun_path) + strlen(server_name);
	
	if (connect(fd, (struct sockaddr *)&un, len) < 0)
	{
		DEBUG_ERR("connect error:%s", strerror(errno));
		goto errout;
	}

	return fd;

errout:
	close(fd);
	return -1;
}

/* Write "n" bytes to a descriptor. */
ssize_t swriten(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					return (-1);          /* fd is non-blocking */
				}
				else if (errno == EINTR)
				{
					nwritten = 0;          /* and call write() again */
				}
			}
			else
			{
				return(-1);               /* error */
			}
		}

		nleft -= nwritten;
		ptr += nwritten;
	}

	return(n);
}

int main()
{
	int fd;
	char buf[MAX_MSG_LEN] = {0};
	char client_name[128] = {0};
	pid_t pid;

	signal_init();
	
	sprintf(client_name, "%05d.socket", getpid());

	fd = client_conn(SERVER_IPC_DOMAIN_NAME, client_name);

	pid = fork();

	if (pid < 0)
	{
		DEBUG_ERR("fork error\n");
	}
	else if (pid == 0)
	{
		sprintf(client_name, "%05d.socket", getpid());
		fd = client_conn(SERVER_IPC_DOMAIN_NAME, client_name);
		if (fd < 0)
		{
			DEBUG_ERR("create server unix domain socket error\n");
			return -1;
		}

		memset(buf, 0 ,sizeof(buf));
		sprintf(buf, "client_name %s", client_name);
		write(fd, buf, sizeof(buf));

		close(fd);
		unlink(client_name);

		exit(0);
	}
	
	if (fd < 0)
	{
		DEBUG_ERR("create server unix domain socket error\n");
		return -1;
	}

	sprintf(buf, "client_name %s", client_name);
	swriten(fd, buf, sizeof(buf));

	close(fd);
	unlink(client_name);
	
	return 0;
}
