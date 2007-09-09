/* $Id$
 * $URL$
 *
 * icon widget handling
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
 * WIDGET_CLASS Widget_Icon
 *   the icon widget
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
#include "widget_icon.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* icons size is same as char size */
extern int XRES, YRES;

static void widget_icon_read_bitmap(const char *section, WIDGET_ICON * Icon)
{
    int row, n;
    char key[15];
    char *val, *v;
    unsigned char *map;

    for (row = 0; row < YRES; row++) {
	qprintf(key, sizeof(key), "Bitmap.Row%d", row + 1);
	val = cfg_get(section, key, "");
	map = Icon->bitmap + row;
	n = 0;
	for (v = val; *v != '\0'; v++) {
	    if (n >= Icon->maxmap) {
		Icon->maxmap++;
		Icon->bitmap = realloc(Icon->bitmap, Icon->maxmap * YRES * sizeof(char));
		memset(Icon->bitmap + n * YRES, 0, YRES * sizeof(char));
		map = Icon->bitmap + n * YRES + row;
	    }
	    switch (*v) {
	    case '|':
		n++;
		map += YRES;
		break;
	    case '*':
		(*map) <<= 1;
		(*map) |= 1;
		break;
	    default:
		(*map) <<= 1;
	    }
	}
	free(val);
    }
}


void widget_icon_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_ICON *Icon = W->data;

    /* process the parent only */
    if (W->parent == NULL) {

	/* evaluate properties */
	property_eval(&Icon->speed);
	property_eval(&Icon->visible);

	/* rotate icon bitmap */
	Icon->curmap++;
	if (Icon->curmap >= Icon->maxmap)
	    Icon->curmap = 0;
    }

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

    /* add a new one-shot timer */
    if (P2N(&Icon->speed) > 0) {
	timer_add(widget_icon_update, Self, P2N(&Icon->speed), 1);
    }
}



int widget_icon_init(WIDGET * Self)
{
    char *section;
    WIDGET_ICON *Icon;

    /* re-use the parent if one exists */
    if (Self->parent == NULL) {

	/* prepare config section */
	/* strlen("Widget:")=7 */
	section = malloc(strlen(Self->name) + 8);
	strcpy(section, "Widget:");
	strcat(section, Self->name);

	Icon = malloc(sizeof(WIDGET_ICON));
	memset(Icon, 0, sizeof(WIDGET_ICON));

	/* load properties */
	property_load(section, "speed", "100", &Icon->speed);
	property_load(section, "visible", "1", &Icon->visible);

	/* read bitmap */
	widget_icon_read_bitmap(section, Icon);

	free(section);
	Self->data = Icon;

	/* as the speed is evaluatod on every call, we use 'one-shot'-timers.  */
	/* The timer will be reactivated on every call to widget_icon_update().  */
	/* We do the initial call here... */
	Icon->prvmap = -1;

	/* reset ascii  */
	Icon->ascii = -1;

    } else {

	/* re-use the parent */
	Self->data = Self->parent->data;

    }

    /* just do it! */
    widget_icon_update(Self);

    return 0;
}


int widget_icon_quit(WIDGET * Self)
{
    if (Self) {
	/* do not deallocate child widget! */
	if (Self->parent == NULL) {
	    if (Self->data) {
		WIDGET_ICON *Icon = Self->data;
		property_free(&Icon->speed);
		property_free(&Icon->visible);
		if (Icon->bitmap)
		    free(Icon->bitmap);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }

    return 0;

}



WIDGET_CLASS Widget_Icon = {
    .name = "icon",
    .type = WIDGET_TYPE_RC,
    .init = widget_icon_init,
    .draw = NULL,
    .quit = widget_icon_quit,
};
