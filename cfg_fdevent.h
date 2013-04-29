#ifndef _CFG_FDEVENT_H_
#define _CFG_FDEVENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>

#define SEGFAULT() do { fprintf(stderr, "%s.%d: aborted\n", __FILE__, __LINE__); abort(); } while(0)
#define UNUSED(x) ( (void)(x) )

#define BV(x) (1 << x)

/* these are the POLL* values from <bits/poll.h> (linux poll)
 */

#define FDEVENT_IN     BV(0)
#define FDEVENT_PRI    BV(1)
#define FDEVENT_OUT    BV(2)
#define FDEVENT_ERR    BV(3)
#define FDEVENT_HUP    BV(4)
#define FDEVENT_NVAL   BV(5)

typedef enum { FDEVENT_HANDLER_UNSET,
		FDEVENT_HANDLER_SELECT
} cfg_fdevent_handler_t;


/**
 * fd-event handler for select(), poll() and rt-signals on Linux 2.4
 *
 */
typedef struct cfg_fdevents {
	cfg_fdevent_handler_t type;

	size_t maxfds;

#ifdef USE_SELECT
	fd_set select_read;
	fd_set select_write;
	fd_set select_error;

	fd_set select_set_read;
	fd_set select_set_write;
	fd_set select_set_error;

	int select_max_fd;
#endif
	int (*reset)(struct cfg_fdevents *ev);
	void (*free)(struct cfg_fdevents *ev);

	int (*event_set)(struct cfg_fdevents *ev, int fde_ndx, int fd, int events);
	int (*event_del)(struct cfg_fdevents *ev, int fde_ndx, int fd);
	int (*event_get_revent)(struct cfg_fdevents *ev, size_t ndx);
	int (*event_get_fd)(struct cfg_fdevents *ev, size_t ndx);

	int (*event_next_fdndx)(struct cfg_fdevents *ev, int ndx);

	int (*poll)(struct cfg_fdevents *ev, int timeout_ms);

	/* handle client msg */
	int (*handler_client_msg)(int fd, void *msg);
} cfg_fdevents;

int cfg_fdevent_init(cfg_fdevents *ev);
int cfg_fdevent_reset(cfg_fdevents *ev);
void cfg_fdevent_free(cfg_fdevents *ev);

int cfg_fdevent_event_set(cfg_fdevents *ev, int *fde_ndx, int fd, int events); /* events can be FDEVENT_IN, FDEVENT_OUT or FDEVENT_IN | FDEVENT_OUT */
int cfg_fdevent_event_del(cfg_fdevents *ev, int *fde_ndx, int fd);
int cfg_fdevent_event_get_revent(cfg_fdevents *ev, size_t ndx);
int cfg_fdevent_event_get_fd(cfg_fdevents *ev, size_t ndx);

int cfg_fdevent_event_next_fdndx(cfg_fdevents *ev, int ndx);

int cfg_fdevent_poll(cfg_fdevents *ev, int timeout_ms);

int fdevent_select_init(cfg_fdevents *ev);

#endif	/* _CFG_EVENT_H_ */

