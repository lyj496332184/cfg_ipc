#include "cfg_common.h"
#include "cfg_fdevent.h"
#include "cfg_ipc_server.h"

#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define ELF_CORE_SIZE				(256 * 1024)
#define MAX_OPEN_FILES				300
#define DEFAULT_SERVER_MAX_LISTEN	10
#define POLL_TIMEOUT				1
#define MAX_MSG_LEN					1024

static void signal_init(void)
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGUSR1, &act, NULL);
}

static void rlimit_init(void)
{
	struct rlimit rlim;
	int ret;
	
	ret = getrlimit(RLIMIT_NOFILE, &rlim);

	if (ret < 0)
	{
		DEBUG_ERR("getrlimit:RLIMIT_NOFILE error\n");

		return;
	}

	if (rlim.rlim_cur < MAX_OPEN_FILES)
	{
		rlim.rlim_cur = MAX_OPEN_FILES;
		setrlimit(RLIMIT_NOFILE, &rlim);
	}

	rlim.rlim_cur = ELF_CORE_SIZE;
	rlim.rlim_max = ELF_CORE_SIZE;
	
	setrlimit(RLIMIT_CORE, &rlim);
}

static int server_listen(const char *name, int backlog)
{
	int fd, len;
	struct sockaddr_un un;
	mode_t old_mode;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		DEBUG_ERR("create socket error: %s\n", strerror(errno));
		return -1;
	}

	unlink(name);

	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, name);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

	if (bind(fd, (struct sockaddr *)&un, len) < 0)
	{
		DEBUG_ERR("bind socket error: %s\n", strerror(errno));
		goto errout;
	}
	
	old_mode = umask(0);
	if (chmod(name, 0777) < 0)
	{
		umask(old_mode);
		DEBUG_ERR("chmod %s error\n", name);
		goto errout;
	}

	umask(old_mode);

	if (backlog <= 0)
	{
		backlog = DEFAULT_SERVER_MAX_LISTEN;
	}
	
	if (listen(fd, backlog) < 0)
	{
		DEBUG_ERR("listen socket error: %s\n", strerror(errno));
		goto errout;
	}

	return fd;

errout:
	close(fd);
	return -1;
}

static ssize_t sread(int fd, void *buf, size_t n)
{
	ssize_t nread = -1;
	 
	while (1) 
	{
		if ((nread = read(fd, buf, n)) < 0) 
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) 
			{
				return (-1);     /* fd is non-blocking */
			}
			else if (errno == EINTR) 
			{
				nread = 0;          /* and call read() again */
			}
			else
			{
				return(-1);
			}
		}
		else if (nread == 0)
		{
			break;                    /* EOF */
		}
		else
		{
			break;
		}
	}

	return nread;
} 

static int handle_listen_event(cfg_fdevents *ev, int fd, int revents)
{
	int cli_fd;
	struct sockaddr_un cli;
	socklen_t len;
	int fde_ndx = -1;

	if (revents &= FDEVENT_IN)
	{
		len = sizeof(cli);
		
		if ((cli_fd = accept(fd, (struct sockaddr *)&cli, &len)) < 0)
			return -1;

		return cfg_fdevent_event_set(ev, &fde_ndx, cli_fd, FDEVENT_IN);
	}

	return -1;
}

static int handle_client_event(cfg_fdevents *ev, int fd, int revents)
{
	char buf[MAX_MSG_LEN + 1]= {0};
	int nread;
	int fde_ndx = -1;

	if (revents &= FDEVENT_IN)
	{
		nread = sread(fd, buf, MAX_MSG_LEN);

		if (nread < 0)
		{
			DEBUG_ERR("read socket error: %s\n", strerror(errno));

			return -1;
		}
		else if (nread == 0)
		{
			close(fd);
			cfg_fdevent_event_del(ev, &fde_ndx, fd);

			return 0;
		}
		
		if (ev->handler_client_msg)
		{
			ev->handler_client_msg(fd, buf);
		}

		return nread;
	}

	return -1;
}

int cfg_ipc_server_start(cfg_fdevents *ev)
{
	int listenfd = -1;
	int fde_ndx = -1;
	int timeout = POLL_TIMEOUT;
	int ready_num = -1;

	signal_init();
	rlimit_init();
	
	if (cfg_fdevent_init(ev) < 0)
	{
		DEBUG_ERR("init server fdevent error");
		goto errout;
	}
	
	cfg_fdevent_reset(ev);
	listenfd = server_listen(SERVER_IPC_DOMAIN_NAME, ev->maxfds);

	if (listenfd < 0)
	{
		DEBUG_ERR("create server socket error: %s\n", strerror(errno));
		goto errout;
	}

	cfg_fdevent_event_set(ev, &fde_ndx, listenfd, FDEVENT_IN);

	while(1)
	{
		if ((ready_num = cfg_fdevent_poll(ev, timeout)) > 0)
		{
			int revents;
			int fd_ndx;
			int fd;
			
			fd_ndx = -1;

			do
			{
				fd_ndx = cfg_fdevent_event_next_fdndx(ev, fd_ndx);

				if (-1 == fd_ndx)
					break;

				revents = cfg_fdevent_event_get_revent(ev, fd_ndx);
				fd = cfg_fdevent_event_get_fd(ev, fd_ndx);

				if (fd == listenfd)
					handle_listen_event(ev, fd, revents);
				else
					handle_client_event(ev, fd, revents);
			}while (--ready_num > 0);
		}
		else if (ready_num < 0 && errno != EINTR)
		{
			DEBUG_ERR("cfg poll fdevent error: %s\n", strerror(errno));
		}	
	}
	
errout:
	cfg_fdevent_free(ev);
	
	if (listenfd > 0)
	{
		close(listenfd);
		unlink(SERVER_IPC_DOMAIN_NAME);		
	}

	return -1;
}
