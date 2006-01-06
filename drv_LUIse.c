/* $Id: drv_LUIse.c,v 1.2 2006/01/06 16:56:49 tooly-bln Exp $
 *
 * LUIse lcd4linux driver
 *
 * Copyright (C) 2005 Theo Schneider <theo@schneider-berlin.net>
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
 * $Log: drv_LUIse.c,v $
 * Revision 1.2  2006/01/06 16:56:49  tooly-bln
 * *** empty log message ***
 *
 * Revision 1.1  2006/01/03 13:20:06  reinelt
 * LUIse driver added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_LUIse
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <usb.h>
#include <luise.h>

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

static char Name[] = "LUIse";

/* default Wert */
static int devNum = 0;



/****************************************/
/***  hardware dependant functions    ***/
/****************************************/
static void drv_LUIse_clear(void)
{
    unsigned char buf[9600];
    int x;

    // clear text
    LUI_Text(devNum, 0, 0, 320, 240, 0, 0, 1, 1, "");

    // clear picture
    for (x = 0; x < 9600; x++)
	buf[x] = 0x00;
    LUI_Bitmap(devNum, 0, 0, 0, 0, 0, DCOLS, DROWS, DCOLS, DROWS, buf);
    LUI_Bitmap(devNum, 1, 0, 0, 0, 0, DCOLS, DROWS, DCOLS, DROWS, buf);
}


static void drv_LUIse_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    if (drv_generic_graphic_FB[r * LCOLS + c]) {
		LUI_SetPixel(devNum, 0, c, r, 1);
	    } else {
		LUI_SetPixel(devNum, 0, c, r, 0);
	    }
	}
    }
}

static int drv_LUIse_contrast(int contrast)
{
    /* adjust limits according to the display */
    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    LUI_SetContrast(devNum, contrast);

    return contrast;
}

static int drv_LUIse_backlight(int backlight)
{
    if (backlight < 0)
	backlight = 0;
    if (backlight > 1)
	backlight = 1;

    LUI_CCFL(devNum, backlight);

    return backlight;
}


/* start graphic display */
static int drv_LUIse_start(const char *section)
{
    char *s;
    int gfxmode, gfxinvert, ScreenRotation, IOrefresh;
    int contrast, backlight;

    /* read devNum from config */
    s = cfg_get(section, "DeviceNum", 0);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.DeviceNum' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%d", &devNum) < 0 || devNum > 4) {
	error("s: bad DeviceNum '%s' from %s", Name, s, cfg_source());
	return -1;
    }
    info("%s: using DeviceNum '%d'", Name, devNum);

    /* open communication with the display */
    if (LUI_OpenDevice(devNum) > 0) {
	error("unable to open DeviceNum: %d", devNum);
	return -1;
    }

    /*
     * 0 : gfxmode              0 = or, 1 = and, 2 = xor
     * 0 : gfxinvert            0 = normal, 1 = invert
     * 0 : ScreenRotation       0 =, 1 =, 2 =, 3 =, 
     * 2 : IOrefresh            0 = 25ms...255=256*25ms
     */

    s = cfg_get(section, "Mode", "0.0.0.2");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Mode' entry from %s", Name, section, cfg_source());
	return -1;
    }

    if (sscanf(s, "%d.%d.%d.%d", &gfxmode, &gfxinvert, &ScreenRotation, &IOrefresh) != 4 ||
	gfxmode < 0 || gfxmode > 2 || gfxinvert < 0 || gfxinvert > 1 ||
	ScreenRotation < 0 || ScreenRotation > 255 || IOrefresh < 0 || IOrefresh > 255) {
	error("%s: bad Mode '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    if (LUI_LCDmode(devNum, gfxmode, gfxinvert, ScreenRotation, IOrefresh) > 0) {
	error("Error LUI_LCDmode");
	return -1;
    }

    switch (ScreenRotation) {
    case 0:{
	    DCOLS = 320;
	    DROWS = 240;
	    break;
	}
    case 1:{
	    DCOLS = 240;
	    DROWS = 320;
	    break;
	}
    case 2:{
	    DCOLS = 320;
	    DROWS = 240;
	    break;
	}
    case 3:{
	    DCOLS = 240;
	    DROWS = 320;
	    break;
	}
    }

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

    if (cfg_number(section, "Contrast", 128, 0, 255, &contrast) > 0) {
	drv_LUIse_contrast(contrast);
    }

    if (cfg_number(section, "Backlight", 0, 0, 1, &backlight) > 0) {
	drv_LUIse_backlight(backlight);
    }

    s = cfg_get(section, "Backpicture", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Backpicture' entry from %s", Name, section, cfg_source());
    } else {
	drv_LUIse_clear();
	if (LUI_BMPfile(devNum, 1, 0, 0, 0, 0, DCOLS, DROWS, s)) {
	    error("%s: Sorry unable to load: %s", Name, s);
	    return -1;
	}
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_LUIse_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_LUIse_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}

/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_LUIse_list(void)
{
    printf("generic");
    return 0;
}

/* initialize driver & display */
int drv_LUIse_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_LUIse_blit;

    /* start display */
    if ((ret = drv_LUIse_start(section)) != 0)
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
    AddFunction("LCD::contrast", 1, plugin_contrast);
    AddFunction("LCD::backlight", 1, plugin_backlight);

    return 0;
}

/* close driver & display */
/* use this function for a graphic display */
int drv_LUIse_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    /* clear display */
    drv_LUIse_clear();

    /* set default for Contrast, ScreenRotation, gfxmode, gfxinvert, IOrefresh */
    LUI_SetContrast(devNum, 128);
    LUI_LCDmode(devNum, 0, 0, 0, 2);


    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    debug("closing connection");
    LUI_CloseDevice(devNum);

    return (0);
}

/* use this one for a graphic display */
DRIVER drv_LUIse = {
  name:Name,
  list:drv_LUIse_list,
  init:drv_LUIse_init,
  quit:drv_LUIse_quit,
};
