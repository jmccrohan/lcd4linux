/* $Id$
 * $URL$
 *
 * timer widget handling
 *
 * Copyright (C) 2006 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * WIDGET_CLASS Widget_Timer
 *   the timer widget
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "property.h"
#include "timer.h"
#include "widget.h"
#include "widget_timer.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

void widget_timer_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_TIMER *Timer = W->data;
    int update, active;

    /* evaluate expressions */
    property_eval(&Timer->update);
    property_eval(&Timer->active);

    /* get new update interval */
    update = P2N(&Timer->update);
    if (update < 10)
	update = 10;

    /* finally, fire it! */
    active = P2N(&Timer->active);
    if (active > 0) {
	property_eval(&Timer->expression);
    }

    /* add a new one-shot timer */
    timer_add(widget_timer_update, Self, update, 1);
}



int widget_timer_init(WIDGET * Self)
{
    char *section;
    WIDGET_TIMER *Timer;

    /* prepare config section */
    /* strlen("Widget:")=7 */
    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Timer = malloc(sizeof(WIDGET_TIMER));
    memset(Timer, 0, sizeof(WIDGET_TIMER));

    /* load properties */
    property_load(section, "expression", NULL, &Timer->expression);
    property_load(section, "update", "100", &Timer->update);
    property_load(section, "active", "1", &Timer->active);

    free(section);
    Self->data = Timer;

    /* just do it! */
    widget_timer_update(Self);

    return 0;
}


int widget_timer_quit(WIDGET * Self)
{
    if (Self) {
	/* do not deallocate child widget! */
	if (Self->parent == NULL) {
	    if (Self->data) {
		WIDGET_TIMER *Timer = Self->data;
		property_free(&Timer->expression);
		property_free(&Timer->update);
		property_free(&Timer->active);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }

    return 0;

}


int widget_timer_register(void)
{
    WIDGET_CLASS wc;
    wc = Widget_Timer;
    widget_register(&wc);
    return 0;
}


WIDGET_CLASS Widget_Timer = {
  name:"timer",
  type:WIDGET_TYPE_TIMER,
  init:widget_timer_init,
  draw:NULL,
  quit:widget_timer_quit,
};
