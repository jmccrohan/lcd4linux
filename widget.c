/* $Id$
 * $URL$
 *
 * generic widget handling
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
	switch (Class->type) {
	case WIDGET_TYPE_RC:
	    error("  Widgetclass %s is placed by Row/Column", class);
	    break;
	case WIDGET_TYPE_XY:
	    error("  Widgetclass %s is placed by X/Y", class);
	    break;
	case WIDGET_TYPE_GPO:
	case WIDGET_TYPE_TIMER:
	case WIDGET_TYPE_KEYPAD:
	default:
	    error("  Widgetclass %s has unknown type %d", class, Class->type);
	}
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
	error("internal error: widget buffer full! Tried to allocate %d widgets (max: %d)", nWidgets, MAX_WIDGETS);
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

    info(" widget '%s': Class '%s', Parent '%s', Layer %d, %s %d, %s %d",
	 name, (NULL == Class) ? "<none>" : Class->name,
	 (NULL == Parent) ? "<root>" : Parent->name,
	 layer, (WIDGET_TYPE_XY == Class->type) ? "Y" : "Row", row, (WIDGET_TYPE_XY == Class->type) ? "X" : "Col", col);

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
