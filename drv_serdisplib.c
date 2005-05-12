/* $Id: drv_serdisplib.c,v 1.3 2005/05/12 05:52:43 reinelt Exp $
 *
 * driver for serdisplib displays
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_serdisplib.c,v $
 * Revision 1.3  2005/05/12 05:52:43  reinelt
 * serdisplib GET_VERSION_MAJOR macro
 *
 * Revision 1.2  2005/05/11 04:27:49  reinelt
 * small serdisplib bugs fixed
 *
 * Revision 1.1  2005/05/10 13:20:14  reinelt
 * added serdisplib driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_serdisplib
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <serdisplib/serdisp.h>

/* Fixme: This should be removed as soon as serdisp.h 
 * contains this macros
 */
#ifndef SERDISP_VERSION_GET_MAJOR
#define SERDISP_VERSION_GET_MAJOR(_c)  ((int)( (_c) >> 8 ))
#define SERDISP_VERSION_GET_MINOR(_c)  ((int)( (_c) & 0xFF ))
#endif


#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_graphic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[] = "serdisplib";

static serdisp_CONN_t *sdcd;
static serdisp_t *dd;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_SD_blit(const int row, const int col, const int height, const int width)
{
    int r, c;
    long color;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    color = drv_generic_graphic_FB[r * LCOLS + c] ? SD_COL_BLACK : SD_COL_WHITE;
	    serdisp_setcolour(dd, c, r, color);
	}
    }

    serdisp_update(dd);
}


static int drv_SD_start(const char *section)
{
    long version;
    char *port, *model, *options, *s;

    version = serdisp_getversioncode();
    info("%s: header  version %d.%d", Name, SERDISP_VERSION_MAJOR, SERDISP_VERSION_MINOR);
    info("%s: library version %d.%d", Name, SERDISP_VERSION_GET_MAJOR(version), SERDISP_VERSION_GET_MINOR(version));

    port = cfg_get(section, "Port", NULL);
    if (port == NULL || *port == '\0') {
	error("%s: no '%s.Port' entry from %s", Name, section, cfg_source());
	return -1;
    }

    /* opening the output device */
    sdcd = SDCONN_open(port);
    if (sdcd == NULL) {
	error("%s: open(%s) failed: %s", Name, port, sd_geterrormsg());
	return -1;
    }


    model = cfg_get(section, "Model", "");
    if (model == NULL || *model == '\0') {
	error("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
	return -1;
    }
    info("%s: using model '%s'", Name, model);

    options = cfg_get(section, "Options", "");
    info("%s: using options '%s'", Name, options);

    /* opening and initialising the display */
    dd = serdisp_init(sdcd, model, options);
    if (dd == NULL) {
	error("%s: init(%s, %s, %s) failed: %s", Name, port, model, options, sd_geterrormsg());
	SDCONN_close(sdcd);
	return -1;
    }

    DROWS = serdisp_getheight(dd);
    DCOLS = serdisp_getwidth(dd);
    info("%s: display size %dx%d", Name, DCOLS, DROWS);

    XRES = -1;
    YRES = -1;
    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    /* clear display */
    serdisp_clear(dd);

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */
/* Fixme: SD_FEATURE's */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_graphic_draw(W) */
/* using drv_generic_graphic_icon_draw(W) */
/* using drv_generic_graphic_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_SD_list(void)
{
    printf("%s", "any");
    return 0;
}


/* initialize driver & display */
int drv_SD_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_SD_blit;

    /* start display */
    if ((ret = drv_SD_start(section)) != 0)
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

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_graphic_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_graphic_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_graphic_bar_draw;
    widget_register(&wc);

    /* register plugins */
    /* none at the moment... */
    /* Fixme: SD_FEATURE's */

    return 0;
}


/* close driver & display */
int drv_SD_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_graphic_clear();

    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    serdisp_quit(dd);

    return (0);
}


DRIVER drv_serdisplib = {
  name:Name,
  list:drv_SD_list,
  init:drv_SD_init,
  quit:drv_SD_quit,
};
