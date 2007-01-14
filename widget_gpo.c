/* $Id$
 * $URL$
 *
 * GPO widget handling
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: widget_gpo.c,v $
 * Revision 1.1  2005/12/18 16:18:36  reinelt
 * GPO's added again
 *
 */

/* 
 * exported functions:
 *
 * WIDGET_CLASS Widget_GPO
 *   the GPO widget
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cfg.h"
#include "evaluator.h"
#include "timer.h"
#include "widget.h"
#include "widget_gpo.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_gpo_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_GPO *GPO = W->data;
    RESULT result = { 0, 0, 0, NULL };

    int val;

    /* evaluate expression */
    val = 0;
    if (GPO->tree != NULL) {
	Eval(GPO->tree, &result);
	val = R2N(&result);
	DelResult(&result);
    }
    GPO->num = W->row;
    GPO->val = val;

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

}


int widget_gpo_init(WIDGET * Self)
{
    char *section;
    WIDGET_GPO *GPO;

    /* prepare config section */
    /* strlen("Widget:")=7 */
    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    GPO = malloc(sizeof(WIDGET_GPO));
    memset(GPO, 0, sizeof(WIDGET_GPO));

    /* get raw expression (we evaluate them ourselves) */
    GPO->expression = cfg_get_raw(section, "expression", NULL);

    /* sanity check */
    if (GPO->expression == NULL || *GPO->expression == '\0') {
	error("widget %s has no expression, using '0.0'", Self->name);
	GPO->expression = "0";
    }

    /* compile expression */
    Compile(GPO->expression, &GPO->tree);

    /* update interval (msec), default 1 sec */
    cfg_number(section, "update", 1000, 10, -1, &(GPO->update));

    free(section);
    Self->data = GPO;

    timer_add(widget_gpo_update, Self, GPO->update, 0);

    return 0;
}


int widget_gpo_quit(WIDGET * Self)
{
    if (Self) {
	if (Self->data) {
	    WIDGET_GPO *GPO = Self->data;
	    DelTree(GPO->tree);
	    free(Self->data);
	}
	Self->data = NULL;
    }
    return 0;
}



WIDGET_CLASS Widget_GPO = {
  name:"gpo",
  type:WIDGET_TYPE_GPO,
  init:widget_gpo_init,
  draw:NULL,
  quit:widget_gpo_quit,
};
