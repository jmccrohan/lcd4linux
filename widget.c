/* $Id: widget.c,v 1.26 2006/08/09 17:25:34 harbaum Exp $
 *
 * generic widget handling
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
 * $Log: widget.c,v $
 * Revision 1.26  2006/08/09 17:25:34  harbaum
 * Better bar color support and new bold font
 *
 * Revision 1.25  2006/08/08 20:16:29  harbaum
 * Added "extracolor" (used for e.g. bar border) and RGB support for LEDMATRIX
 *
 * Revision 1.24  2006/08/08 19:28:18  reinelt
 * widget type checking corrected
 *
 * Revision 1.23  2006/02/21 05:50:34  reinelt
 * keypad support from Cris Maj
 *
 * Revision 1.22  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 * Revision 1.21  2005/12/18 16:18:36  reinelt
 * GPO's added again
 *
 * Revision 1.20  2005/11/06 09:17:20  reinelt
 * re-use icons (thanks to Jesus de Santos Garcia)
 *
 * Revision 1.19  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.18  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.17  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.16  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.15  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.14  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.13  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.12  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.11  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.10  2004/01/29 04:40:03  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.9  2004/01/23 04:53:57  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.8  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.7  2004/01/13 08:18:20  reinelt
 * timer queues added
 * liblcd4linux deactivated turing transformation to new layout
 *
 * Revision 1.6  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.5  2004/01/11 09:26:15  reinelt
 * layout starts to exist...
 *
 * Revision 1.4  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.3  2004/01/10 17:34:40  reinelt
 * further matrixOrbital changes
 * widgets initialized
 *
 * Revision 1.2  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.1  2003/09/19 03:51:29  reinelt
 * minor fixes, widget.c added
 *
 */

/* 
 * exported functions:
 *
 * int widget_junk(void)
 *   does something
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "cfg.h"
#include "widget.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* we use a static array of widgets and not realloc() */
#define MAX_WIDGETS 256

static WIDGET_CLASS *Classes = NULL;
static int nClasses = 0;

static WIDGET *Widgets = NULL;
static int nWidgets = 0;

static int widget_added = 0;

int widget_register(WIDGET_CLASS * widget)
{
    int i;

    /* sanity check: disallow widget registering after at least one */
    /* widget has been added, because we use realloc here, and there may  */
    /* be pointers to the old memory area */
    if (widget_added) {
	error("internal error: register_widget(%s) after add_widget()", widget->name);
	return -1;
    }

    for (i = 0; i < nClasses; i++) {
	if (strcasecmp(widget->name, Classes[i].name) == 0) {
	    error("internal error: widget '%s' already exists!", widget->name);
	    return -1;
	}
    }

    nClasses++;
    Classes = realloc(Classes, nClasses * sizeof(WIDGET_CLASS));
    Classes[nClasses - 1] = *widget;

    return 0;
}

void widget_unregister(void)
{
    int i;
    for (i = 0; i < nWidgets; i++) {
	Widgets[i].class->quit(&(Widgets[i]));
	if (Widgets[i].name)
	    free(Widgets[i].name);
    }
    free(Widgets);

    free(Classes);

    nWidgets = 0;
    nClasses = 0;
}

int widget_color(const char *section, const char *name, const char *key, RGBA * C)
{
    char *color;

    C->R = 0;
    C->G = 0;
    C->B = 0;
    C->A = 0;

    color = cfg_get(section, key, NULL);

    if (color == NULL)
	return 0;

    if (*color == '\0') {
	free(color);
	return 0;
    }

    if (color2RGBA(color, C) < 0) {
	error("widget '%s': ignoring illegal %s color '%s'", name, key, color);
	free(color);
	return 0;
    }
    free(color);
    return 1;
}

int widget_add(const char *name, const int type, const int layer, const int row, const int col)
{
    int i;
    char *section;
    char *class;
    int fg_valid, bg_valid;
    RGBA FG, BG;

    WIDGET_CLASS *Class;
    WIDGET *Widget;
    WIDGET *Parent;

    /* prepare config section */
    /* strlen("Widget:")=7 */
    section = malloc(strlen(name) + 8);
    strcpy(section, "Widget:");
    strcat(section, name);

    /* get widget class */
    class = cfg_get(section, "class", NULL);
    if (class == NULL || *class == '\0') {
	error("error: widget '%s' has no class!", name);
	if (class)
	    free(class);
	free(section);
	return -1;
    }

    /* get widget foreground color */
    fg_valid = widget_color(section, name, "foreground", &FG);
    bg_valid = widget_color(section, name, "background", &BG);

    free(section);

    /* lookup widget class */
    Class = NULL;
    for (i = 0; i < nClasses; i++) {
	if (strcasecmp(class, Classes[i].name) == 0) {
	    Class = &(Classes[i]);
	    break;
	}
    }
    if (i == nClasses) {
	error("widget '%s': class '%s' not supported", name, class);
	if (class)
	    free(class);
	return -1;
    }

    /* check if widget type matches */
    if (Class->type != type) {
	error("widget '%s': class '%s' not applicable", name, class);
	free(class);
	return -1;
    }

    if (class)
	free(class);


    /* do NOT use realloc here because there may be pointers to the old */
    /* memory area, which would point to nowhere if realloc moves the area */
    if (Widgets == NULL) {
	Widgets = malloc(MAX_WIDGETS * sizeof(WIDGET));
	if (Widgets == NULL) {
	    error("internal error: allocation of widget buffer failed: %s", strerror(errno));
	    return -1;
	}
    }

    /* another sanity check */
    if (nWidgets >= MAX_WIDGETS) {
	error("internal error: widget buffer full!");
	return -1;
    }

    /* look up parent widget (widget with the same name) */
    Parent = NULL;
    for (i = 0; i < nWidgets; i++) {
	if (strcmp(name, Widgets[i].name) == 0) {
	    Parent = &(Widgets[i]);
	    break;
	}
    }

    Widget = &(Widgets[nWidgets]);
    nWidgets++;

    Widget->name = strdup(name);
    Widget->class = Class;
    Widget->parent = Parent;
    Widget->fg_color = FG;
    Widget->bg_color = BG;
    Widget->fg_valid = fg_valid;
    Widget->bg_valid = bg_valid;
    Widget->layer = layer;
    Widget->row = row;
    Widget->col = col;

    if (Class->init != NULL) {
	Class->init(Widget);
    }

    return 0;
}

/* return the found widget, or else NULL */
WIDGET *widget_find(int type, void *needle)
{
    WIDGET *widget = NULL;
    int i;

    for (i = 0; i < nWidgets; i++) {
	widget = &(Widgets[i]);
	if (widget->class->type == type) {
	    if (widget->class->find != NULL && widget->class->find(widget, needle) == 0)
		break;
	}
	widget = NULL;
    }

    return widget;
}
