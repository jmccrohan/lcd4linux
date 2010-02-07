/* $Id$
 * $URL$
 *
 * generic grouping of widget timers that have been set to the same
 * update interval, thus allowing synchronized updates
 *
 * Copyright (C) 2010 Martin Zuther <code@mzuther.de>
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
 * exported functions:
 *
 * int timer_add_group(void (*callback) (void *data), const int interval)
 *   make sure that generic timer exists for given update interval
 *
 * int timer_remove_group(void (*callback) (void *data), const int interval)
 *   remove generic timer for given update interval
 *
 * void timer_process_group(void *data)
 *   process all widgets of a given update interval (*data)
 *
 * void timer_exit_group(void)
 *   release all timers and free reserved memory
 *
 * int timer_add_widget(void (*callback) (void *data), void *data, const int interval, const int one_shot)
 *  add widget to group of the given update interval (creates a new group if
 *  necessary)
 *
 * int timer_remove_widget(void (*callback) (void *data), void *data)
 *  remove widget from group of the given update interval (als removes emtpy
 *  groups)
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


/* contains the actual timers, one for each update interval */
typedef struct TIMER_GROUP {
    void (*callback) (void *data);
    int *interval;
    int active;
} TIMER_GROUP;

TIMER_GROUP *GroupTimers = NULL;
int nGroupTimers = 0;

/* contains callback and data for each widget */
typedef struct SINGLE_TIMER {
    void (*callback) (void *data);
    void *data;
    int interval;
    int one_shot;
    int active;
} SINGLE_TIMER;

SINGLE_TIMER *SingleTimers = NULL;
int nSingleTimers = 0;

int timer_add_group(void (*callback) (void *data), const int interval)
{
    int group;

    /* check whether timer exists for given update interval */
    for (group = 0; group < nGroupTimers; group++) {
	if (*GroupTimers[group].interval == interval)
	    return 0;
    }

    /* otherwise look for a free group */
    for (group = 0; group < nGroupTimers; group++) {
	if (GroupTimers[group].active == 0)
	    break;
    }

    /* none found, allocate a new group */
    if (group >= nGroupTimers) {
	nGroupTimers++;
	GroupTimers = realloc(GroupTimers, nGroupTimers * sizeof(*GroupTimers));

	/* also allocate memory for callback data */
	GroupTimers[group].interval = malloc(sizeof(int));
    }

    /* initialize group */
    info("Creating new timer group (%d ms)", interval);

    GroupTimers[group].callback = callback;
    *GroupTimers[group].interval = interval;
    GroupTimers[group].active = 1;

    /* finally, request a generic timer */
    return timer_add(GroupTimers[group].callback, GroupTimers[group].interval, interval, 0);
}

int timer_remove_group(void (*callback) (void *data), const int interval)
{
    int group;

    /* look for group with given callback function and update interval */
    for (group = 0; group < nGroupTimers; group++) {
	if (GroupTimers[group].callback == callback && *GroupTimers[group].interval == interval
	    && GroupTimers[group].active) {
	    /* inactivate group */
	    GroupTimers[group].active = 0;

	    /* remove generic timer */
	    return timer_remove(GroupTimers[group].callback, GroupTimers[group].interval);
	}
    }
    return -1;
}

void timer_process_group(void *data)
{
    int slot;
    int *interval = (int *) data;

    /* sanity check */
    if (nSingleTimers == 0) {
	error("huh? not even a single widget timer to process? dazed and confused...");
	return;
    }

    /* process every (active) widget associated with given update interval */
    for (slot = 0; slot < nSingleTimers; slot++) {
	if (SingleTimers[slot].active == 0)
	    continue;

	if (SingleTimers[slot].interval == *interval) {
	    /* call one-shot timers only once */
	    if (SingleTimers[slot].one_shot)
		SingleTimers[slot].active = 0;
	    /* execute callback function */
	    if (SingleTimers[slot].callback != NULL)
		SingleTimers[slot].callback(SingleTimers[slot].data);
	}
    }
}

int timer_add_widget(void (*callback) (void *data), void *data, const int interval, const int one_shot)
{
    int slot;

    /* make sure that group for update interval exists or can be created */
    if (timer_add_group(timer_process_group, interval) != 0)
	return -1;

    /* find a free slot for callback data */
    for (slot = 0; slot < nSingleTimers; slot++) {
	if (SingleTimers[slot].active == 0)
	    break;
    }

    /* none found, allocate a new slot */
    if (slot >= nSingleTimers) {
	nSingleTimers++;
	SingleTimers = realloc(SingleTimers, nSingleTimers * sizeof(*SingleTimers));
    }

    /* fill slot with callback data */
    SingleTimers[slot].callback = callback;
    SingleTimers[slot].data = data;
    SingleTimers[slot].interval = interval;
    SingleTimers[slot].one_shot = one_shot;
    SingleTimers[slot].active = 1;

    return 0;
}

int timer_remove_widget(void (*callback) (void *data), void *data)
{
    int slot, interval;

    interval = 0;
    /* look for (active) widget with given callback function and data */
    for (slot = 0; slot < nSingleTimers; slot++) {
	if (SingleTimers[slot].callback == callback && SingleTimers[slot].data == data && SingleTimers[slot].active) {
	    /* deactivate widget */
	    SingleTimers[slot].active = 0;
	    interval = SingleTimers[slot].interval;
	    break;
	}
    }

    /* signal an error if no matching widget was found */
    if (interval == 0)
	return -1;

    /* look for other widgets with given update interval */
    for (slot = 0; slot < nSingleTimers; slot++) {
	if (SingleTimers[slot].active == 0)
	    continue;
	/* at least one other widget with given update interval exists */
	if (SingleTimers[slot].interval == interval)
	    return 0;
    }

    /* remove group with given update interval */
    return timer_remove_group(timer_process_group, interval);
}

void timer_exit_group(void)
{
    int group;

    /* remove generic timer and free memory for callback data */
    for (group = 0; group < nGroupTimers; group++) {
	timer_remove(GroupTimers[group].callback, GroupTimers[group].interval);
	free(GroupTimers[group].interval);
    }

    /* free memory for groups */
    nGroupTimers = 0;

    if (GroupTimers != NULL) {
	free(GroupTimers);;
	GroupTimers = NULL;
    }

    /* free memory for widget callback data */
    nSingleTimers = 0;

    if (SingleTimers != NULL) {
	free(SingleTimers);;
	SingleTimers = NULL;
    }
}
