/* $Id$
 * $URL$
 *
 * Driver for Electronic Assembly serial graphic display
 *
 * Copyright (C) 2007 Stefan Gmeiner <stefangmeiner@solnet.ch>
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
 * Electronic Assembly RS232 Graphic Displays
 *
 * This driver supports some of the Electronic Assembly serial graphic displays. Although
 * most of this display support higher level operation like text and graphic command, non of
 * these are used. Instead the lcd4linux graphic routines creates the graphic which is then
 * transferd to the display.
 * 
 * FIXME: Some display have addition features such as GPI which are not yet implemented.
 *
 * Display          Protocol   Output  Contrast
 * ============================================
 * GE120-5NV24         1         8       yes
 * GE128-6N3V24        1         8       no
 * GE128-6N9V24        2         0       yes
 * GE128-7KV24         3         8       no
 * GE240-6KV24         3         8       no
 * GE240-6KCV24        3         8       no
 * GE240-7KV24         3         8       no
 * GE240-7KLWV24       3         8       no
 * GE240-6KLWV24       3         8       no
 *
 * Supported protocol commands:
 *
 *            Clear
 * Protocol  Display  Set Output     Set Contrast    Bitmap                    Orientation
 * =======================================================================================
 *    1      DL       Y(0..7)(0..1)  K(0..20)        U(x)(y)(w)(h)(...)        Vertical
 *    2      <ESC>DL  --             <ESC>DK(0..63)  <ESC>UL(x)(y)(w)(h)(...)  Vertical
 *    3      DL       Y(0..7)(0..1)  --              U(x)(y)(w)(h)(...)        Horizontal
 *
 * Bitmap orientation:
 *
 * Vertical:        Byte-No.
 *                 123456789....
 *           Bit 0 *********
 *           Bit 1 *********
 *           Bit 2 *********
 *           Bit 3 *********
 *           Bit 4 *********
 *           Bit 5 *********
 *           Bit 6 *********
 *           Bit 7 *********
 *
 * Horizontal:      Bit-No.
 *                 76543210
 *          Byte 0 ********
 *          Byte 1 ********
 *          Byte 3 ********
 *          ...
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_EA232graphic
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>

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

/* graphic display */
#include "drv_generic_graphic.h"

/* serial port */
#include "drv_generic_serial.h"

/* GPO */
#include "drv_generic_gpio.h"



#define ESC ((char)27)
#define MSB_BYTE 0x80
#define LSB_BYTE 0x01

static char Name[] = "EA232graphic";

typedef struct {
    char *name;
    int columns;
    int rows;
    int max_contrast;
    int default_contrast;
    int gpo;
    int protocol;
} MODEL;

static MODEL Models[] = {
    /* Protocol 1 models */
    {"GE120-5NV24", 120, 32, 20, 8, 8, 1},
    {"GE128-6N3V24", 128, 64, 0, 0, 8, 1},

    /* Protocol 2 models */
    {"GE128-6N9V24", 128, 64, 63, 40, 0, 2},

    /* Protocol 3 models */
    {"GE128-7KV24", 128, 128, 0, 0, 8, 3},
    {"GE240-6KV24", 240, 64, 0, 0, 8, 3},
    {"GE240-6KCV24", 240, 64, 0, 0, 8, 3},
    {"GE240-7KV24", 240, 128, 0, 0, 8, 3},
    {"GE240-7KLWV24", 240, 128, 0, 0, 8, 3},
    {"GE240-6KLWV24", 240, 64, 0, 0, 8, 3},

    {NULL, 0, 0, 0, 0, 0, 0}
};

static MODEL *Model;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


static int drv_EA232graphic_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */
    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_EA232graphic_close(void)
{
    drv_generic_serial_close();

    return 0;
}


/* write data to the display */
static void drv_EA232graphic_send(const char *data, const int len)
{
    drv_generic_serial_write(data, len);
}


/* delete Display */
static void drv_EA232graphic_clear_display()
{
    char cmd[3];

    switch (Model->protocol) {
    case 1:
    case 3:
	cmd[0] = 'D';
	cmd[1] = 'L';
	drv_EA232graphic_send(cmd, 2);
	break;

    case 2:
	cmd[0] = ESC;
	cmd[1] = 'D';
	cmd[2] = 'L';
	drv_EA232graphic_send(cmd, 3);
	break;
    default:
	error("%s: undefined protocol type", Name);
	return;
    }
}



/* copy framebuffer to display */
static void drv_EA232graphic_blit(const int row, const int col, const int height, const int width)
{

    int r, c, l, p;
    char *cmd;

    /* calculate length of command */
    l = 0;
    switch (Model->protocol) {
    case 1:
    case 2:
	l = ((height + 7) / 8) * width;
	break;
    case 3:
	l = ((width + 7) / 8) * height;
	break;
    default:
	error("%s: undefined protocol type", Name);
	return;
    }

    /* add maximum length of command header */
    cmd = (char *) malloc(l + 10);
    if (cmd == NULL) {
	error("%s: allocation of buffer failed: %s", Name, strerror(errno));
	return;
    }
    p = 0;

    /* write command header */
    switch (Model->protocol) {
    case 1:
    case 3:
	cmd[p++] = 'U';
	cmd[p++] = col;
	cmd[p++] = row;
	cmd[p++] = width;
	cmd[p++] = height;
	break;
    case 2:
	cmd[p++] = ESC;
	cmd[p++] = 'U';
	cmd[p++] = 'L';
	cmd[p++] = col;
	cmd[p++] = row;
	cmd[p++] = width;
	cmd[p++] = height;
	break;
    default:
	error("%s: undefined protocol type", Name);
	free(cmd);
	return;
    }

    /* clear all pixels */
    memset(cmd + p, 0, l);

    /* set pixels */
    switch (Model->protocol) {
    case 1:
    case 2:
	for (r = 0; r < height; r++) {
	    for (c = 0; c < width; c++) {
		if (drv_generic_graphic_black(r + row, c + col)) {
		    cmd[(r / 8) * width + c + p] |= (LSB_BYTE << (r % 8));
		}
	    }
	}
	break;
    case 3:
	for (c = 0; c < width; c++) {
	    for (r = 0; r < height; r++) {
		if (drv_generic_graphic_black(r + row, c + col)) {
		    cmd[(c / 8) * height + r + p] |= (MSB_BYTE >> (c % 8));
		}
	    }
	}
	break;
    default:
	error("%s: undefined protocol type", Name);
	free(cmd);
	return;
    }

    drv_EA232graphic_send(cmd, p + l);

    free(cmd);
}


static int drv_EA232graphic_GPO(const int num, const int val)
{
    char cmd[4];

    if (Model->gpo == 0) {
	error("%s: GPO not supported on this model.", Name);
	return -1;
    }


    if (Model->gpo < num) {
	error("%s: GPO %d is not available.", Name, num);
	return -1;
    }

    cmd[0] = 'Y';
    cmd[1] = num;
    cmd[2] = (val > 0) ? 1 : 0;

    drv_EA232graphic_send(cmd, 3);

    return 0;
}


/* adjust contast */
static int drv_EA232graphic_contrast(int contrast)
{
    char cmd[4];

    if (Model->max_contrast == 0) {
	error("%s: contrast setting not support by model", Name);
	return -1;
    }

    /* adjust limits according to the display */
    if (contrast < 0)
	contrast = 0;
    if (contrast > Model->max_contrast)
	contrast = Model->max_contrast;


    switch (Model->protocol) {
    case 1:
	cmd[0] = 'K';
	cmd[1] = contrast;
	drv_EA232graphic_send(cmd, 2);
	break;
    case 2:
	cmd[0] = ESC;
	cmd[1] = 'D';
	cmd[2] = 'K';
	cmd[3] = contrast;
	drv_EA232graphic_send(cmd, 4);
	break;
    default:
	error("%s: undefined protocol type", Name);
	return -1;
    }

    return contrast;
}


/* start graphic display */
static int drv_EA232graphic_start(const char *section)
{
    char *s;
    int contrast;
    int i;

    /* read model from configuration */
    s = cfg_get(section, "Model", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
	return -1;
    }

    for (i = 0; Models[i].name != NULL; i++) {
	if (strcasecmp(Models[i].name, s) == 0)
	    break;
    }
    if (!Models[i].name) {
	error("%s: %s.Model '%s' is unknown from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    Model = &Models[i];
    info("%s: using model '%s'", Name, Model->name);
    free(s);

    /* read display size from model configuration */
    DROWS = Model->rows;
    DCOLS = Model->columns;

    /* set number of GPO's from model */
    GPOS = Model->gpo;

    /* read font size from configuration */
    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	free(s);
	return -1;
    }

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    /* open communication with the display */
    if (drv_EA232graphic_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    drv_EA232graphic_clear_display();

    if (cfg_number(section, "Contrast", Model->default_contrast, 0, Model->max_contrast, &contrast) > 0) {
	drv_EA232graphic_contrast(contrast);
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_EA232graphic_contrast(R2N(arg1));
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
int drv_EA232graphic_list(void)
{
    int i;

    for (i = 0; Models[i].name; i++) {
	printf("%s ", Models[i].name);
    }

    return 0;
}


/* initialize driver & display */
int drv_EA232graphic_init(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_EA232graphic_blit;
    drv_generic_gpio_real_set = drv_EA232graphic_GPO;

    /* start display */
    if ((ret = drv_EA232graphic_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    /* initialize GPO */
    if (Model->gpo > 0 && (ret = drv_generic_gpio_init(section, Name)) != 0)
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

    return 0;
}

/* close driver & display */
int drv_EA232graphic_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    if (Model->gpo > 0)
	drv_generic_gpio_quit();

    debug("%s: closing connection", Name);
    drv_EA232graphic_close();

    return (0);
}

/* use this one for a graphic display */
DRIVER drv_EA232graphic = {
    .name = Name,
    .list = drv_EA232graphic_list,
    .init = drv_EA232graphic_init,
    .quit = drv_EA232graphic_quit,
};
