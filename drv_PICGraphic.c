/* $Id$
 * $URL$
 *
 * PICGraphic lcd4linux driver
 *
 * Copyright (C) 2009 Peter Bailey <peter.eldridge.bailey@gmail.com>
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
 * struct DRIVER drv_PICGraphic
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
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
#include "timer.h"

#include "drv_generic_graphic.h"
#include "drv_generic_gpio.h"
#include "drv_generic_serial.h"

#define min(a,b) (a < b ? a : b)

static char Name[] = "PICGraphic";

//#define partialFrame

/* example config:

Display PICGraphic {
    Driver    'PICGraphic'
    Port      '/dev/ttyS1'
    Contrast  8
# these could be automatic
    Size      '48x128'
    Speed     115200
    Font      '6x8'
}

*/

/*
typedef struct {
    char *name;
    int columns;
    int rows;
    int max_contrast;
    int default_contrast;
    int gpo;
    int protocol;
} MODEL;
*/

static char *fbPG = 0, delayDone = 0;

void drv_PICGraphic_1s(void *arg)
{
    delayDone = 1;
    info("delay done");
}


/****************************************/
/***  hardware-dependent functions    ***/
/****************************************/
static int drv_PICGraphic_open(const char *section)
{
    /* open serial port */
    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_PICGraphic_close(void)
{
    /* close opened serial port */
    return drv_generic_serial_close();
}

static void convert2ASCII(char input, char * output)
{
	unsigned char temp = input >> 4;
	if(temp < 10)
		output[0] = temp + '0';
	else
		output[0] = temp - 10 + 'a';
	input &= 0xf;
	if(input < 10)
		output[1] = input + '0';
	else
		output[1] = input - 10 + 'a';
}


static void drv_PICGraphic_send(const char *data, const unsigned int len)
{
    unsigned int i;
    char hexDigits[3];
    hexDigits[2] = 0;
    drv_generic_serial_write(data, len);
    info("sending %d bytes: ", len);
    for(i = 0; i < len; i++){ // min(10, len)
    	convert2ASCII(data[i], hexDigits);
    	info("0x%s (%c)", hexDigits, data[i]);
    }
}

static void drv_PICGraphic_blit(const int row, const int col, const int height, const int width)
{
    /* update a rectangular portion of the display */
    int r, c, index;
    unsigned char cmd[5];

    info("blit from (%d,%d) to (%d,%d) out of (%d,%d)", row, col, row + height, col + width, DROWS, DCOLS);
    if (!fbPG)
	return;
    for (c = min(col, DCOLS - 1); c < min(col + width, DCOLS); c++) {
	for (r = min(row, DROWS - 1); r < min(row + height, DROWS); r++) {
	    index = DCOLS * (r / 8) + c;
	    if (index < 0 || index >= DCOLS * DROWS / 8) {
		error("index too large: %d, r: %d, c: %d", index, r, c);
		break;
	    }
	    if (drv_generic_graphic_black(r, c)) {
		fbPG[index] |= (1 << (r % 8));
	    } else {
		fbPG[index] &= ~(1 << (r % 8));
	    }
	}
    }

    // send rectangular portion with height divisible by 8
#ifdef partialFrame
    if (delayDone) {
	delayDone = 0;
	int row8, height8;
	row8 = 8*(row/8);
	height8 = 8*(height/8) + !!(height % 8);
	info("sending blit");
	cmd[0] = 'b';
	cmd[1] = row8;
	cmd[2] = col;
	cmd[3] = height8;
	cmd[4] = width;
	drv_PICGraphic_send(cmd, 5);
	for (r = min(row8, DROWS - 1); r < min(row8 + height8, DROWS); r+=8) {
		drv_PICGraphic_send(fbPG+DCOLS * (r / 8) + col, width);
	}
    }
#else
    // send full frame
    if (delayDone) {
	delayDone = 0;
	info("sending frame");
	cmd[0] = 'f';
	drv_PICGraphic_send(cmd, 1);
	drv_PICGraphic_send(fbPG, DROWS * DCOLS / 8);
    }
#endif
}

static int drv_PICGraphic_GPO(const int num, const int val)
{
    char cmd[3];

    cmd[0] = 'g';
    cmd[1] = val ? 's' : 'c';
    cmd[2] = num;

    // todo: fixme
//    drv_PICGraphic_send(cmd, 3);

    return 0;
}

static int drv_PICGraphic_GPI(const int num)
{
    char cmd[3];
    int ret = 0;

    cmd[0] = 'g';
    cmd[1] = 'r';
    cmd[2] = num;

    // todo: fixme
//    drv_PICGraphic_send(cmd, 3);
//    ret = drv_generic_serial_read(cmd, 1);

    if (ret)
	return -1;
    else
	return cmd[0];
}

/* example function used in a plugin */
static int drv_PICGraphic_contrast(int contrast)
{
    char cmd[2];

    /* adjust limits according to the display */
    if (contrast < 0)
	contrast = 0;
    if (contrast > 15)
	contrast = 15;

    /* call a 'contrast' function */
    cmd[0] = 'c';
    cmd[1] = contrast;
    // todo: fixme
//    drv_PICGraphic_send(cmd, 2);

    return contrast;
}

/* start graphic display */
static int drv_PICGraphic_start2(const char *section)
{
    char *s;
    char cmd[1];
    int contrast;

    /* read display size from config */
    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }

    DROWS = -1;
    DCOLS = -1;
    if (sscanf(s, "%dx%d", &DCOLS, &DROWS) != 2 || DCOLS < 1 || DROWS < 1) {
	error("%s: bad Size '%s' from %s", Name, s, cfg_source());
	return -1;
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

    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    /* you surely want to allocate a framebuffer or something... */
    fbPG = calloc(DCOLS * DROWS / 8, 1);
    if (!fbPG) {
	error("failed to allocate framebuffer");
	return -1;
    }

    info("allocated framebuffer with size %d", DCOLS * DROWS / 8);

    timer_add(drv_PICGraphic_1s, 0, 500, 0);	// 1s

    /* open communication with the display */
    if (drv_PICGraphic_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    cmd[0] = 'i';
    drv_PICGraphic_send(cmd, 1);

    if (cfg_number(section, "Contrast", 8, 0, 15, &contrast) > 0) {
	drv_PICGraphic_contrast(contrast);
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_PICGraphic_contrast(R2N(arg1));
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
int drv_PICGraphic_list(void)
{
    printf("PICGraphic serial-to-graphic by Peter Bailey");
    return 0;
}

/* initialize driver & display */
int drv_PICGraphic_init2(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_PICGraphic_blit;
    drv_generic_gpio_real_set = drv_PICGraphic_GPO;
    drv_generic_gpio_real_get = drv_PICGraphic_GPI;

    /* start display */
    if ((ret = drv_PICGraphic_start2(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();	// also clears main framebuffer
	}
    }

    /* register plugins */
    AddFunction("LCD::contrast", 1, plugin_contrast);

    return 0;
}

/* close driver & display */
/* use this function for a graphic display */
int drv_PICGraphic_quit2(const int quiet)
{

    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    drv_generic_gpio_quit();


    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    info("freeing framebuffer");
    free(fbPG);

    drv_generic_graphic_quit();

    debug("closing connection");
    drv_PICGraphic_close();

    return (0);
}

/* use this one for a graphic display */
DRIVER drv_PICGraphic = {
    .name = Name,
    .list = drv_PICGraphic_list,
    .init = drv_PICGraphic_init2,
    .quit = drv_PICGraphic_quit2,
};
