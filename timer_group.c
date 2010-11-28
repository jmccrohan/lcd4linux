/* $Id$
 * $URL$
 *
 * Generic grouping of widgets that have been set to the same update
 * interval, thus allowing synchronized updates.
 *
 * Copyright (C) 2010 Martin Zuther <code@mzuther.de>
 * Copyright (C) 2010 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Based on "timer.c" which is
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
 * void timer_process_group(void *data)
 *
 *  Process all widgets of a timer group; if the timer group only
 *  contains one-shot timers, it will be deleted after processing.
 *
 *
 * void timer_exit_group(void)
 *
 *   Release all timer groups and widgets and free the associated
 *   memory blocks.
 *
 *
 * int timer_add_widget(void (*callback) (void *data), void *data,
 *     const int interval, const int one_shot)
 *
 *   Add widget to timer group of the specified update interval
 *   (also creates a new timer group if necessary).
 *
 *
 * int timer_remove_widget(void (*callback) (void *data), void *data)
 *
 *   Remove widget from the timer group with the specified update
 *   interval (also removes corresponding timer group if empty).
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "timer.h"
#include "timer_group.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* structure for storing all relevant data of a single timer group */
typedef struct TIMER_GROUP {
    /* pointer to the group's triggering interval in milliseconds;
       this will be used to identify a specific timer group and also
       as callback data for the underlying generic timer */
    int *interval;

    /* marks timer group as being active (so it will get processed) or
       inactive (which means the timer group has been deleted and its
       allocated memory may be re-used) */
    int active;
} TIMER_GROUP;

/* number of allocated timer group slots */
int nTimerGroups = 0;

/* pointer to memory allocated for storing the timer group slots */
TIMER_GROUP *TimerGroups = NULL;


/* structure for storing all relevant timer data of a single widget */
typedef struct TIMER_GROUP_WIDGET {
    /* pointer to function of type void func(void *data) that will be
       called when the timer is processed; it will also be used to
       identify a specific widget */
    void (*callback) (void *data);

    /* pointer to data which will be passed to the callback function;
       it will also be used to identify a specific widget */
    void *data;

    /* specifies the timer's triggering interval in milliseconds; it
       will also be used to identify a specific widget */
    int interval;

    /* specifies whether the timer should trigger indefinitely until
       it is deleted (value of 0) or only once (all other values) */
    int one_shot;

    /* marks timer as being active (so it will get processed) or
       inactive (which means the timer has been deleted and its
       allocated memory may be re-used) */
    int active;
} TIMER_GROUP_WIDGET;

/* number of allocated widget slots */
int nTimerGroupWidgets = 0;

/* pointer to memory allocated for storing the widget slots */
TIMER_GROUP_WIDGET *TimerGroupWidgets = NULL;


int timer_group_exists(const int interval)
/*  Check whether a timer group for the specified interval exists.

    interval (integer): the sought-after triggering interval in
    milliseconds

	return value (integer): returns a value of 1 if timer group
	exists; otherwise returns a value of 0
*/
{
    int group;			/* current timer group's ID */

    /* loop through the timer group slots to search for one that
       matches the specified interval */
    for (group = 0; group < nTimerGroups; group++) {
	/* skip inactive (i.e. deleted) timer groups */
	if (TimerGroups[group].active == 0)
	    continue;

	if (*TimerGroups[group].interval == interval) {
	    /* matching timer group found, so signal success by returning
	       a value of 1 */
	    return 1;
	}
    }

    /* matching timer group not found, so signal failure by returning
       a value of 0 */
    return 0;
}


int timer_add_group(const int interval)
/*  Create a new timer group (unless it already exists) and link it to
	the timer queue.

	interval (integer): the new timer group's triggering interval in
	milliseconds

	return value (integer): returns a value of 0 on successful timer
	group creation; otherwise returns a value of -1
*/
{
    /* if timer group for update interval already exists, signal
       success by returning a value of 0 */
    if (timer_group_exists(interval))
	return 0;

    /* display an info message to inform the user that a new timer
       group is being created */
    info("Creating new timer group (%d ms)", interval);

    int group;			/* current timer group's ID */

    /* try to minimize memory usage by looping through timer group
       slots and looking for an inactive timer group */
    for (group = 0; group < nTimerGroups; group++) {
	if (TimerGroups[group].active == 0) {
	    /* we've just found one, so let's reuse it ("group" holds its
	       ID) by breaking the loop */
	    break;
	}
    }

    /* no inactive timer groups (or none at all) found, so we have to
       add a new timer group slot */
    if (group >= nTimerGroups) {
	/* increment number of timer groups and (re-)allocate memory used
	   for storing the timer group slots */
	nTimerGroups++;
	TimerGroups = realloc(TimerGroups, nTimerGroups * sizeof(*TimerGroups));

	/* make sure "group" points to valid memory */
	group = nTimerGroups - 1;

	/* realloc() has failed */
	if (TimerGroups == NULL) {
	    /* restore old number of timer groups */
	    nTimerGroups--;

	    /* signal unsuccessful timer group creation */
	    return -1;
	}

	/* allocate memory for the underlying generic timer's callback
	   data (i.e. the group's triggering interval in milliseconds) */
	TimerGroups[group].interval = malloc(sizeof(int));

	/* malloc() has failed */
	if (TimerGroups[group].interval == NULL) {
	    /* signal unsuccessful timer group creation */
	    return -1;
	}
    }

    /* initialize timer group's interval */
    *TimerGroups[group].interval = interval;

    /* set timer group to active so that it is processed and not
       overwritten by the memory optimization routine above */
    TimerGroups[group].active = 1;

    /* finally, request a generic timer that calls this group and
       signal success or failure */
    return timer_add(timer_process_group, TimerGroups[group].interval, interval, 0);
}


int timer_remove_group(const int interval)
/*  Remove a timer group and unlink it from the timer queue (also
	removes all remaining widget slots in this timer group).

	interval (integer): triggering interval in milliseconds; here, it
    will be used to identify the timer group

	return value (integer): returns a value of 0 on successful timer
	group removal; otherwise returns a value of -1
*/
{
    /* display an info message to inform the user that a timer group
       is being removed */
    info("Removing timer group (%d ms)", interval);

    int group;			/* current timer group's ID */
    int widget;			/* current widget's ID */

    /* loop through the widget slots to look for remaining widgets
       with the specified update interval */
    for (widget = 0; widget < nTimerGroupWidgets; widget++) {
	/* skip inactive (i.e. deleted) widget slots */
	if (TimerGroupWidgets[widget].active == 0)
	    continue;

	if (TimerGroupWidgets[widget].interval == interval) {
	    /* we have found a matching widget slot, so mark it as being
	       inactive; we will not actually delete the slot, so its
	       allocated memory may be re-used */
	    TimerGroupWidgets[widget].active = 0;
	}
    }

    /* loop through timer group slots and try to find the specified
       timer group slot by looking for its settings */
    for (group = 0; group < nTimerGroups; group++) {
	/* skip inactive (i.e. deleted) timer groups */
	if (TimerGroups[group].active == 0)
	    continue;

	if (*TimerGroups[group].interval == interval) {
	    /* we have found the timer group slot, so mark it as being
	       inactive; we will not actually delete the slot, so its
	       allocated memory may be re-used */
	    TimerGroups[group].active = 0;

	    /* remove the generic timer that calls this group */
	    if (timer_remove(timer_process_group, TimerGroups[group].interval)) {
		/* signal successful removal of timer group */
		return 0;
	    } else {
		/* an error occurred on generic timer removal, so signal
		   failure by returning a value of -1 */
		return -1;
	    }
	}
    }

    /* we have NOT found the timer group slot, so signal failure by
       returning a value of -1 */
    return -1;
}


int timer_remove_empty_group(const int interval)
/*  Remove timer group *only* if it contains no more widget slots.

	interval (integer): triggering interval in milliseconds; here, it
    will be used to identify the timer group

	return value (integer): returns a value of 0 on successful
	processing; otherwise returns a value of -1
*/
{
    int widget;			/* current widget's ID */

    /* loop through the widget slots to look for widgets with the
       specified update interval */
    for (widget = 0; widget < nTimerGroupWidgets; widget++) {
	/* skip inactive (i.e. deleted) widget slots */
	if (TimerGroupWidgets[widget].active == 0)
	    continue;

	/* at least one other widget with specified update interval
	   exists, so signal success by returning a value of 0 */
	if (TimerGroupWidgets[widget].interval == interval)
	    return 0;
    }

    /* no other widgets with specified update interval exist, so
       remove corresponding timer group and signal success or
       failure */
    return timer_remove_group(interval);
}


void timer_process_group(void *data)
/*  Process all widgets of a timer group; if the timer group only
	contains one-shot timers, it will be deleted after processing.

	data (void pointer): points to an integer holding the triggering
    interval in milliseconds; here, it will be used to identify the
    timer group

	return value: void
*/
{
    int widget;			/* current widget's ID */

    /* convert callback data to integer (triggering interval in
       milliseconds) */
    int interval = *((int *) data);

    /* sanity check; by now, at least one timer group should be
       instantiated */
    if (nTimerGroups <= 0) {
	/* otherwise, print an error and return early */
	error("Huh? Not even a single timer group to process? Dazed and confused...");
	return;
    }

    /* sanity check; by now, at least one widget slot should be
       instantiated */
    if (nTimerGroupWidgets <= 0) {
	/* otherwise, print an error and return early */
	error("Huh? Not even a single widget slot to process? Dazed and confused...");
	return;
    }

    /* loop through widgets and search for those matching the timer
       group's update interval */
    for (widget = 0; widget < nTimerGroupWidgets; widget++) {
	/* skip inactive (i.e. deleted) widgets */
	if (TimerGroupWidgets[widget].active == 0)
	    continue;

	/* the current widget belongs to the specified timer group */
	if (TimerGroupWidgets[widget].interval == interval) {
	    /* if the widget's callback function has been set, call it and
	       pass the corresponding data */
	    if (TimerGroupWidgets[widget].callback != NULL)
		TimerGroupWidgets[widget].callback(TimerGroupWidgets[widget].data);

	    /* mark one-shot widget as inactive (which means the it has
	       been deleted and its allocated memory may be re-used) */
	    if (TimerGroupWidgets[widget].one_shot) {
		TimerGroupWidgets[widget].active = 0;

		/* also remove the corresponding timer group if it is empty */
		timer_remove_empty_group(interval);
	    }
	}
    }
}


int timer_add_widget(void (*callback) (void *data), void *data, const int interval, const int one_shot)
/*  Add widget to timer group of the specified update interval
    (also creates a new timer group if necessary).

    callback (void pointer): function of type void func(void *data)
	which will be called whenever the timer group triggers; this
	pointer will also be used to identify a specific widget

	data (void pointer): data which will be passed to the callback
	function; this pointer will also be used to identify a specific
	widget

	interval (integer): specifies the timer's triggering interval in
	milliseconds

	one_shot (integer): specifies whether the timer should trigger
	indefinitely until it is deleted (value of 0) or only once (all
	other values)

	return value (integer): returns a value of 0 on successful widget
	addition; otherwise returns a value of -1
*/
{
    int widget;			/* current widget's ID */

    /* if no timer group for update interval exists, create one */
    if (!timer_group_exists(interval)) {
	/* creation of new timer group failed, so signal failure by
	   returning a value of -1 */
	if (timer_add_group(interval) != 0)
	    return -1;
    }

    /* try to minimize memory usage by looping through the widget
       slots and looking for an inactive widget slot */
    for (widget = 0; widget < nTimerGroupWidgets; widget++) {
	if (TimerGroupWidgets[widget].active == 0) {
	    /* we've just found one, so let's reuse it ("widget" holds its
	       ID) by breaking the loop */
	    break;
	}
    }

    /* no inactive widget slots (or none at all) found, so we have to
       add a new widget slot */
    if (widget >= nTimerGroupWidgets) {
	/* increment number of widget slots and (re-)allocate memory used
	   for storing the widget slots */
	nTimerGroupWidgets++;
	TimerGroupWidgets = realloc(TimerGroupWidgets, nTimerGroupWidgets * sizeof(*TimerGroupWidgets));

	/* make sure "widget" points to valid memory */
	widget = nTimerGroupWidgets - 1;

	/* realloc() has failed */
	if (TimerGroupWidgets == NULL) {
	    /* restore old number of widget slots */
	    nTimerGroupWidgets--;

	    /* signal unsuccessful creation of widget slot */
	    return -1;
	}
    }

    /* initialize widget slot */
    TimerGroupWidgets[widget].callback = callback;
    TimerGroupWidgets[widget].data = data;
    TimerGroupWidgets[widget].interval = interval;
    TimerGroupWidgets[widget].one_shot = one_shot;

    /* set widget slot to active so that it is processed and not
       overwritten by the memory optimization routine above */
    TimerGroupWidgets[widget].active = 1;

    /* signal successful addition of widget slot */
    return 0;
}


int timer_remove_widget(void (*callback) (void *data), void *data)
/*  Remove widget from the timer group with the specified update
    interval (also removes corresponding timer group if empty).

    callback (void pointer): function of type void func(void *data);
	here, it will be used to identify a specific widget

	data (void pointer): data which will be passed to the callback
	function; here, it will be used to identify a specific widget

	return value (integer): returns a value of 0 on successful widget
	removal; otherwise returns a value of -1
*/
{
    int widget;			/* current widget's ID */
    int interval = -1;		/* specified widget's triggering interval in
				   milliseconds */

    /* loop through the widget slots and try to find the specified
       widget slot by looking for its settings */
    for (widget = 0; widget < nTimerGroupWidgets; widget++) {
	/* skip inactive (i.e. deleted) widget slots */
	if (TimerGroupWidgets[widget].active == 0)
	    continue;

	if (TimerGroupWidgets[widget].callback == callback && TimerGroupWidgets[widget].data == data) {
	    /* we have found the widget slot, so mark it as being
	       inactive; we will not actually delete the slot, so its
	       allocated memory may be re-used */
	    TimerGroupWidgets[widget].active = 0;

	    /* store the widget's triggering interval for later use and
	       break the loop */
	    interval = TimerGroupWidgets[widget].interval;
	    break;
	}
    }

    /* if no matching widget was found, signal an error by returning
       a value of -1 */
    if (interval < 0)
	return -1;

    /* if no other widgets with specified update interval exist,
       remove corresponding timer group and signal success or
       failure */
    return timer_remove_empty_group(interval);
}


void timer_exit_group(void)
/*  Release all timer groups and widgets and free the associated
	memory blocks.

	return value: void
*/
{
    int group;			/* current timer group's ID */

    /* loop through all timer groups and remove them one by one */
    for (group = 0; group < nTimerGroups; group++) {
	/* remove generic timer */
	timer_remove(timer_process_group, TimerGroups[group].interval);

	/* free memory allocated for callback data (i.e. the group's
	   triggering interval in milliseconds) */
	free(TimerGroups[group].interval);
    }

    /* reset number of allocated timer groups */
    nTimerGroups = 0;

    /* free allocated memory containing the timer group slots */
    if (TimerGroups != NULL) {
	free(TimerGroups);
	TimerGroups = NULL;
    }

    /* reset number of allocated widget slots */
    nTimerGroupWidgets = 0;

    /* free allocated memory containing the widget slots */
    if (TimerGroupWidgets != NULL) {
	free(TimerGroupWidgets);
	TimerGroupWidgets = NULL;
    }
}
