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
 *
 * $Log: layout.c,v $
 * Revision 1.22  2006/02/21 05:50:34  reinelt
 * keypad support from Cris Maj
 *
 * Revision 1.21  2006/02/08 04:55:05  reinelt
 * moved widget registration to drv_generic_graphic
 *
 * Revision 1.20  2006/02/07 05:36:13  reinelt
 * Layers added to Layout
 *
 * Revision 1.19  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 * Revision 1.18  2006/01/23 06:17:18  reinelt
 * timer widget added
 *
 * Revision 1.17  2005/12/18 16:18:36  reinelt
 * GPO's added again
 *
 * Revision 1.16  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.15  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.14  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.13  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.12  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.11  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.10  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.9  2004/02/01 19:37:40  reinelt
 * got rid of every strtok() incarnation.
 *
 * Revision 1.8  2004/02/01 18:08:50  reinelt
 * removed strtok() from layout processing (took me hours to find this bug)
 * further strtok() removind should be done!
 *
 * Revision 1.7  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.6  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.5  2004/01/13 08:18:19  reinelt
 * timer queues added
 * liblcd4linux deactivated turing transformation to new layout
 *
 * Revision 1.4  2004/01/12 03:51:01  reinelt
 * evaluating the 'Variables' section in the config file
 *
 * Revision 1.3  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.2  2004/01/11 09:26:15  reinelt
 * layout starts to exist...
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
