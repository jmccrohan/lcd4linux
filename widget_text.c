/* $Id: widget_text.c,v 1.27 2006/08/13 18:14:03 harbaum Exp $
 *
 * simple text widget handling
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
 * $Log: widget_text.c,v $
 * Revision 1.27  2006/08/13 18:14:03  harbaum
 * Added KVV plugin
 *
 * Revision 1.26  2006/08/13 11:38:20  reinelt
 * text widget uses dynamic properties
 *
 * Revision 1.25  2006/08/13 09:53:10  reinelt
 * dynamic properties added (used by 'style' of text widget)
 *
 * Revision 1.24  2006/01/23 06:17:18  reinelt
 * timer widget added
 *
 * Revision 1.23  2005/12/18 16:18:36  reinelt
 * GPO's added again
 *
 * Revision 1.22  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.21  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.20  2004/11/29 04:42:07  reinelt
 * removed the 99999 msec limit on widget update time (thanks to Petri Damsten)
 *
 * Revision 1.19  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.18  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.17  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.16  2004/03/08 16:26:26  reinelt
 * re-introduced \nnn (octal) characters in strings
 * text widgets can have a 'update' speed of 0 which means 'never'
 * (may be used for static content)
 *
 * Revision 1.15  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
 * Revision 1.14  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.13  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.12  2004/02/09 19:49:38  nicowallmeier
 * Minor bugfix
 *
 * Revision 1.11  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.10  2004/01/29 04:40:03  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.9  2004/01/23 04:54:04  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.8  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.7  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 * Revision 1.6  2004/01/15 07:47:02  reinelt
 * debian/ postinst and watch added (did CVS forget about them?)
 * evaluator: conditional expressions (a?b:c) added
 * text widget nearly finished
 *
 * Revision 1.5  2004/01/15 04:29:45  reinelt
 * moved lcd4linux.conf.sample to *.old
 * lcd4linux.conf.sample with new layout
 * new plugins 'loadavg' and 'meminfo'
 * text widget have pre- and postfix
 *
 * Revision 1.4  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.3  2004/01/13 08:18:20  reinelt
 * timer queues added
 * liblcd4linux deactivated turing transformation to new layout
 *
 * Revision 1.2  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.1  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 */

/* 
 * exported functions:
 *
 * WIDGET_CLASS Widget_Text
 *   a simple text widget which 
 *   must be supported by all displays
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
#include "property.h"
#include "timer.h"
#include "widget.h"
#include "widget_text.h"
#
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_text_scroll(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_TEXT *T = W->data;

    char *prefix = P2S(&T->prefix);
    char *postfix = P2S(&T->postfix);

    char *string = T->string;

    int num, len, width, pad;
    char *src, *dst;

    num = 0;
    len = strlen(string);
    width = T->width - strlen(prefix) - strlen(postfix);
    if (width < 0)
	width = 0;

    switch (T->align) {
    case ALIGN_LEFT:
	pad = 0;
	break;
    case ALIGN_CENTER:
	pad = (width - len) / 2;
	if (pad < 0)
	    pad = 0;
	break;
    case ALIGN_RIGHT:
	pad = width - len;
	if (pad < 0)
	    pad = 0;
	break;
    case ALIGN_MARQUEE:
	pad = width - T->scroll;
	T->scroll++;
	if (T->scroll >= width + len)
	    T->scroll = 0;
	break;
    default:			/* not reached  */
	pad = 0;
    }

    dst = T->buffer;

    /* process prefix */
    src = prefix;
    while (num < T->width) {
	if (*src == '\0')
	    break;
	*(dst++) = *(src++);
	num++;
    }

    src = string;

    /* pad blanks on the beginning */
    while (pad > 0 && num < T->width) {
	*(dst++) = ' ';
	num++;
	pad--;
    }

    /* skip src chars (marquee) */
    while (pad < 0) {
	src++;
	pad++;
    }

    /* copy content */
    while (num < T->width) {
	if (*src == '\0')
	    break;
	*(dst++) = *(src++);
	num++;
    }

    /* pad blanks on the end */
    src = postfix;
    len = strlen(src);
    while (num < T->width - len) {
	*(dst++) = ' ';
	num++;
    }

    /* process postfix */
    while (num < T->width) {
	if (*src == '\0')
	    break;
	*(dst++) = *(src++);
	num++;
    }

    *dst = '\0';

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);
}



void widget_text_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_TEXT *T = W->data;
    char *string;
    int update = 0;

    /* evaluate prefix and postfix */
    update += property_eval(&T->prefix);
    update += property_eval(&T->postfix);

    /* evaluate value */
    property_eval(&T->value);

    /* string or number? */
    if (T->precision == 0xDEAD) {
	string = strdup(P2S(&T->value));
    } else {
	double number = P2N(&T->value);
	int width = T->width - strlen(P2S(&T->prefix)) - strlen(P2S(&T->postfix));
	int precision = T->precision;
	/* print zero bytes so we can specify NULL as target  */
	/* and get the length of the resulting string */
	int size = snprintf(NULL, 0, "%.*f", precision, number);
	/* number does not fit into field width: try to reduce precision */
	if (width < 0)
	    width = 0;
	if (size > width && precision > 0) {
	    int delta = size - width;
	    if (delta > precision)
		delta = precision;
	    precision -= delta;
	    size -= delta;
	    /* zero precision: omit decimal point, too */
	    if (precision == 0)
		size--;
	}
	/* number still doesn't fit: display '*****'  */
	if (size > width) {
	    string = malloc(width + 1);
	    memset(string, '*', width);
	    *(string + width) = '\0';
	} else {
	    string = malloc(size + 1);
	    snprintf(string, size + 1, "%.*f", precision, number);
	}
    }

    /* did the formatted string change? */
    if (T->string == NULL || strcmp(T->string, string) != 0) {
	update++;
	if (T->string)
	    free(T->string);
	T->string = string;
    } else {
	free(string);
    }

    /* text style */
    update += property_eval(&T->style);

    /* something has changed and should be updated */
    if (update) {
	/* reset marquee counter if content has changed */
	T->scroll = 0;
	/* if there's a marquee scroller active, it has its own */
	/* update callback timer, so we do nothing here; otherwise */
	/* we simply call this scroll callback directly */
	if (T->align != ALIGN_MARQUEE) {
	    widget_text_scroll(Self);
	}
    }

}


int widget_text_init(WIDGET * Self)
{
    char *section;
    char *c;
    WIDGET_TEXT *Text;

    /* prepare config section */
    /* strlen("Widget:")=7 */
    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    Text = malloc(sizeof(WIDGET_TEXT));
    memset(Text, 0, sizeof(WIDGET_TEXT));

    /* load properties */
    property_load(section, "prefix", NULL, &Text->prefix);
    property_load(section, "expression", NULL, &Text->value);
    property_load(section, "postfix", NULL, &Text->postfix);
    property_load(section, "style", "norm", &Text->style);

    /* field width, default 10 */
    cfg_number(section, "width", 10, 0, -1, &(Text->width));

    /* precision: number of digits after the decimal point (default: none) */
    /* Note: this is the *maximum* precision on small values, */
    /* for larger values the precision may be reduced to fit into the field width. */
    /* The default value 0xDEAD is used to distinguish between numbers and strings: */
    /* if no precision is given, the result is always treated as a string. If a */
    /* precision is specified, the result is treated as a number. */
    cfg_number(section, "precision", 0xDEAD, 0, 80, &(Text->precision));

    /* field alignment: Left (default), Center, Right or Marquee */
    c = cfg_get(section, "align", "L");
    switch (toupper(*c)) {
    case 'L':
	Text->align = ALIGN_LEFT;
	break;
    case 'C':
	Text->align = ALIGN_CENTER;
	break;
    case 'R':
	Text->align = ALIGN_RIGHT;
	break;
    case 'M':
	Text->align = ALIGN_MARQUEE;
	break;
    default:
	error("widget %s has unknown alignment '%s', using 'Left'", section, c);
	Text->align = ALIGN_LEFT;
    }
    free(c);

    /* update interval (msec), default 1 sec, 0 stands for never */
    cfg_number(section, "update", 1000, 0, -1, &(Text->update));
    /* limit update interval to min 10 msec */
    if (Text->update > 0 && Text->update < 10)
	Text->update = 10;

    /* marquee scroller speed: interval (msec), default 500msec */
    if (Text->align == ALIGN_MARQUEE) {
	cfg_number(section, "speed", 500, 10, -1, &(Text->speed));
    }

    /* buffer */
    Text->buffer = malloc(Text->width + 1);

    free(section);
    Self->data = Text;

    /* add update timer, use one-shot if 'update' is zero */
    timer_add(widget_text_update, Self, Text->update, Text->update == 0);

    /* a marquee scroller has its own timer and callback */
    if (Text->align == ALIGN_MARQUEE) {
	timer_add(widget_text_scroll, Self, Text->speed, 0);
    }

    return 0;
}


int widget_text_quit(WIDGET * Self)
{
    WIDGET_TEXT *Text;
    if (Self) {
	Text = Self->data;
	if (Self->data) {
	    property_free(&Text->prefix);
	    property_free(&Text->value);
	    property_free(&Text->postfix);
	    property_free(&Text->style);
	    if (Text->string)
		free(Text->string);
	    if (Text->buffer)
		free(Text->buffer);
	    free(Self->data);
	    Self->data = NULL;
	}

    }
    return 0;

}



WIDGET_CLASS Widget_Text = {
  name:"text",
  type:WIDGET_TYPE_RC,
  init:widget_text_init,
  draw:NULL,
  quit:widget_text_quit,
};
