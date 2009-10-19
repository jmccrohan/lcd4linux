/* $Id$
 * $URL$
 *
 * driver for Logic Way GmbH ABP08 (serial) and ABP09 (USB) appliance control panels
 * http://www.logicway.de/pages/mde-hardware.shtml#ABP.AMC
 *
 * Copyright (C) 2009 Arndt Kritzner <kritzner@logicway.de>
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_LW_ABP
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

/* i2c bus? */
#ifdef WITH_I2C
#include "drv_generic_i2c.h"
#endif


static char Name[] = "LW_ABP";


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_LW_ABP_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_LW_ABP_close(void)
{
    drv_generic_serial_close();

    return 0;
}


/* dummy function that sends something to the display */
static void drv_LW_ABP_send(const char *data, const unsigned int len)
{
    /* send data to the serial port is easy... */
    drv_generic_serial_write(data, len);
}


/* text mode displays only */
static void drv_LW_ABP_clear(void)
{
    char cmd[] = "lcd init\r\n";

    /* do whatever is necessary to clear the display */
    drv_LW_ABP_send(cmd, strlen(cmd));
}


/* text mode displays only */
static void drv_LW_ABP_write(const int row, const int col, const char *data, int len)
{
    char cmd[] = "lcd set line ";
    char row_[5];
    char col_[5];

    /* do the cursor positioning here */
    drv_LW_ABP_send(cmd, strlen(cmd));
    sprintf(row_, "%d", row + 1);
    drv_LW_ABP_send(row_, strlen(row_));
    if (col > 0) {
	drv_LW_ABP_send(",", 1);
	sprintf(col_, "%d", col + 1);
	drv_LW_ABP_send(col_, strlen(col_));
    }
    drv_LW_ABP_send(" ", 1);

    /* send string to the display */
    drv_LW_ABP_send(data, len);
    drv_LW_ABP_send("\r\n", 2);

}

/* text mode displays only */
static void drv_LW_ABP_defchar(const int ascii, const unsigned char *matrix)
{
}

/* example function used in a plugin */
static int drv_LW_ABP_contrast(int contrast)
{
    char cmd[2];

    /* adjust limits according to the display */
    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    /* call a 'contrast' function */
    /* assume 0x04 to be the 'set contrast' command */
    cmd[0] = 0x04;
    cmd[1] = contrast;
    drv_LW_ABP_send(cmd, 2);

    return contrast;
}


/* start text mode display */
static int drv_LW_ABP_start(const char *section)
{
    int contrast;
    int rows = -1, cols = -1;
    char *s;
    char cmd[1];

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
    if (drv_LW_ABP_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    /* assume 0x00 to be a 'reset' command */
    cmd[0] = 0x00;
    drv_LW_ABP_send(cmd, 0);

    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_LW_ABP_contrast(contrast);
    }

    drv_LW_ABP_clear();		/* clear display */

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_LW_ABP_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
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
int drv_LW_ABP_list(void)
{
    printf("Logic Way ABP driver");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_LW_ABP_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 10;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_LW_ABP_write;
    drv_generic_text_real_defchar = drv_LW_ABP_defchar;

    /* start display */
    if ((ret = drv_LW_ABP_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.logicway.de")) {
	    sleep(3);
	    drv_LW_ABP_clear();
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

    /* register plugins */
    AddFunction("LCD::contrast", 1, plugin_contrast);

    return 0;
}



/* close driver & display */
/* use this function for a text display */
int drv_LW_ABP_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_LW_ABP_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_LW_ABP_close();

    return (0);
}


/* use this one for a text display */
DRIVER drv_LW_ABP = {
    .name = Name,
    .list = drv_LW_ABP_list,
    .init = drv_LW_ABP_init,
    .quit = drv_LW_ABP_quit,
};
