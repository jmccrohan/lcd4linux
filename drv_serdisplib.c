/* $Id$
 * $URL$
 *
 * driver for serdisplib displays
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
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
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_serdisplib
 *
 */

#include "config.h"
#include "debug.h"		// verbose_level

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
#include "drv.h"
#include "drv_generic_graphic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[] = "serdisplib";

static serdisp_CONN_t *sdcd;
static serdisp_t *dd;

int NUMCOLS = 1;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_SD_blit(const int row, const int col, const int height, const int width)
{
    int r, c;
    RGBA p;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    p = drv_generic_graphic_rgb(r, c);
	    // printf("blit (%d,%d) A%d.R%d.G%d.B%d\n", c, r, p.A, p.R, p.G, p.B);
	    serdisp_setcolour(dd, c, r, serdisp_pack2ARGB(0xff, p.R, p.G, p.B));
	}
    }

    serdisp_update(dd);
}


static int drv_SD_contrast(int contrast)
{
    if (contrast < 0)
	contrast = 0;
    if (contrast > MAX_CONTRASTSTEP)
	contrast = MAX_CONTRASTSTEP;

    serdisp_feature(dd, FEATURE_CONTRAST, contrast);

    return contrast;
}


static int drv_SD_backlight(int backlight)
{
    if (backlight < FEATURE_NO)
	backlight = FEATURE_NO;
    if (backlight > FEATURE_YES)
	backlight = FEATURE_YES;

    serdisp_feature(dd, FEATURE_BACKLIGHT, backlight);

    return backlight;
}


static int drv_SD_reverse(int reverse)
{
    if (reverse < FEATURE_NO)
	reverse = FEATURE_NO;
    if (reverse > FEATURE_YES)
	reverse = FEATURE_YES;

    serdisp_feature(dd, FEATURE_REVERSE, reverse);

    return reverse;
}


static int drv_SD_rotate(int rotate)
{
    if (rotate < 0)
	rotate = 0;
    if (rotate > 3)
	rotate = 3;

    serdisp_feature(dd, FEATURE_ROTATE, rotate);

    return rotate;
}



static int drv_SD_start(const char *section)
{
    long version;
    char *port, *model, *options, *s;
    int contrast, backlight, reverse, rotate;

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
    NUMCOLS = serdisp_getcolours(dd);
    info("%s: display size %dx%d, %d colors", Name, DCOLS, DROWS, NUMCOLS);

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

    if (cfg_number(section, "Contrast", 0, 0, MAX_CONTRASTSTEP, &contrast) > 0) {
	drv_SD_contrast(contrast);
    }

    if (cfg_number(section, "Backlight", 0, 0, 1, &backlight) > 0) {
	drv_SD_backlight(backlight);
    }

    if (cfg_number(section, "Reverse", 0, 0, 1, &reverse) > 0) {
	drv_SD_reverse(reverse);
    }

    if (cfg_number(section, "Rotate", 0, 0, 3, &rotate) > 0) {
	drv_SD_rotate(rotate);
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_SD_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}


static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_SD_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}


static void plugin_reverse(RESULT * result, RESULT * arg1)
{
    double reverse;

    reverse = drv_SD_reverse(R2N(arg1));
    SetResult(&result, R_NUMBER, &reverse);
}


static void plugin_rotate(RESULT * result, RESULT * arg1)
{
    double rotate;

    rotate = drv_SD_rotate(R2N(arg1));
    SetResult(&result, R_NUMBER, &rotate);
}


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_SD_list(void)
{
    serdisp_display_t displaydesc;

    if (verbose_level > 0) {
	printf("  version %i.%i, supported displays:\n",
	       SERDISP_VERSION_GET_MAJOR(SERDISP_VERSION_CODE), SERDISP_VERSION_GET_MINOR(SERDISP_VERSION_CODE));
	displaydesc.dispname = "";
	printf("    display name     alias names           description\n");
	printf("    ---------------  --------------------  -----------------------------------\n");
	while (serdisp_nextdisplaydescription(&displaydesc)) {
	    printf("    %-15s  %-20s  %-35s\n", displaydesc.dispname, displaydesc.aliasnames, displaydesc.description);
	}
    } else {
	displaydesc.dispname = "";
	while (serdisp_nextdisplaydescription(&displaydesc)) {
	    printf("%s ", displaydesc.dispname);
	}
	printf("\n     (use -vl to see detailed list of serdisplib)");
    }

    return 0;
}


/* initialize driver & display */
int drv_SD_init(const char *section, const int quiet)
{
    int ret;

    info("%s: %s", Name, "$Rev$");

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

    /* register plugins */
    AddFunction("LCD::contrast", 1, plugin_contrast);
    AddFunction("LCD::backlight", 1, plugin_backlight);
    AddFunction("LCD::reverse", 1, plugin_reverse);
    AddFunction("LCD::rotate", 1, plugin_rotate);

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
    .name = Name,
    .list = drv_SD_list,
    .init = drv_SD_init,
    .quit = drv_SD_quit,
};
