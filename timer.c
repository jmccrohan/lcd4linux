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
 * int timer_add(void (*callback) (void *data), void *data, const int
 *     interval, const int one_shot)
 *
 *   Create a new timer and add it to the timer queue.
 *
 *
 * int timer_add_late(void (*callback) (void *data), void *data, const
 *     int interval, const int one_shot)
 *
 *   This function creates a new timer and adds it to the timer queue
 *   just as timer_add() does, but the timer will NOT be triggered
 *   immediately (useful for scheduling things).
 *
 *
 * int timer_process (struct timespec *delay)
 *
 *   process timer queue
 *
 *
 * int timer_remove(void (*callback) (void *data), void *data)
 *
 *   Remove a new timer with given callback and data.
 *
 *
 * void timer_exit(void)
 *
 *   Release all timers and free the associated memory block.
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


/* number of allocated timer slots */
int nTimers = 0;

/* pointer to memory used for storing the timer slots */
TIMER *Timers = NULL;


static void timer_inc(struct timeval *tv, const int interval)
/*  Update a timer's trigger by adding the given interval.

    tv (timeval pointer): struct holding current time

	interval (integer): interval in milliseconds to be added to the
	timer's trigger

	return value: void */
{
    struct timeval diff = {
	.tv_sec = interval / 1000,
	.tv_usec = (interval % 1000) * 1000
    };

    timeradd(tv, &diff, tv);
}


int timer_remove(void (*callback) (void *data), void *data)
/*  Remove a new timer with given callback and data.

    callback (void pointer): function of type callback(void *data);
    here, it will be used to identify the timer

	data (void pointer): data which will be passed to the callback
	function; here, it will be used to identify the timer

	return value (integer): returns a value of 0 on successful timer
	deletion; otherwise returns a value of -1 */
{
    int i;			/* current timer's ID */

    /* loop through the timer slots and try to find the specified
       timer slot by looking for its settings */
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].callback == callback && Timers[i].data == data && Timers[i].active) {
	    /* we have found the timer slot, so mark it as being inactive;
	       we will not actually delete the timer, so its memory may be
	       re-used */
	    Timers[i].active = 0;
	    return 0;
	}
    }
    /* we have NOT found the timer slot, so signal failure by
       returning a value of -1 */
    return -1;
}


int timer_add(void (*callback) (void *data), void *data, const int interval, const int one_shot)
/*  Create a new timer and add it to the timer queue.

    callback (void pointer): function of type callback(void *data)
	which will be called whenever the timer triggers; this pointer
	will also be used to identify a specific timer

	data (void pointer): data which will be passed to the callback
	function; this pointer will also be used to identify a specific
	timer

	interval (integer): specifies the timer's triggering interval in
	milliseconds

	one_shot (integer): specifies whether the timer should trigger
	indefinitely until it is deleted (value of 0) or only once (all
	other values)

	return value (integer): returns a value of 0 on successful timer
	creation; otherwise returns a value of -1 */
{
    int i;			/* current timer's ID */
    struct timeval now;		/* struct to hold current time */

    /* try to minimize memory usage by looping through the timer slots
       and looking for an inactive timer */
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].active == 0)
	    /* we've just found one, so let's reuse it ("i" holds its ID)
	       by breaking the loop */
	    break;
    }

    /* no inactive timers found, so we have to add a new timer slot */
    if (i >= nTimers) {
	/* increment number of timers and (re-)allocate memory used for
	   storing the timer slots */
	nTimers++;
	Timers = realloc(Timers, nTimers * sizeof(*Timers));

	/* realloc() has failed */
	if (Timers == NULL) {
	    /* restore old number of timers and signal unsuccessful timer
	       creation */
	    nTimers--;
	    return -1;
	}
    }

    /* get current time so the timer triggers immediately */
    gettimeofday(&now, NULL);

    /* fill in timer data */
    Timers[i].callback = callback;
    Timers[i].data = data;
    Timers[i].when = now;
    Timers[i].interval = interval;
    Timers[i].one_shot = one_shot;

    /* set timer to active so that it is processed and not overwritten
       by the memory optimisation above */
    Timers[i].active = 1;

    /* one-shot timers should NOT fire immediately, so delay them by a
       single timer interval */
    if (one_shot) {
	timer_inc(&Timers[i].when, interval);
    }

    /* signal successful timer creation */
    return 0;
}


int timer_add_late(void (*callback) (void *data), void *data, const int interval, const int one_shot)
/*  This function creates a new timer and adds it to the timer queue
	just as timer_add() does, but the timer will NOT be triggered
	immediately (useful for scheduling things).

    callback (void pointer): function of type callback(void *data)
	which will be called whenever the timer triggers; this pointer
	will also be used to identify a specific timer

	data (void pointer): data which will be passed to the callback
	function; this pointer will also be used to identify a specific
	timer

	interval (integer): specifies the timer's triggering interval in
	milliseconds

	one_shot (integer): specifies whether the timer should trigger
	indefinitely until it is deleted (value of 0) or only once (all
	other values)

	return value (integer): returns a value of 0 on successful timer
	creation; otherwise returns a value of -1 */
{
    /* create new timer slot and add it to the timer queue; mask it as
       one-shot timer for now, so the timer will be delayed by a
       single timer interval */
    if (!timer_add(callback, data, interval, 1)) {
	/* signal unsuccessful timer creation */
	return -1;
    }

    int i;			/* current timer's ID */

    /* loop through the timer slots and try to find the new timer slot
       by looking for its settings */
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].callback == callback && Timers[i].data == data && Timers[i].active
	    && Timers[i].interval == interval) {
	    /* we have found the new timer slot, so unmask it by setting
	       its "one_shot" variable to the REAL value; then signal
	       successful timer creation */
	    Timers[i].one_shot = one_shot;
	    return 0;
	}
    }

    /* we have NOT found the new timer slot for some reason, so signal
       failure by returning a value of -1 */
    return -1;
}


int timer_process(struct timespec *delay)
{
    int i, min;
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
		timer_inc(&Timers[i].when, Timers[i].interval);
	    }
	}
    }

    /* find next timer */
    min = -1;
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].active == 0)
	    continue;
	if ((min < 0) || timercmp(&Timers[i].when, &Timers[min].when, <))
	    min = i;
    }

    if (min < 0) {
	error("huh? not one single timer left? dazed and confused...");
	return -1;
    }

    /* update the current moment to compensate for processing delay */
    gettimeofday(&now, NULL);

    /* delay until next timer event */
    struct timeval diff;
    timersub(&Timers[min].when, &now, &diff);

    /* for negative delays, directly trigger next update */
    if (diff.tv_sec < 0)
	timerclear(&diff);

    delay->tv_sec = diff.tv_sec;
    /* microseconds to nanoseconds!! */
    delay->tv_nsec = diff.tv_usec * 1000;

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
/*  Release all timers and free the associated memory block.

	return value: void */
{
    /* reset number of allocated timer slots */
    nTimers = 0;

    if (Timers != NULL) {
	/* free memory used for storing the timer slots */
	free(Timers);
	Timers = NULL;
    }
}
