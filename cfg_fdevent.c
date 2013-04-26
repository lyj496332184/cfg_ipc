#include "cfg_common.h"
#include "cfg_fdevent.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

cfg_fdevents *cfg_fdevent_init(size_t maxfds, cfg_fdevent_handler_t type)
{
	cfg_fdevents *ev;

	ev = calloc(1, sizeof(*ev));
	ev->maxfds = maxfds;

	switch(type)
	{
	case FDEVENT_HANDLER_SELECT:
		if (0 != fdevent_select_init(ev))
		{
			DEBUG_ERR("event-handler select failed: %s\n", strerror(errno));
			return NULL;
		}
		return ev;
	case FDEVENT_HANDLER_UNSET:
		break;
	}

	DEBUG_ERR("event-handler is unknown, try to set server.event-handler = \"select\"\n");
	return NULL;
}

void cfg_fdevent_free(cfg_fdevents *ev)
{
	if (!ev)
		return;

	free(ev);
}

int cfg_fdevent_reset(cfg_fdevents *ev)
{
	if (ev->reset)
		return ev->reset(ev);

	return 0;
}

int cfg_fdevent_event_del(cfg_fdevents *ev, int *fde_ndx, int fd)
{
	int fde = fde_ndx ? *fde_ndx : -1;

	if (ev->event_del)
		fde = ev->event_del(ev, fde, fd);

	if (fde_ndx)
		*fde_ndx = fde;

	return 0;
}

int cfg_fdevent_event_set(cfg_fdevents *ev, int *fde_ndx, int fd, int events)
{
	int fde = fde_ndx ? *fde_ndx : -1;

	if (ev->event_set)
		fde = ev->event_set(ev, fde, fd, events);

	if (fde_ndx)
		*fde_ndx = fde;

	return 0;
}

int cfg_fdevent_poll(cfg_fdevents *ev, int timeout_ms)
{
	if (ev->poll == NULL)
		SEGFAULT();
	return ev->poll(ev, timeout_ms);
}

int cfg_fdevent_event_get_revent(cfg_fdevents *ev, size_t ndx)
{
	if (ev->event_get_revent == NULL)
		SEGFAULT();

	return ev->event_get_revent(ev, ndx);
}

int cfg_fdevent_event_get_fd(cfg_fdevents *ev, size_t ndx)
{
	if (ev->event_get_fd == NULL)
		SEGFAULT();

	return ev->event_get_fd(ev, ndx);
}

int cfg_fdevent_event_next_fdndx(cfg_fdevents *ev, int ndx)
{
	if (ev->event_next_fdndx)
		return ev->event_next_fdndx(ev, ndx);

	return -1;
}

