/* $Id$
 * $URL$
 *
 * Pertelian lcd4linux driver
 * Copyright (C) 2007 Andy Powell <lcd4linux@automted.it> 
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_Pertelian
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

#include "drv_generic_text.h"
#include "drv_generic_serial.h"

static char Name[] = "Pertelian";

static unsigned char rowoffset[4] = { 0x80, 0x80 + 0x40, 0x80 + 0x14, 0x80 + 0x40 + 0x14 };

#define PERTELIAN_LCDCOMMAND 0xfe

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


static int drv_Pertelian_open(const char *section)
{
    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;
    return 0;
}


static int drv_Pertelian_close(void)
{
    drv_generic_serial_close();
    return 0;
}


static void drv_Pertelian_send(const char *data, const unsigned int len)
{
    unsigned int i;

    /* Pertelian interface seems to offer no buffering at all
       so we have to slow things down a tad yes 1usec is enough  */

    for (i = 0; i < len; i++) {
	drv_generic_serial_write(&data[i], 1);
	usleep(100);
    }
}


static void drv_Pertelian_clear(void)
{
    char cmd[2];

    cmd[0] = PERTELIAN_LCDCOMMAND;
    cmd[1] = 0x01;
    drv_Pertelian_send(cmd, 2);
}


static void drv_Pertelian_write(const int row, const int col, const char *data, int len)
{
    char cmd[3];

    cmd[0] = PERTELIAN_LCDCOMMAND;
    cmd[1] = ((rowoffset[row]) + (col));
    drv_Pertelian_send(cmd, 2);
    drv_Pertelian_send(data, len);
}


static void drv_Pertelian_defchar(const int ascii, const unsigned char *matrix)
{
    char cmd[11] = "";
    int i;

    cmd[0] = PERTELIAN_LCDCOMMAND;
    cmd[1] = (0x40 + (8 * ascii));
    for (i = 0; i < 8; i++) {
	cmd[i + 2] = matrix[i] & 0x1f;
    }
    drv_Pertelian_send(cmd, 10);
}


static int drv_Pertelian_backlight(int backlight)
{
    char cmd[2];

    if (backlight <= 0)
	backlight = 2;
    else if (backlight >= 1)
	backlight = 3;

    cmd[0] = PERTELIAN_LCDCOMMAND;
    cmd[1] = backlight;

    drv_Pertelian_send(cmd, 2);

    return backlight;
}


static int drv_Pertelian_start(const char *section)
{
    int backlight;
    int rows = -1, cols = -1;
    char *s;
    char cmd[12] = "";

    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }

    DROWS = rows;
    DCOLS = cols;

    /* open communication with the display */
    if (drv_Pertelian_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    cmd[0] = PERTELIAN_LCDCOMMAND;
    cmd[1] = 0x38;
    cmd[2] = PERTELIAN_LCDCOMMAND;
    cmd[3] = 0x06;
    cmd[4] = PERTELIAN_LCDCOMMAND;
    cmd[5] = 0x10;		/* move cursor on data write */
    cmd[6] = PERTELIAN_LCDCOMMAND;
    cmd[7] = 0x0c;
    cmd[8] = 0x0c;

    drv_Pertelian_send(cmd, 8);

    if (cfg_number(section, "Backlight", 0, 0, 1, &backlight) > 0) {
	drv_Pertelian_backlight(backlight);
    }

    drv_Pertelian_clear();	/* clear display */
    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight = 0;
    backlight = drv_Pertelian_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}

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
int drv_Pertelian_list(void)
{
    printf("Pertelian X2040 displays");
    return 0;
}


/* initialize driver & display */
int drv_Pertelian_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_Pertelian_write;
    drv_generic_text_real_defchar = drv_Pertelian_defchar;


    /* start display */
    if ((ret = drv_Pertelian_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "LinITX.com")) {
	    sleep(3);
	    drv_Pertelian_clear();
	}
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic icon driver */
    if ((ret = drv_generic_text_icon_init()) != 0)
	return ret;

    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(0)) != 0)
	return ret;

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */


    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_text_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);
    AddFunction("LCD::backlight", 1, plugin_backlight);
    return 0;
}


/* close driver & display */
int drv_Pertelian_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_Pertelian_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_Pertelian_backlight(0);
    debug("closing connection");
    drv_Pertelian_close();

    return (0);
}


DRIVER drv_Pertelian = {
    .name = Name,
    .list = drv_Pertelian_list,
    .init = drv_Pertelian_init,
    .quit = drv_Pertelian_quit,
};
