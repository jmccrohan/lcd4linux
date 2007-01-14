/* $Id$
 * $URL$
 *
 * new layouter framework
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
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
 * layout_init (char *section)
 *    initializes the layouter
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cfg.h"
#include "widget.h"
#include "layout.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


int layout_init(const char *layout)
{
    char *section;
    char *list, *l;
    char *widget;
    int lay, row, col, num;

    info("initializing layout '%s'", layout);

    /* prepare config section */
    /* strlen("Layout:")=7 */
    section = malloc(strlen(layout) + 8);
    strcpy(section, "Layout:");
    strcat(section, layout);

    /* get a list of all keys in this section */
    list = cfg_list(section);

    /* map to lower char for scanf() */
    for (l = list; *l != '\0'; l++)
	*l = tolower(*l);

    l = list;
    while (l != NULL) {

	char *p;
	int i, n;

	/* list is delimited by | */
	while (*l == '|')
	    l++;
	if ((p = strchr(l, '|')) != NULL)
	    *p = '\0';

	/* layer/x/y widgets */
	i = sscanf(l, "layer:%d.x%d.y%d%n", &lay, &row, &col, &n);
	if (i == 3 && l[n] == '\0') {
	    if (lay < 0 || lay >= LAYERS) {
		error("%s: layer %d out of bounds (0..%d)", section, lay, LAYERS - 1);
	    } else {
		widget = cfg_get(section, l, NULL);
		if (widget != NULL && *widget != '\0') {
		    widget_add(widget, WIDGET_TYPE_XY, lay, row - 1, col - 1);
		}
		free(widget);
	    }
	}

	/* layer/row/col widgets */
	i = sscanf(l, "layer:%d.row%d.col%d%n", &lay, &row, &col, &n);
	if (i == 3 && l[n] == '\0') {
	    if (lay < 0 || lay >= LAYERS) {
		error("%s: layer %d out of bounds (0..%d)", section, lay, LAYERS - 1);
	    } else {
		widget = cfg_get(section, l, NULL);
		if (widget != NULL && *widget != '\0') {
		    widget_add(widget, WIDGET_TYPE_RC, lay, row - 1, col - 1);
		}
		free(widget);
	    }
	}

	/* row/col widgets w/o layer */
	i = sscanf(l, "row%d.col%d%n", &row, &col, &n);
	if (i == 2 && l[n] == '\0') {
	    widget = cfg_get(section, l, NULL);
	    if (widget != NULL && *widget != '\0') {
		/* default is layer 1 if outside layer section */
		widget_add(widget, WIDGET_TYPE_RC, 1, row - 1, col - 1);
	    }
	    free(widget);
	}

	/* GPO widgets */
	i = sscanf(l, "gpo%d%n", &num, &n);
	if (i == 1 && l[n] == '\0') {
	    widget = cfg_get(section, l, NULL);
	    if (widget != NULL && *widget != '\0') {
		widget_add(widget, WIDGET_TYPE_GPO, 0, num - 1, 0);
	    }
	    free(widget);
	}

	/* timer widgets */
	i = sscanf(l, "timer%d%n", &num, &n);
	if (i == 1 && l[n] == '\0') {
	    widget = cfg_get(section, l, NULL);
	    if (widget != NULL && *widget != '\0') {
		widget_add(widget, WIDGET_TYPE_TIMER, 0, num - 1, 0);
	    }
	    free(widget);
	}

	/* keypad widget */
	i = sscanf(l, "keypad%d%n", &num, &n);
	if (i == 1 && l[n] == '\0') {
	    widget = cfg_get(section, l, NULL);
	    if (widget != NULL && *widget != '\0') {
		widget_add(widget, WIDGET_TYPE_KEYPAD, 0, num - 1, 0);
	    }
	    free(widget);
	}

	/* next field */
	l = p ? p + 1 : NULL;
    }
    free(list);
    free(section);
    return 0;
}
