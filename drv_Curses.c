/* $Id: drv_Curses.c,v 1.13 2006/07/19 01:48:11 cmay Exp $
 *
 * pure ncurses based text driver
 *
 * Copyright (C) 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old Curses/Text driver which is
 * Copyright (C) 2001 Leopold Toetsch <lt@toetsch.at>
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
 * $Log: drv_Curses.c,v $
 * Revision 1.13  2006/07/19 01:48:11  cmay
 * Ran indent.sh to make pretty code.
 *
 * Revision 1.12  2006/07/19 01:35:31  cmay
 * Renamed keypad direction names to avoid conflict with Curses library defs.
 * Added keypad support to Curses display driver.
 *
 * Revision 1.11  2006/01/30 06:25:49  reinelt
 * added CVS Revision
 *
 * Revision 1.10  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.9  2005/01/18 06:30:22  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.8  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.7  2004/06/26 09:27:20  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.6  2004/06/20 10:09:54  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.5  2004/06/06 06:51:59  reinelt
 *
 * do not display end splash screen if quiet=1
 *
 * Revision 1.4  2004/06/05 06:41:39  reinelt
 *
 * chancged splash screen again
 *
 * Revision 1.3  2004/06/05 06:13:11  reinelt
 *
 * splash screen for all text-based display drivers
 *
 * Revision 1.2  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.1  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Curses
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "timer.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "widget_keypad.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_keypad.h"


static char Name[] = "Curses";

static WINDOW *w = NULL;
static WINDOW *e = NULL;

static int EROWS;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_Curs_clear(void)
{
    werase(w);
    box(w, 0, 0);
    wrefresh(w);
}


static void drv_Curs_write(const int row, const int col, const char *data, const int len)
{
    int l = len;
    char *p;

    while ((p = strpbrk(data, "\r\n")) != NULL) {
	*p = '\0';
    }

    if (col < DCOLS) {
	if (DCOLS - col < l)
	    l = DCOLS - col;
	mvwprintw(w, row + 1, col + 1, "%.*s", l, data);
	wmove(w, DROWS + 1, 0);
	wrefresh(w);
    }
}


static void drv_Curs_defchar(const __attribute__ ((unused))
			     int ascii, const __attribute__ ((unused))
			     unsigned char *buffer)
{
    /* empty */
}


/* ncures scroll SIGSEGVs on my system, so this is a workaroud */

int curses_error(char *buffer)
{
    static int lines = 0;
    static char *lb[100];
    int start, i;
    char *p;

    if (e == NULL)
	return 0;

    /* replace \r, \n with underscores */
    while ((p = strpbrk(buffer, "\r\n")) != NULL) {
	*p = '_';
    }

    if (lines >= EROWS) {
	free(lb[0]);
	for (i = 1; i <= EROWS; i++) {
	    lb[i - 1] = lb[i];
	}
	start = 0;
    } else {
	start = lines;
    }

    lb[lines] = strdup(buffer);
    for (i = start; i <= lines; i++) {
	mvwprintw(e, i + 1, 1, "%s", lb[i]);
	wclrtoeol(e);
    }

    box(e, 0, 0);
    mvwprintw(e, 0, 3, "Stderr:");
    wrefresh(e);

    if (lines < EROWS)
	lines++;

    return 1;
}


static int drv_Curs_start(const char *section, const int quiet)
{
    char *s;

    if (!running_foreground) {
	error("%s: You want me to display on /dev/null? Sorry, I can't ...", Name);
	error("%s: Maybe you want me to run in foreground? Try '-F'", Name);
	return -1;
    }

    s = cfg_get(section, "Size", "20x4");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	free(s);
	return -1;
    }
    if (sscanf(s, "%dx%d", &DCOLS, &DROWS) != 2 || DROWS < 1 || DCOLS < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source);
	free(s);
	return -1;
    }
    free(s);

    initscr();
    noecho();
    debug("%s: curses thinks that COLS=%d LINES=%d", Name, COLS, LINES);
    w = newwin(DROWS + 2, DCOLS + 2, 0, 0);
    keypad(w, TRUE);
    nodelay(w, TRUE);

    EROWS = LINES - DROWS - 3;
    if (EROWS > 99)
	EROWS = 99;
    debug("EROWS=%d", EROWS);

    if (EROWS >= 4) {
	e = newwin(EROWS, COLS, DROWS + 3, 0);
	EROWS -= 3;
	box(e, 0, 0);
	mvwprintw(e, 0, 3, "Stderr:");
	wmove(e, 1, 0);
	wrefresh(e);
    }

    drv_Curs_clear();

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, NULL)) {
	    sleep(3);
	    drv_Curs_clear();
	}
    }

    return 0;
}

static void drv_Curs_timer(void __attribute__ ((unused)) * notused)
{
    int c;
    while (1) {
	c = wgetch(w);
	if (c <= 0)
	    break;
	drv_generic_keypad_press(c);
    }
}

static int drv_Curs_keypad(const int num)
{
    int val = 0;

    switch (num) {
    case KEY_UP:
	debug("Key Up");
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_UP;
	break;
    case KEY_DOWN:
	debug("Key Down");
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_DOWN;
	break;
    case KEY_LEFT:
	debug("Key Left");
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_LEFT;
	break;
    case KEY_RIGHT:
	debug("Key Right");
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_RIGHT;
	break;
    default:
	debug("Unbound Key '%d'", num);
	break;
    }

    return val;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


/****************************************/
/***        widget callbacks          ***/
/****************************************/

/* using drv_generic_text_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_keypad_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_Curs_list(void)
{
    printf("any");
    return 0;
}


/* initialize driver & display */
int drv_Curs_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Revision: 1.13 $");

    /* display preferences */
    XRES = 1;			/* pixel width of one char  */
    YRES = 1;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 0;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_Curs_write;
    drv_generic_text_real_defchar = drv_Curs_defchar;
    drv_generic_keypad_real_press = drv_Curs_keypad;

    /* regularly process display answers */
    timer_add(drv_Curs_timer, NULL, 100, 0);

    /* start display */
    if ((ret = drv_Curs_start(section, quiet)) != 0) {
	return ret;
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(1)) != 0)
	return ret;

    /* initialize generic key pad driver */
    if ((ret = drv_generic_keypad_init(section, Name)) != 0)
	return ret;

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    drv_generic_text_bar_add_segment(255, 255, 255, '*');	/* asterisk */

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);

    /* register plugins */
    /* none at the moment... */

    return 0;
}


/* close driver & display */
int drv_Curs_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();
    drv_generic_keypad_quit();

    /* clear display */
    drv_Curs_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    endwin();

    return (0);
}


DRIVER drv_Curses = {
  name:Name,
  list:drv_Curs_list,
  init:drv_Curs_init,
  quit:drv_Curs_quit,
};
