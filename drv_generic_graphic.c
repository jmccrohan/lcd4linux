/* $Id
 *
 * generic driver helper for graphic displays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_generic_graphic.c,v $
 * Revision 1.17  2006/01/03 06:13:46  reinelt
 * GPIO's for MatrixOrbital
 *
 * Revision 1.16  2005/12/13 14:07:28  reinelt
 * LPH7508 driver finished
 *
 * Revision 1.15  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.14  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.13  2005/01/09 10:53:24  reinelt
 * small type in plugin_uname fixed
 * new homepage lcd4linux.bulix.org
 *
 * Revision 1.12  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.11  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.10  2004/06/20 10:09:55  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.9  2004/06/09 06:40:29  reinelt
 *
 * splash screen for T6963 driver
 *
 * Revision 1.8  2004/06/08 21:46:38  reinelt
 *
 * splash screen for X11 driver (and generic graphic driver)
 *
 * Revision 1.7  2004/06/01 06:45:30  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.6  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.5  2004/02/29 14:30:59  reinelt
 * icon visibility fix for generic graphics from Xavier
 *
 * Revision 1.4  2004/02/24 05:55:04  reinelt
 *
 * X11 driver ported
 *
 * Revision 1.3  2004/02/22 17:35:41  reinelt
 * some fixes for generic graphic driver and T6963
 * removed ^M from plugin_imon (Nico, are you editing under Windows?)
 *
 * Revision 1.2  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.1  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 */

/* 
 *
 * exported functions:
 *
 * int drv_generic_graphic_init (char *section, char *driver);
 *   initializes the generic graphic driver
 *
 * int drv_generic_graphic_draw (WIDGET *W);
 *   renders Text widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_icon_draw (WIDGET *W);
 *   renders Icon widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_bar_draw (WIDGET *W);
 *   renders Bar widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_quit (void);
 *   closes generic graphic driver
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_graphic.h"
#include "font_6x8.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static char *Section = NULL;
static char *Driver = NULL;

int DROWS, DCOLS;		/* display size (pixels!) */
int LROWS, LCOLS;		/* layout size  (pixels!) */
int XRES, YRES;			/* pixels of one char cell */

unsigned char *drv_generic_graphic_FB = NULL;

void (*drv_generic_graphic_real_blit) () = NULL;

/****************************************/
/*** generic Framebuffer stuff        ***/
/****************************************/

static void drv_generic_graphic_resizeFB(int rows, int cols)
{
    unsigned char *newFB;
    int row, col;

    /* Layout FB is large enough */
    if (rows <= LROWS && cols <= LCOLS)
	return;

    /* get maximum values */
    if (rows < LROWS)
	rows = LROWS;
    if (cols < LCOLS)
	cols = LCOLS;

    /* allocate new Layout FB */
    newFB = malloc(cols * rows * sizeof(char));
    memset(newFB, 0, rows * cols * sizeof(char));

    /* transfer contents */
    if (drv_generic_graphic_FB != NULL) {
	for (row = 0; row < LROWS; row++) {
	    for (col = 0; col < LCOLS; col++) {
		newFB[row * cols + col] = drv_generic_graphic_FB[row * LCOLS + col];
	    }
	}
	free(drv_generic_graphic_FB);
    }
    drv_generic_graphic_FB = newFB;

    LCOLS = cols;
    LROWS = rows;
}


int drv_generic_graphic_clear(void)
{
    memset(drv_generic_graphic_FB, 0, LCOLS * LROWS * sizeof(*drv_generic_graphic_FB));
    if (drv_generic_graphic_real_blit)
	drv_generic_graphic_real_blit(0, 0, LROWS, LCOLS);

    return 0;
}


/****************************************/
/*** generic text handling            ***/
/****************************************/

static void drv_generic_graphic_render(const int row, const int col, const char *txt)
{
    int c, r, x, y;
    int len = strlen(txt);

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + YRES, col + XRES * len);

    r = row;
    c = col;

    /* render text into layout FB */
    while (*txt != '\0') {
	for (y = 0; y < YRES; y++) {
	    int mask = 1 << XRES;
	    for (x = 0; x < XRES; x++) {
		mask >>= 1;
		drv_generic_graphic_FB[(r + y) * LCOLS + c + x] = Font_6x8[(int) *txt][y] & mask ? 1 : 0;
	    }
	}
	c += XRES;
	txt++;
    }

    /* flush area */
    if (drv_generic_graphic_real_blit)
	drv_generic_graphic_real_blit(row, col, YRES, XRES * len);

}


/* say hello to the user */
int drv_generic_graphic_greet(const char *msg1, const char *msg2)
{
    char *line1[] = { "* LCD4Linux " VERSION " *",
	"LCD4Linux " VERSION,
	"* LCD4Linux *",
	"LCD4Linux",
	"L4Linux",
	NULL
    };

    char *line2[] = { "http://lcd4linux.bulix.org",
	"lcd4linux.bulix.org",
	NULL
    };

    int i;
    int flag = 0;

    unsigned int cols = DCOLS / XRES;
    unsigned int rows = DROWS / YRES;

    for (i = 0; line1[i]; i++) {
	if (strlen(line1[i]) <= cols) {
	    drv_generic_graphic_render(YRES * 0, XRES * (cols - strlen(line1[i])) / 2, line1[i]);
	    flag = 1;
	    break;
	}
    }

    if (rows >= 2) {
	for (i = 0; line2[i]; i++) {
	    if (strlen(line2[i]) <= cols) {
		drv_generic_graphic_render(YRES * 1, XRES * (cols - strlen(line2[i])) / 2, line2[i]);
		flag = 1;
		break;
	    }
	}
    }

    if (msg1 && rows >= 3) {
	unsigned int len = strlen(msg1);
	if (len <= cols) {
	    drv_generic_graphic_render(YRES * 2, XRES * (cols - len) / 2, msg1);
	    flag = 1;
	}
    }

    if (msg2 && rows >= 4) {
	unsigned int len = strlen(msg2);
	if (len <= cols) {
	    drv_generic_graphic_render(YRES * 3, XRES * (cols - len) / 2, msg2);
	    flag = 1;
	}
    }

    return flag;
}


int drv_generic_graphic_draw(WIDGET * W)
{
    WIDGET_TEXT *Text = W->data;
    drv_generic_graphic_render(YRES * W->row, XRES * W->col, Text->buffer);
    return 0;
}


/****************************************/
/*** generic icon handling            ***/
/****************************************/

int drv_generic_graphic_icon_draw(WIDGET * W)
{
    WIDGET_ICON *Icon = W->data;
    unsigned char *bitmap = Icon->bitmap + YRES * Icon->curmap;
    int row, col;
    int x, y;

    row = YRES * W->row;
    col = XRES * W->col;

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + YRES, col + XRES);

    /* render icon */
    for (y = 0; y < YRES; y++) {
	int mask = 1 << XRES;
	for (x = 0; x < XRES; x++) {
	    int i = (row + y) * LCOLS + col + x;
	    mask >>= 1;
	    if (Icon->visible) {
		drv_generic_graphic_FB[i] = bitmap[y] & mask ? 1 : 0;
	    } else {
		drv_generic_graphic_FB[i] = 0;
	    }
	}
    }

    /* flush area */
    if (drv_generic_graphic_real_blit)
	drv_generic_graphic_real_blit(row, col, YRES, XRES);


    return 0;

}


/****************************************/
/*** generic bar handling             ***/
/****************************************/

int drv_generic_graphic_bar_draw(WIDGET * W)
{
    WIDGET_BAR *Bar = W->data;
    int row, col, len, res, rev, max, val1, val2;
    int x, y;
    DIRECTION dir;

    row = YRES * W->row;
    col = XRES * W->col;
    dir = Bar->direction;
    len = Bar->length;

    /* maybe grow layout framebuffer */
    if (dir & (DIR_EAST | DIR_WEST)) {
	drv_generic_graphic_resizeFB(row + YRES, col + XRES * len);
    } else {
	drv_generic_graphic_resizeFB(row + YRES * len, col + XRES);
    }

    res = dir & (DIR_EAST | DIR_WEST) ? XRES : YRES;
    max = len * res;
    val1 = Bar->val1 * (double) (max);
    val2 = Bar->val2 * (double) (max);

    if (val1 < 1)
	val1 = 1;
    else if (val1 > max)
	val1 = max;

    if (val2 < 1)
	val2 = 1;
    else if (val2 > max)
	val2 = max;

    rev = 0;

    switch (dir) {
    case DIR_WEST:
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_EAST:
	for (y = 0; y < YRES; y++) {
	    int val = y < YRES / 2 ? val1 : val2;
	    for (x = 0; x < max; x++) {
		drv_generic_graphic_FB[(row + y) * LCOLS + col + x] = x < val ? !rev : rev;
	    }
	}
	break;

    case DIR_SOUTH:
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_NORTH:
	for (y = 0; y < max; y++) {
	    for (x = 0; x < XRES; x++) {
		int val = x < XRES / 2 ? val1 : val2;
		drv_generic_graphic_FB[(row + y) * LCOLS + col + x] = y < val ? !rev : rev;
	    }
	}
	break;
    }

    /* flush area */
    if (dir & (DIR_EAST | DIR_WEST)) {
	if (drv_generic_graphic_real_blit)
	    drv_generic_graphic_real_blit(row, col, YRES, XRES * len);
    } else {
	if (drv_generic_graphic_real_blit)
	    drv_generic_graphic_real_blit(row, col, YRES * len, XRES);
    }

    return 0;
}


/****************************************/
/*** generic init/quit                ***/
/****************************************/

int drv_generic_graphic_init(const char *section, const char *driver)
{
    Section = (char *) section;
    Driver = (char *) driver;

    /* init layout framebuffer */
    LROWS = 0;
    LCOLS = 0;
    drv_generic_graphic_FB = NULL;
    drv_generic_graphic_resizeFB(DROWS, DCOLS);

    /* sanity check */
    if (drv_generic_graphic_FB == NULL) {
	error("%s: framebuffer could not be allocated: malloc() failed", Driver);
	return -1;
    }

    return 0;
}


int drv_generic_graphic_quit(void)
{
    if (drv_generic_graphic_FB) {
	free(drv_generic_graphic_FB);
	drv_generic_graphic_FB = NULL;
    }

    widget_unregister();
    return (0);
}
