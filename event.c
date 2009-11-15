/* $Id: event.c 1040 2009-09-23 04:14:17Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/event.c $
 *
 * generic timer handling
 *
 * Copyright (C) 2009 Ed Martin <edman007@edman007.com>
 * Copyright (C) 2004, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* 
 * exported functions:
 *
 * int event_add(void (*callback) (void *data), void *data, const int fd, const int read, const int write);
 *   Adds a file description to watch
 *
 * int event_del(const int fd);
 *   Remove an event
 *
 * int event_modify(const int fd, const int read, const int write, const int active);
 *   Modify an event
 *
 * int named_event_add(char *event, void (*callback) (void *data), void *data);
 *   Add an event identified by a string
 *
 * int named_event_del(char *event, void (*callback) (void *data), void *data);
 *   delete the event identified by this string/callback/data
 *
 * int named_event_trigger(char *event);	//call all calbacks for this event
 *   call the callbacks of all events that have identified as this string
 *
 * int event_process(const struct timespec *delay);
 *   process the event list
 *
 * void event_exit();
 *   releases all events
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#include <poll.h>

#include "debug.h"
#include "cfg.h"
#include "event.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

typedef struct {
    void (*callback) (event_flags_t flags, void *data);
    void *data;
    int fd;
    int read;
    int write;
    int active;
    int fds_id;
} event_t;


//our set of FDs
static event_t *events = NULL;
static int event_count = 0;
static void free_events(void);

int event_add(void (*callback) (event_flags_t flags, void *data), void *data, const int fd, const int read,
	      const int write, const int active)
{
    event_count++;
    events = realloc(events, sizeof(event_t) * event_count);

    int i = event_count - 1;
    events[i].callback = callback;
    events[i].data = data;
    events[i].fd = fd;
    events[i].read = read;
    events[i].write = write;
    events[i].active = active;
    events[i].fds_id = -1;
    return 0;
}


int event_process(const struct timespec *timeout)
{
    int i, j;
    struct pollfd *fds = malloc(sizeof(struct pollfd) * event_count);
    for (i = 0, j = 0; i < event_count; i++) {
	events[i].fds_id = -1;
	if (events[i].active) {
	    events[i].fds_id = j;
	    fds[j].events = 0;
	    fds[j].fd = events[i].fd;
	    if (events[i].read) {
		fds[j].events |= POLLIN;
	    }
	    if (events[i].write) {
		fds[j].events |= POLLOUT;
	    }
	    j++;
	}
    }
    int ready = ppoll(fds, j, timeout, NULL);

    if (ready > 0) {
	//search the file descriptors, call all relavant callbacks
	for (i = 0, j = 0; i < event_count; i++) {
	    if (events[i].fds_id != j) {
		continue;
	    }
	    if (fds[j].revents) {
		int flags = 0;
		if (fds[j].revents & POLLIN) {
		    flags |= EVENT_READ;
		}
		if (fds[j].revents & POLLOUT) {
		    flags |= EVENT_WRITE;
		}
		if (fds[j].revents & POLLHUP) {
		    flags |= EVENT_HUP;
		}
		if (fds[j].revents & POLLERR) {
		    flags |= EVENT_ERR;
		}
		events[i].callback(flags, events[i].data);

	    }
	    j++;
	}
    }

    return 0;

}

int event_del(const int fd)
{
    int i;
    for (i = 0; i < event_count; i++) {
	if (events[i].fd == fd) {
	    events[i] = events[event_count - 1];
	    break;
	}
    }
    event_count--;
    events = realloc(events, sizeof(event_t) * event_count);
    return 0;
}

int event_modify(const int fd, const int read, const int write, const int active)
{
    int i;
    for (i = 0; i < event_count; i++) {
	if (events[i].fd == fd) {
	    events[i].read = read;
	    events[i].write = write;
	    events[i].active = active;
	    break;
	}
    }
    event_count--;
    events = realloc(events, sizeof(event_t) * event_count);
    return 0;
}

static void free_events(void)
{
    if (events != NULL) {
	free(events);
    }
    event_count = 0;
    events = NULL;

}

void event_exit(void)
{
    free_events();
}

/*
 * Named events are the user facing side of the event subsystem
 *
 */


//we rely on "=" working for copying these structs (no pointers except the callers)
typedef struct {
    void (*callback) (void *data);
    void *data;
} event_callback_t;

typedef struct {
    char *name;
    event_callback_t *c;
    int callback_count;
} named_event_list_t;


static named_event_list_t *ev_names = NULL;
static int ev_count = 0;

int named_event_add(char *event, void (*callback) (void *data), void *data)
{
    if (event == NULL || strlen(event) == 0) {
	return 1;
    }
    if (callback == NULL) {
	return 2;
    }
    int i;
    for (i = 0; i < ev_count; i++) {
	if (0 == strcmp(event, ev_names[i].name)) {
	    break;
	}
    }
    if (i >= ev_count) {
	//create the entry
	ev_count++;
	ev_names = realloc(ev_names, sizeof(named_event_list_t) * ev_count);
	ev_names[i].name = strdup(event);
	ev_names[i].callback_count = 0;
	ev_names[i].c = NULL;
    }
    int j = ev_names[i].callback_count;
    ev_names[i].callback_count++;
    ev_names[i].c = realloc(ev_names[i].c, sizeof(event_callback_t) * ev_names[i].callback_count);

    ev_names[i].c[j].callback = callback;
    ev_names[i].c[j].data = data;
    return 0;
}

int named_event_del(char *event, void (*callback) (void *data), void *data)
{
    int i, j;
    for (i = 0; i < ev_count; i++) {
	if (0 == strcmp(event, ev_names[i].name)) {
	    break;
	}
    }
    if (i >= ev_count) {
	return 1;		//nothing removed
    }
    for (j = 0; j < ev_names[i].callback_count; j++) {
	if (ev_names[i].c[j].callback == callback && ev_names[i].c[j].data == data) {
	    ev_names[i].callback_count--;
	    ev_names[i].c[j] = ev_names[i].c[ev_names[i].callback_count];
	    ev_names[i].c = realloc(ev_names[i].c, sizeof(event_callback_t) * ev_names[i].callback_count);
	    if (ev_names[i].callback_count == 0) {
		//drop this event
		free(ev_names[i].name);
		ev_count--;
		ev_names[i] = ev_names[ev_count];
		ev_names = realloc(ev_names, sizeof(named_event_list_t) * ev_count);
	    }
	    return 0;
	}
    }
    return 2;
}

int named_event_trigger(char *event)
{
    int i, j;
    for (i = 0; i < ev_count; i++) {
	if (0 == strcmp(event, ev_names[i].name)) {
	    for (j = 0; j < ev_names[i].callback_count; j++) {
		ev_names[i].c[j].callback(ev_names[i].c[j].data);
	    }
	    return 0;
	}
    }
    return 1;
}
