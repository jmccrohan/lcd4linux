/* $Id$
 * $URL$
 *
 * generic timer handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * int timer_add (void(*callback)(void *data), void *data, int interval, int one_shot);
 *   adds a timer to the queue
 *
 * int timer_process (struct timespec *delay);
 *   process timer queue
 *
 * int timer_remove(void (*callback) (void *data), void *data);
 *   remove a timer with given callback and data
 *
 * int timer_add_late(void (*callback) (void *data), void *data, const int interval, const int one_shot)
 *   same as timer_add, but the one shot does not fire now (useful for scheduling things)
 *
 * void timer_exit();
 *   release all timers
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "debug.h"
#include "cfg.h"
#include "timer.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define CLOCK_SKEW_DETECT_TIME_IN_S 1

typedef struct TIMER {
    void (*callback) (void *data);
    void *data;
    struct timeval when;
    int interval;
    int one_shot;
    int active;
} TIMER;


TIMER *Timers = NULL;
int nTimers = 0;


static void timer_inc(struct timeval *tv, const int msec)
{
    tv->tv_sec += msec / 1000;
    tv->tv_usec += (msec - 1000 * (msec / 1000)) * 1000;

    if (tv->tv_usec >= 1000000) {
	tv->tv_usec -= 1000000;
	tv->tv_sec++;
    }
}

int timer_remove(void (*callback) (void *data), void *data)
{
    int i;
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].callback == callback && Timers[i].data == data && Timers[i].active) {
	    Timers[i].active = 0;
	    return 0;
	}
    }
    return -1;
}

int timer_add_late(void (*callback) (void *data), void *data, const int interval, const int one_shot)
{
    if (!timer_add(callback, data, interval, 1)) {
	return -1;
    }
    if (one_shot) {
	return 0;
    }
    int i;
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].callback == callback && Timers[i].data == data && Timers[i].active
	    && Timers[i].interval == interval && Timers[i].one_shot) {
	    //we forced it to one_shot when adding to make it late (which gives us the alternate behavior)
	    Timers[i].one_shot = one_shot;
	    return 0;
	}
    }

    return -1;
}

int timer_add(void (*callback) (void *data), void *data, const int interval, const int one_shot)
{
    int i;
    struct timeval now;

    /* find a free slot */
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].active == 0)
	    break;
    }

    /* none found, allocate a new slot */
    if (i >= nTimers) {
	nTimers++;
	Timers = realloc(Timers, nTimers * sizeof(*Timers));
    }

    gettimeofday(&now, NULL);

    /* fill slot */
    Timers[i].callback = callback;
    Timers[i].data = data;
    Timers[i].when = now;
    Timers[i].interval = interval;
    Timers[i].one_shot = one_shot;
    Timers[i].active = 1;

    /* if one-shot timer, don't fire now */
    if (one_shot) {
	timer_inc(&Timers[i].when, interval);
    }

    return 0;
}


int timer_process(struct timespec *delay)
{
    int i, flag, min;
    struct timeval now;

    /* the current moment */
    gettimeofday(&now, NULL);

    /* sanity check */
    if (nTimers == 0) {
	error("huh? not one single timer to process? dazed and confused...");
	return -1;
    }

    /* process expired timers */
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].active == 0)
	    continue;
	if (!timercmp(&Timers[i].when, &now, >)) {
	    /* callback */
	    if (Timers[i].callback != NULL) {
		Timers[i].callback(Timers[i].data);
	    }
	    /* respawn or delete timer */
	    if (Timers[i].one_shot) {
		Timers[i].active = 0;
	    } else {
		Timers[i].when = now;
		timer_inc(&Timers[i].when, Timers[i].interval);
	    }
	}
    }

    /* find next timer */
    flag = 1;
    min = -1;
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].active == 0)
	    continue;
	if (flag || timercmp(&Timers[i].when, &Timers[min].when, <)) {
	    flag = 0;
	    min = i;
	}
    }

    if (min < 0) {
	error("huh? not one single timer left? dazed and confused...");
	return -1;
    }

    /* delay until next timer event */
    delay->tv_sec = Timers[min].when.tv_sec - now.tv_sec;
    delay->tv_nsec = Timers[min].when.tv_usec - now.tv_usec;
    if (delay->tv_nsec < 0) {
	delay->tv_sec--;
	delay->tv_nsec += 1000000;
    }
    /* nanoseconds!! */
    delay->tv_nsec *= 1000;

    /* check if date changed */
    if ((delay->tv_sec) > CLOCK_SKEW_DETECT_TIME_IN_S) {
	delay->tv_sec = 0;
	delay->tv_nsec = 0;
	info("Oops, clock skewed, update timestamp");
	gettimeofday(&now, NULL);
	Timers[min].when = now;
    }

    return 0;

}


void timer_exit(void)
{
    nTimers = 0;

    if (Timers != NULL) {
	free(Timers);;
	Timers = NULL;
    }
}
