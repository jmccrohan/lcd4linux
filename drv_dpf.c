/* $Id: drv_dpf.c 980 2009-01-28 21:18:52Z michux $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_dpf.c $
 *
 * Very basic hacked picture frame driver. Uses external libdpf.
 * This is a first working approach for AX206 based DPFs. In future,
 * more DPFs might be covered by that library. Work in progress.
 *
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
 * struct DRIVER drv_DPF
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libdpf/libdpf.h>

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

static char Name[] = "DPF";

static DPFContext *g_h;

/* Display data */
static unsigned char *g_fb;

static int drv_dpf_open(const char *section)
{
    int error;
    char *dev;

    // Currently, the Port specification is unused.

    dev = cfg_get(section, "Port", NULL);
    if (dev == NULL || *dev == '\0') {
	error("dpf: no '%s.Port' entry from %s", section, cfg_source());
	return -1;
    }

    error = dpf_open(NULL, &g_h);
    if (error < 0) {
	error("dpf: cannot open dpf device %s", dev);
	return -1;
    }

    return 0;
}


static int drv_dpf_close(void)
{
    dpf_close(g_h);

    return 0;
}

#define _RGB565_0(p) \
	(( ((p.R) & 0xf8)      ) | (((p.G) & 0xe0) >> 5))
#define _RGB565_1(p) \
	(( ((p.G) & 0x1c) << 3 ) | (((p.B) & 0xf8) >> 3))

static void drv_dpf_blit(const int row, const int col, const int height, const int width)
{
    int r, c;
    short rect[4];
    unsigned long i;
    RGBA p;
    unsigned char *pix;

    pix = g_fb;
    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    p = drv_generic_graphic_rgb(r, c);
	    *pix++ = _RGB565_0(p);
	    *pix++ = _RGB565_1(p);
	}
    }
    rect[0] = col;
    rect[1] = row;
    rect[2] = col + width;
    rect[3] = row + height;
    dpf_screen_blit(g_h, g_fb, rect);
}


/* start graphic display */
static int drv_dpf_start2(const char *section)
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
    if (drv_dpf_open(section) < 0) {
	return -1;
    }

    /* you surely want to allocate a framebuffer or something... */
    g_fb = malloc(g_h->height * g_h->width * g_h->bpp);

    /* set width/height from dpf firmware specs */
    DROWS = g_h->height;
    DCOLS = g_h->width;

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    int bl_on;
    bl_on = (R2N(arg1) == 0 ? 0 : 1);
    dpf_backlight(g_h, bl_on);
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
int drv_dpf_list(void)
{
    printf("generic hacked photo frame");
    return 0;
}


/* initialize driver & display */
int drv_dpf_init2(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_dpf_blit;

    /* start display */
    if ((ret = drv_dpf_start2(section)) != 0)
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
int drv_dpf_quit2(const int quiet)
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
    drv_dpf_close();

    return (0);
}


DRIVER drv_DPF = {
    .name = Name,
    .list = drv_dpf_list,
    .init = drv_dpf_init2,
    .quit = drv_dpf_quit2,
};
