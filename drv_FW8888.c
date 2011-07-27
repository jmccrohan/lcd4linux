/* $Id$
 * $URL$
 *
 * driver for Allnet FW8888 display
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * Copyright (C) 2010 Linum Software GmbH <support@linum.com>
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
 *
 * exported fuctions:
 *
 * struct DRIVER drv_FW8888
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"

/* text mode display? */
#include "drv_generic_text.h"

/* serial port? */
#include "drv_generic_serial.h"

static char Name[] = "FW8888";


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_FW8888_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_FW8888_close(void)
{
    /* close whatever port you've opened */
    drv_generic_serial_close();

    return 0;
}


static void drv_FW8888_send_cmd(const unsigned int cmd)
{
    char cmd_str[2];

    cmd_str[0] = 0x10;
    cmd_str[1] = cmd;
    drv_generic_serial_write(cmd_str, 2);
}


static void drv_FW8888_send_text(const char *text, const unsigned int len)
{
    unsigned int i;
    char cmd_str[2];

    cmd_str[0] = 0x12;

    for (i = 0; i < len; i++) {
	cmd_str[1] = text[i];
	drv_generic_serial_write(cmd_str, 2);
    }
}

/* text mode displays only */
static void drv_FW8888_clear(void)
{
    drv_FW8888_send_cmd(0x01);
}

static void __attribute__ ((unused)) drv_FW8888_home(void)
{
    drv_FW8888_send_cmd(0x02);
}

static void __attribute__ ((unused)) drv_FW8888_display_off(void)
{
    drv_FW8888_send_cmd(0x08);
}

static void __attribute__ ((unused)) drv_FW8888_display_on_cursor_off(void)
{
    drv_FW8888_send_cmd(0x0C);
}

static void __attribute__ ((unused)) drv_FW8888_display_on_cursor_on(void)
{
    drv_FW8888_send_cmd(0x0E);
}

static void __attribute__ ((unused)) drv_FW8888_backlight_off(void)
{
    drv_FW8888_send_cmd(0x38);
}

static void __attribute__ ((unused)) drv_FW8888_backlight_on(void)
{
    drv_FW8888_send_cmd(0x39);
}

static void drv_FW8888_set_cursor(int row, int col)
{
    int pos;
    switch (row) {
    case 0:
	pos = 0x80 + col;
	break;
    case 1:
	pos = 0xC0 + col;
	break;
    default:
	error("%s: invalid row(%d) or col(%d)", Name, row, col);
	return;
    }
    drv_FW8888_send_cmd(pos);
}


/* text mode displays only */
static void drv_FW8888_write(const int row, const int col, const char *data, int len)
{
    /* do the cursor positioning here */
    drv_FW8888_set_cursor(row, col);

    /* send string to the display */
    drv_FW8888_send_text(data, len);
}


/* start text mode display */
static int drv_FW8888_start(const char *section)
{
    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    DROWS = 2;
    DCOLS = 16;

    GOTO_COST = -1;		/* number of bytes a goto command requires */

    /* open communication with the display */
    if (drv_FW8888_open(section) < 0) {
	return -1;
    }

    drv_FW8888_clear();		/* clear display */

    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/

/* none */

/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_gpio_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_FW8888_list(void)
{
    printf("Allnet-FW8888");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_FW8888_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 7;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */

    /* real worker functions */
    drv_generic_text_real_write = drv_FW8888_write;

    /* start display */
    if ((ret = drv_FW8888_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.linum.com")) {
	    sleep(3);
	    drv_FW8888_clear();
	}
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register plugins */
    //    AddFunction("LCD::contrast", 1, plugin_contrast);

    return 0;
}


/* close driver & display */
/* use this function for a text display */
int drv_FW8888_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_FW8888_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_FW8888_close();

    return (0);
}


/* use this one for a text display */
DRIVER drv_FW8888 = {
    .name = Name,
    .list = drv_FW8888_list,
    .init = drv_FW8888_init,
    .quit = drv_FW8888_quit,
};
