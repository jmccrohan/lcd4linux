/* $Id$
 * $URL$
 *
 * ST2205U-driven hacked picture frame driver.
 * See http://picframe.spritesserver.nl/ for more info.
 * 
 * Copyright (C) 2008 Jeroen Domburg <picframe@spritesmods.com>
 * Modified from sample code by:
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
 * struct DRIVER drv_st2205
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <st2205.h>

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

#include "drv_generic_graphic.h"

static char Name[] = "st2205";
/* libst2205 handle */
static st2205_handle *h;
/* Display data */
static unsigned char *fb;

static int drv_st2205_open(const char *section)
{
    char *dev;

    dev = cfg_get(section, "Port", NULL);
    if (dev == NULL || *dev == '\0') {
	error("st2205: no '%s.Port' entry from %s", section, cfg_source());
	return -1;
    }

    h = st2205_open(dev);
    if (h == NULL) {
	error("st2205: cannot open st2205 device %s", dev);
	return -1;
    }

    return 0;
}


static int drv_st2205_close(void)
{
    st2205_close(h);

    return 0;
}


static void drv_st2205_blit(const int row, const int col, const int height, const int width)
{
    int r, c;
    RGBA p;
    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    p = drv_generic_graphic_rgb(r, c);
	    fb[(r * h->width + c) * 3 + 0] = p.R;
	    fb[(r * h->width + c) * 3 + 1] = p.G;
	    fb[(r * h->width + c) * 3 + 2] = p.B;
	}
    }
    st2205_send_data(h, fb);
}


/* start graphic display */
static int drv_st2205_start2(const char *section)
{
    char *s;

    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }


    /* open communication with the display */
    if (drv_st2205_open(section) < 0) {
	return -1;
    }

    /* you surely want to allocate a framebuffer or something... */
    fb = malloc(h->height * h->width * 3);

    /* set width/height from st2205 firmware specs */
    DROWS = h->width;
    DCOLS = h->height;

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    int bl_on;
    bl_on = (R2N(arg1) == 0 ? 0 : 1);
    st2205_backlight(h, bl_on);
    SetResult(&result, R_NUMBER, &bl_on);
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
int drv_st2205_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_st2205_init2(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_st2205_blit;

    /* start display */
    if ((ret = drv_st2205_start2(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    /* register plugins */
    AddFunction("LCD::backlight", 1, plugin_backlight);

    return 0;
}


/* close driver & display */
int drv_st2205_quit2(const int quiet)
{
    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    debug("closing connection");
    drv_st2205_close();

    return (0);
}


DRIVER drv_st2205 = {
    .name = Name,
    .list = drv_st2205_list,
    .init = drv_st2205_init2,
    .quit = drv_st2205_quit2,
};
