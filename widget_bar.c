/* $Id$
 * $URL$
 *
 * bar widget handling
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
 * WIDGET_CLASS Widget_Bar
 *   the bar widget
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
#include "widget_bar.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_bar_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_BAR *Bar = W->data;

    double val1, val2;
    double min, max;

    /* evaluate properties */
    property_eval(&Bar->expression1);
    val1 = P2N(&Bar->expression1);

    if (property_valid(&Bar->expression2)) {
	property_eval(&Bar->expression2);
	val2 = P2N(&Bar->expression2);
    } else {
	val2 = val1;
    }

    /* minimum: if expression is empty, do auto-scaling */
    if (property_valid(&Bar->expr_min)) {
	property_eval(&Bar->expr_min);
	min = P2N(&Bar->expr_min);
    } else {
	min = Bar->min;
	if (val1 < min)
	    min = val1;
	if (val2 < min)
	    min = val2;
    }

    /* maximum: if expression is empty, do auto-scaling */
    if (property_valid(&Bar->expr_max)) {
	property_eval(&Bar->expr_max);
	max = P2N(&Bar->expr_max);
    } else {
	max = Bar->max;
	if (val1 > max)
	    max = val1;
	if (val2 > max)
	    max = val2;
    }

    /* calculate bar values */
    Bar->min = min;
    Bar->max = max;
    if (max > min) {
	Bar->val1 = (val1 - min) / (max - min);
	Bar->val2 = (val2 - min) / (max - min);
    } else {
	Bar->val1 = 0.0;
	Bar->val2 = 0.0;
    }

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

}


int widget_bar_init(WIDGET * Self)
{
    char *section;
    char *c;
    WIDGET_BAR *Bar;

    /* prepare config section */
    /* strlen("Widget:")=7 */
    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Bar = malloc(sizeof(WIDGET_BAR));
    memset(Bar, 0, sizeof(WIDGET_BAR));

    /* load properties */
    property_load(section, "expression", NULL, &Bar->expression1);
    property_load(section, "expression2", NULL, &Bar->expression2);
    property_load(section, "min", NULL, &Bar->expr_min);
    property_load(section, "max", NULL, &Bar->expr_max);

    /* sanity checks */
    if (!property_valid(&Bar->expression1)) {
	error("Warning: widget %s has no expression", section);
    }

    /* bar length, default 1 */
    cfg_number(section, "length", 1, 0, -1, &(Bar->length));

    /* direction: East (default), West, North, South */
    c = cfg_get(section, "direction", "E");
    switch (toupper(*c)) {
    case 'E':
	Bar->direction = DIR_EAST;
	break;
    case 'W':
	Bar->direction = DIR_WEST;
	break;
    case 'N':
	Bar->direction = DIR_NORTH;
	break;
    case 'S':
	Bar->direction = DIR_SOUTH;
	break;
    default:
	error("widget %s has unknown direction '%s'; known directions: 'E', 'W', 'N', 'S'; using 'E(ast)'", Self->name,
	      c);
	Bar->direction = DIR_EAST;
    }
    free(c);

    /* style: none (default), hollow */
    c = cfg_get(section, "style", "0");
    switch (toupper(*c)) {
    case 'H':
	Bar->style = STYLE_HOLLOW;
	if (!(Bar->direction & (DIR_EAST | DIR_WEST))) {
	    error("widget %s with style \"hollow\" not implemented for other directions than E(ast) or W(est)",
		  Self->name);
	    Bar->style = 0;
	}
	break;
    case '0':
	Bar->style = 0;
	break;
    default:
	error("widget %s has unknown style '%s'; known styles: '0' or 'H'; using '0'", Self->name, c);
	Bar->style = 0;
    }
    free(c);

    /* update interval (msec), default 1 sec */
    cfg_number(section, "update", 1000, 10, -1, &(Bar->update));

    /* get widget special colors */
    Bar->color_valid[0] = widget_color(section, Self->name, "barcolor0", &Bar->color[0]);
    Bar->color_valid[1] = widget_color(section, Self->name, "barcolor1", &Bar->color[1]);

    free(section);
    Self->data = Bar;

    timer_add(widget_bar_update, Self, Bar->update, 0);

    return 0;
}


int widget_bar_quit(WIDGET * Self)
{
    if (Self) {
	if (Self->data) {
	    WIDGET_BAR *Bar = Self->data;
	    property_free(&Bar->expression1);
	    property_free(&Bar->expression2);
	    property_free(&Bar->expr_min);
	    property_free(&Bar->expr_max);
	    free(Self->data);
	}
	Self->data = NULL;
    }


    return 0;

}



WIDGET_CLASS Widget_Bar = {
    .name = "bar",
    .type = WIDGET_TYPE_RC,
    .init = widget_bar_init,
    .draw = NULL,
    .quit = widget_bar_quit,
};
