/* $Id$
 * $URL$
 *
 * GPO widget handling
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
#include "property.h"
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

    /* evaluate properties */
    property_eval(&GPO->expression);
    property_eval(&GPO->update);

    GPO->num = W->row;
    GPO->val = P2N(&GPO->expression);

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

    /* add a new one-shot timer */
    if (P2N(&GPO->update) > 0) {
	timer_add(widget_gpo_update, Self, P2N(&GPO->update), 1);
    }

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

    /* load properties */
    property_load(section, "expression", NULL, &GPO->expression);
    property_load(section, "update", "1000", &GPO->update);

    /* sanity checks */
    if (!property_valid(&GPO->expression)) {
	error("Warning: widget %s has no expression", section);
    }

    free(section);
    Self->data = GPO;

    /* fire it the first time */
    widget_gpo_update(Self);

    return 0;
}


int widget_gpo_quit(WIDGET * Self)
{
    if (Self) {
	if (Self->data) {
	    WIDGET_GPO *GPO = Self->data;
	    property_free(&GPO->expression);
	    property_free(&GPO->update);
	    free(Self->data);
	    Self->data = NULL;
	}
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
