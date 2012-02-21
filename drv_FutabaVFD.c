/* $Id: drv_FutabaVFD.c -1   $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_FutabaVFD.c $
 *
 * A driver to run Futaba VFD M402SD06GL with LCD4Linux on parallel port
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * Copyright (C) 2012 Marcus Menzel <codingmax@gmx-topmail.de>
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
 * struct DRIVER drv_FutabaVFD
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
#include "drv_generic_parport.h"


static char Name[] = "FutabaVFD";

static unsigned char SIGNAL_WR;
static unsigned char SIGNAL_SELECT;
static unsigned char SIGNAL_TEST;
static unsigned char SIGNAL_BUSY;

static unsigned char dim;	/* brightness 0..3 */
static unsigned char curPos;	/* cursor position */
static unsigned char curOn;	/* cursor on */

static unsigned char BUSY_SHIFT;
static unsigned char BUSY_VALUE;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


/* example for sending one byte over the wire */
static void drv_FutabaVFD_writeChar(const unsigned char c)
{

    int i;
    unsigned char status;

    drv_generic_parport_data(c);
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_SELECT, 0);
    ndelay(60);
    drv_generic_parport_control(SIGNAL_WR, 0xFF);

    /*wait <=60ns till busy */
    for (i = 0; i < 60; i++) {
	status = drv_generic_parport_status();
	if (((status >> BUSY_SHIFT) & 1) == BUSY_VALUE)
	    break;
	ndelay(1);
    }
    ndelay(60 - i);
    drv_generic_parport_control(SIGNAL_SELECT, 0xFF);

    /*wait max 0.1s till not busy */
    for (i = 0; i < 100000000; i++) {
	status = drv_generic_parport_status();
	if (((status >> BUSY_SHIFT) & 1) != BUSY_VALUE)
	    break;
	ndelay(1);
    }
    ndelay(210);
}



static void drv_FutabaVFD_showCursor(int b)
{
    drv_FutabaVFD_writeChar((b) ? 0x13 : 0x14);	/* b==0: cursor off */
}

static void drv_FutabaVFD_clear()
{
    drv_FutabaVFD_writeChar(0x1F);	/*reset */
    drv_FutabaVFD_writeChar(0x11);	/*DC1 - normal display */
    drv_FutabaVFD_showCursor(0);
}


static int drv_FutabaVFD_open(const char *section)
{

    if (drv_generic_parport_open(section, Name) != 0) {
	error("%s: could not initialize parallel port!", Name);
	return -1;
    }

    /* read the wiring from config */
    if ((SIGNAL_WR = drv_generic_parport_wire_ctrl("WR", "STROBE")) == 0xFF)
	return -1;
    if ((SIGNAL_SELECT = drv_generic_parport_wire_ctrl("SEL", "SLCTIN")) == 0xFF)
	return -1;
    if ((SIGNAL_TEST = drv_generic_parport_wire_ctrl("TEST", "AUTOFD")) == 0xFF)
	return -1;
    if ((SIGNAL_BUSY = drv_generic_parport_wire_status("BUSY", "BUSY")) == 0xFF)
	return -1;

    BUSY_SHIFT = 0;
    BUSY_VALUE = SIGNAL_BUSY;
    while (BUSY_VALUE > 1) {
	BUSY_VALUE >>= 1;
	BUSY_SHIFT++;
    }
    BUSY_VALUE = (SIGNAL_BUSY == 0x80) ? 0 : 1;	/* portpin 11 inverted */

    /* set all signals to high */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_SELECT | SIGNAL_TEST, 0xFF);

    /* set direction: write */
    drv_generic_parport_direction(0);

    return 0;
}


static int drv_FutabaVFD_close(void)
{
    drv_generic_parport_close();
    return 0;
}

static void drv_FutabaVFD_send(const char *data, const unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
	drv_FutabaVFD_writeChar(*data++);
}


static void drv_FutabaVFD_goto(unsigned char pos)
{
    curPos = pos;
    drv_FutabaVFD_writeChar(0x10);
    drv_FutabaVFD_writeChar(curPos);
}


static void drv_FutabaVFD_write(const int row, const int col, const char *data, int len)
{
    unsigned char oldPos = curPos;
    if (curOn)
	drv_FutabaVFD_showCursor(0);
    drv_FutabaVFD_goto(row * DCOLS + col);
    drv_FutabaVFD_send(data, len);
    drv_FutabaVFD_goto(oldPos);
    if (curOn)
	drv_FutabaVFD_showCursor(1);
}

static void drv_FutabaVFD_defchar(const int ascii, const unsigned char *matrix)
{
    return;			// no defchars
}


static void drv_FutabaVFD_brightness(int brightness)
{

    if (brightness < 0)
	dim = 0;
    else if (brightness > 3)
	dim = 3;
    else
	dim = brightness;

    drv_FutabaVFD_writeChar(0x04);
    drv_FutabaVFD_writeChar((dim < 3) ? (1 + dim) * 0x20 : 0xFF);
}


static void drv_FutabaVFD_test(int test)
{
    drv_generic_parport_control(SIGNAL_TEST, (test) ? 0 : 0xFF);
}



/* start text mode display */
static int drv_FutabaVFD_start(const char *section)
{

    int brightness;
    int rows = -1, cols = -1;
    char *s;


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
    if (drv_FutabaVFD_open(section) < 0)
	return -1;


    if (cfg_number(section, "Brightness", 0, 0, 255, &brightness) > 0)
	drv_FutabaVFD_brightness(brightness);


    drv_FutabaVFD_clear();	/* clear display */

    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_brightness(RESULT * result, const int argc, RESULT * argv[])
{

    double d = dim;

    switch (argc) {
    case 0:
	SetResult(&result, R_NUMBER, &d);
	break;
    case 1:
	drv_FutabaVFD_brightness(R2N(argv[0]));
	d = dim;
	SetResult(&result, R_NUMBER, &d);
	break;
    default:
	error("%s::brightness(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
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
int drv_FutabaVFD_list(void)
{
    printf("Futaba VFD M402SD06GL");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_FutabaVFD_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;


    info("%s: %s", Name, "$Rev: 0.0.1 $");


    XRES = 5;			/* pixel width of one char  */
    YRES = 7;			/* pixel height of one char  */
    GOTO_COST = 2;		/* number of bytes a goto command requires */


    /* real worker functions */
    drv_generic_text_real_write = drv_FutabaVFD_write;
    drv_generic_text_real_defchar = drv_FutabaVFD_defchar;


    /* start display */
    if ((ret = drv_FutabaVFD_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "---")) {
	    sleep(3);
	    drv_FutabaVFD_clear();
	}
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic icon driver TODO */

    if ((ret = drv_generic_text_icon_init()) != 0)
	return ret;


    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(0)) != 0)
	return ret;

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 255, '_');	/* ASCII  32 = blank */
    drv_generic_text_bar_add_segment(255, 255, 255, 0x7F);	/* 0x7F = full dot */


    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register icon widget TODO */
    wc = Widget_Icon;
    wc.draw = drv_generic_text_icon_draw;
    widget_register(&wc);


    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);

    /* register plugins */
    AddFunction("LCD::brightness", -1, plugin_brightness);



    return 0;
}


/* close driver & display */
/* use this function for a text display */
int drv_FutabaVFD_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    drv_FutabaVFD_clear();

    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_FutabaVFD_close();

    return (0);
}

/* use this one for a text display */
DRIVER drv_FutabaVFD = {
    .name = Name,
    .list = drv_FutabaVFD_list,
    .init = drv_FutabaVFD_init,
    .quit = drv_FutabaVFD_quit,
};
