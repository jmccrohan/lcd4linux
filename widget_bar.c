/* $Id: widget_bar.c,v 1.16 2005/05/08 04:32:45 reinelt Exp $
 *
 * bar widget handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <reinelt@eunet.at>
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
 *
 * $Log: widget_bar.c,v $
 * Revision 1.16  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.15  2005/05/06 06:37:34  reinelt
 * hollow bar patch from geronet
 *
 * Revision 1.14  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.13  2004/11/29 04:42:07  reinelt
 * removed the 99999 msec limit on widget update time (thanks to Petri Damsten)
 *
 * Revision 1.12  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.11  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.10  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.9  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
 * Revision 1.8  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.7  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.6  2004/01/29 04:40:03  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.5  2004/01/23 04:54:00  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.4  2004/01/20 14:25:12  reinelt
 * some reorganization
 * moved drv_generic to drv_generic_serial
 * moved port locking stuff to drv_generic_serial
 *
 * Revision 1.3  2004/01/20 12:45:47  reinelt
 * "Default screen" working with MatrixOrbital
 *
 * Revision 1.2  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 * Revision 1.1  2004/01/18 21:25:16  reinelt
 * Framework for bar widget opened
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
#include "evaluator.h"
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
    RESULT result = { 0, 0, 0, NULL };

    double val1, val2;
    double min, max;

    /* evaluate expressions */
    val1 = 0.0;
    if (Bar->tree1 != NULL) {
	Eval(Bar->tree1, &result);
	val1 = R2N(&result);
	DelResult(&result);
    }

    val2 = val1;
    if (Bar->tree2 != NULL) {
	Eval(Bar->tree2, &result);
	val2 = R2N(&result);
	DelResult(&result);
    }

    /* minimum: if expression is empty, do auto-scaling */
    if (Bar->tree_min != NULL) {
	Eval(Bar->tree_min, &result);
	min = R2N(&result);
	DelResult(&result);
    } else {
	min = Bar->min;
	if (val1 < min)
	    min = val1;
	if (val2 < min)
	    min = val2;
    }

    /* maximum: if expression is empty, do auto-scaling */
    if (Bar->tree_max != NULL) {
	Eval(Bar->tree_max, &result);
	max = R2N(&result);
	DelResult(&result);
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

    /* get raw expressions (we evaluate them ourselves) */
    Bar->expression1 = cfg_get_raw(section, "expression", NULL);
    Bar->expression2 = cfg_get_raw(section, "expression2", NULL);

    /* sanity check */
    if (Bar->expression1 == NULL || *Bar->expression1 == '\0') {
	error("widget %s has no expression, using '0.0'", Self->name);
	Bar->expression1 = "0";
    }

    /* minimum and maximum value */
    Bar->expr_min = cfg_get_raw(section, "min", NULL);
    Bar->expr_max = cfg_get_raw(section, "max", NULL);

    /* compile all expressions */
    Compile(Bar->expression1, &Bar->tree1);
    Compile(Bar->expression2, &Bar->tree2);
    Compile(Bar->expr_min, &Bar->tree_min);
    Compile(Bar->expr_max, &Bar->tree_max);

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
	error("widget %s has unknown direction '%s', using 'East'", Self->name, c);
	Bar->direction = DIR_EAST;
    }
    free(c);

    /* style: none (default), hollow */
    c = cfg_get(section, "style", "0");
    switch (toupper(*c)) {
    case 'H':
	Bar->style = STYLE_HOLLOW;
	if (!(Bar->direction & (DIR_EAST | DIR_WEST))) {
	    error("widget %s with style \"hollow\" not implemented", Self->name);
	    Bar->style = 0;
	}
	break;
    default:
	Bar->style = 0;
    }
    free(c);

    /* update interval (msec), default 1 sec */
    cfg_number(section, "update", 1000, 10, -1, &(Bar->update));

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
	    DelTree(Bar->tree1);
	    DelTree(Bar->tree2);
	    DelTree(Bar->tree_min);
	    DelTree(Bar->tree_max);
	    free(Self->data);
	}
	Self->data = NULL;
    }


    return 0;

}



WIDGET_CLASS Widget_Bar = {
  name:"bar",
  init:widget_bar_init,
  draw:NULL,
  quit:widget_bar_quit,
};
