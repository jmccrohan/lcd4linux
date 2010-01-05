/* $Id: event.h 840 2007-09-09 12:17:42Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/event.h $
 *
 * generic timer handling
 *
 * Copyright (C) 2009 Ed Martin <edman007@edman007.com>
 * Copyright (C) 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#ifndef _EVENT_H_
#define _EVENT_H_

#include <time.h>

//events are identified by their file descriptor only

/*
 * these functions allow plugins to add event hooks
 * (and thus break out of the main loop when sleeping)
 * these callbacks than then trigger named events to propagate
 * the event to the screen
 */

typedef enum {
    EVENT_READ = 1,
    EVENT_WRITE = 2,
    EVENT_HUP = 4,
    EVENT_ERR = 8
} event_flags_t;


int event_add(void (*callback) (event_flags_t flags, void *data), void *data, const int fd, const int read,
	      const int write, const int active);
int event_del(const int fd);
int event_modify(const int fd, const int read, const int write, const int active);
int event_process(const struct timespec *timeout);
void event_exit(void);

/*
 * These fuctions keep a list of the events to trigger on allowing multiple
 * things to trigger an event and multiple things to receive the event
 */

//add an event to be triggered
int named_event_add(char *event, void (*callback) (void *data), void *data);
//remove an event from the list of events
int named_event_del(char *event, void (*callback) (void *data), void *data);
int named_event_trigger(char *event);	//call all calbacks for this event

#endif
