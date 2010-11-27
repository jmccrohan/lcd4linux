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
 * Exported functions:
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
 * int timer_process(struct timespec *delay)
 *
 *    Process timer queue.
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

/* delay in seconds between timer events that is considered as being
   induced by clock skew */
#define CLOCK_SKEW_DETECT_TIME_IN_S 1

/* structure for storing all relevant data of a single timer */
typedef struct TIMER {
    /* function of type callback(void *data) that will be called when
       the timer is processed; it will also be used to identify a
       specific timer */
    void (*callback) (void *data);

    /* data which will be passed to the callback function; it will
       also be used to identify a specific timer */
    void *data;

    /* struct to hold the time (in seconds and milliseconds since the
       Epoch) when the timer will be processed for the next time */
    struct timeval when;

    /* specifies the timer's triggering interval in milliseconds */
    int interval;

    /* specifies whether the timer should trigger indefinitely until
       it is deleted (value of 0) or only once (all other values) */
    int one_shot;

    /* marks timers as being active (so it will get processed) or
       inactive (which means the timer has been deleted and its
       allocated memory may be re-used) */
    int active;
} TIMER;


/* number of allocated timer slots */
int nTimers = 0;

/* pointer to memory used for storing the timer slots */
TIMER *Timers = NULL;


static void timer_inc(struct timeval *tv, const int interval)
/*  Update a timer's trigger by adding the given interval.

    tv (timeval pointer): struct holding the last time the timer has
    been processed

	interval (integer): interval in milliseconds to be added to the
    the last time the timer has been processed

	return value: void
 */
{
    /* split time interval to be added (given in milliseconds) into
       microseconds and seconds */
    struct timeval diff = {
	.tv_sec = interval / 1000,
	.tv_usec = (interval % 1000) * 1000
    };

    /* add interval to timer and store the result in the timer */
    timeradd(tv, &diff, tv);
}


int timer_remove(void (*callback) (void *data), void *data)
/*  Remove a new timer with given callback and data.

    callback (void pointer): function of type void func(void *data);
    here, it will be used to identify the timer

	data (void pointer): data which will be passed to the callback
	function; here, it will be used to identify the timer

	return value (integer): returns a value of 0 on successful timer
	deletion; otherwise returns a value of -1
*/
{
    int i;			/* current timer's ID */

    /* loop through the timer slots and try to find the specified
       timer slot by looking for its settings */
    for (i = 0; i < nTimers; i++) {
	if (Timers[i].callback == callback && Timers[i].data == data && Timers[i].active) {
	    /* we have found the timer slot, so mark it as being inactive;
	       we will not actually delete the timer, so its allocated
	       memory may be re-used */
	    Timers[i].active = 0;

	    /* signal successful timer removal */
	    return 0;
	}
    }
    /* we have NOT found the timer slot, so signal failure by
       returning a value of -1 */
    return -1;
}


int timer_add(void (*callback) (void *data), void *data, const int interval, const int one_shot)
/*  Create a new timer and add it to the timer queue.

    callback (void pointer): function of type void func(void *data)
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
	creation; otherwise returns a value of -1
*/
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

    /* no inactive timers (or none at all) found, so we have to add a
       new timer slot */
    if (i >= nTimers) {
	/* increment number of timers and (re-)allocate memory used for
	   storing the timer slots */
	nTimers++;
	Timers = realloc(Timers, nTimers * sizeof(*Timers));

	/* realloc() has failed */
	if (Timers == NULL) {
	    /* restore old number of timers */
	    nTimers--;

	    /* signal unsuccessful timer creation */
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
       by the memory optimisation routine above */
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

	callback (void pointer): function of type void func(void *data)
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
	creation; otherwise returns a value of -1
*/
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

	    /* signal successful timer creation */
	    return 0;
	}
    }

    /* we have NOT found the new timer slot for some reason, so signal
       failure by returning a value of -1 */
    return -1;
}


int timer_process(struct timespec *delay)
/*  Process timer queue.

	delay (timespec pointer): struct holding delay till the next
	upcoming timer event

	return value (integer): returns a value of 0 when timers have been
	processed successfully; otherwise returns a value of -1
*/
{
    struct timeval now;		/* struct to hold current time */

    /* get current time to check which timers need processing */
    gettimeofday(&now, NULL);

    /* sanity check; at least one timer should need processing */
    if (nTimers == 0) {
	/* otherwise, print an error and return a value of -1 to
	   signal an error */
	error("Huh? Not one single timer to process? Dazed and confused...");
	return -1;
    }

    int i;			/* current timer's ID */

    /* process all expired timers */
    for (i = 0; i < nTimers; i++) {
	/* skip inactive (i.e. deleted) timers */
	if (Timers[i].active == 0)
	    continue;
	/* check whether current timer needs to be processed, i.e. the
	   timer's triggering time is less than or equal to the current
	   time; according to the man page of timercmp(), this avoids
	   using the operators ">=", "<=" and "==" which might be broken
	   on some systems */
	if (!timercmp(&Timers[i].when, &now, >)) {
	    /* if the timer's callback function has been set, call it and
	       pass the corresponding data */
	    if (Timers[i].callback != NULL) {
		Timers[i].callback(Timers[i].data);
	    }

	    /* check for one-shot timers */
	    if (Timers[i].one_shot) {
		/* mark one-shot timer as inactive (which means the timer has
		   been deleted and its allocated memory may be re-used) */
		Timers[i].active = 0;
	    } else {
		/* otherwise, respawn timer by adding one triggering interval
		   to its triggering time */
		timer_inc(&Timers[i].when, Timers[i].interval);
	    }
	}
    }

    int min = -1;		/* ID of the next upcoming timer */

    /* loop through the timer slots and try to find the next upcoming
       timer */
    for (i = 0; i < nTimers; i++) {
	/* skip inactive (i.e. deleted) timers */
	if (Timers[i].active == 0)
	    continue;

	/* if this is the first timer that we check, mark it as the next
	   upcoming timer; otherwise, we'll have nothing to compare
	   against in this loop */
	if (min < 0)
	    min = i;
	/* check whether current timer needs processing prior to the one
	   selected */
	else if (timercmp(&Timers[i].when, &Timers[min].when, <))
	    /* if so, mark it as the next upcoming timer */
	    min = i;
    }

    /* sanity check; we should by now have found the next upcoming
       timer */
    if (min < 0) {
	/* otherwise, print an error and return a value of -1 to signal an
	   error */
	error("Huh? Not one single timer left? Dazed and confused...");
	return -1;
    }

    /* processing all the timers might have taken a while, so update
       the current time to compensate for processing delay */
    gettimeofday(&now, NULL);

    struct timeval diff;	/* struct holding the time difference
				   between current time and the triggering time of the
				   next upcoming timer event */

    /* calculate delay to the next upcoming timer event and store it
       in "diff" */
    timersub(&Timers[min].when, &now, &diff);

    /* for negative delays, set "diff" to the Epoch so the next update
       is triggered immediately */
    if (diff.tv_sec < 0)
	timerclear(&diff);

    /* check whether the delay in "diff" has been induced by clock
       skew */
    if (diff.tv_sec > CLOCK_SKEW_DETECT_TIME_IN_S) {
	/* set "diff" to the Epoch so the next update is triggered
	   directly */
	timerclear(&diff);

	/* display an info message to inform the user */
	info("Oops, clock skewed, update timestamp");

	/* update time stamp and timer */
	/* FIXME: shouldn't we update *all* timers? */
	gettimeofday(&now, NULL);
	Timers[min].when = now;
    }

    /* finally, set passed timespec "delay" to "diff" ... */
    delay->tv_sec = diff.tv_sec;
    /* timespec uses nanoseconds instead of microseconds!!! */
    delay->tv_nsec = diff.tv_usec * 1000;

    /* signal successful timer processing */
    return 0;
}


void timer_exit(void)
/*  Release all timers and free the associated memory block.

	return value: void
*/
{
    /* reset number of allocated timer slots */
    nTimers = 0;

    if (Timers != NULL) {
	/* free memory used for storing the timer slots */
	free(Timers);
	Timers = NULL;
    }
}
