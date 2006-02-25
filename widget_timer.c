/* $Id: widget_timer.c,v 1.2 2006/02/25 13:36:33 geronet Exp $
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
 *
 * $Log: widget_timer.c,v $
 * Revision 1.2  2006/02/25 13:36:33  geronet
 * updated indent.sh, applied coding style
 *
 * Revision 1.1  2006/01/23 06:17:18  reinelt
 * timer widget added
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
#include "evaluator.h"
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
    RESULT result = { 0, 0, 0, NULL };

    /* evaluate expressions */
    Timer->update = 10;
    if (Timer->update_tree != NULL) {
	Eval(Timer->update_tree, &result);
	Timer->update = R2N(&result);
	if (Timer->update < 10)
	    Timer->update = 10;
	DelResult(&result);
    }

    Timer->active = 1;
    if (Timer->active_tree != NULL) {
	Eval(Timer->active_tree, &result);
	Timer->active = R2N(&result);
	if (Timer->active < 0)
	    Timer->active = 0;
	DelResult(&result);
    }

    /* finally, fire it! */
    if (Timer->active) {
	Eval(Timer->expr_tree, &result);
	DelResult(&result);
    }

    /* add a new one-shot timer */
    timer_add(widget_timer_update, Self, Timer->update, 1);
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

    /* get raw expressions (we evaluate them ourselves) */
    Timer->expression = cfg_get_raw(section, "axpression", NULL);
    Timer->update_expr = cfg_get_raw(section, "update", "100");
    Timer->active_expr = cfg_get_raw(section, "active", "1");

    /* sanity checks */
    if (Timer->expression == NULL || *Timer->expression == '\0') {
	error("Timer %s has no expression, using '1'", Self->name);
	Timer->expression = "1";
    }
    if (Timer->update_expr == NULL || *Timer->update_expr == '\0') {
	error("Timer %s has no update, using '100'", Self->name);
	Timer->update_expr = "100";
    }

    /* compile'em */
    Compile(Timer->expression, &Timer->expr_tree);
    Compile(Timer->update_expr, &Timer->update_tree);
    Compile(Timer->active_expr, &Timer->active_tree);


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
		DelTree(Timer->expr_tree);
		DelTree(Timer->update_tree);
		DelTree(Timer->active_tree);
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
