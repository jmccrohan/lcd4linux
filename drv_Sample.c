/* $Id$
 * $URL$
 *
 * sample lcd4linux driver
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
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Sample
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

/* graphic display? */
#include "drv_generic_graphic.h"

/* GPO's? */
#include "drv_generic_gpio.h"

/* serial port? */
#include "drv_generic_serial.h"

/* parallel port? */
#include "drv_generic_parport.h"

/* i2c bus? */
#ifdef WITH_I2C
#include "drv_generic_i2c.h"
#endif


static char Name[] = "Sample";


/* for parallel port displays only */
/* use whatever lines you need */
static unsigned char SIGNAL_RS;
static unsigned char SIGNAL_EX;



/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/* low-level parallel port stuff */
/* example for sending one byte over the wire */
static void drv_Sample_bang(const unsigned int data)
{
    /* put data on DB1..DB8 */
    drv_generic_parport_data(data & 0xff);

    /* set/clear some signals */
    drv_generic_parport_control(SIGNAL_RS, SIGNAL_RS);

    /* data setup time (e.g. 200 ns) */
    ndelay(200);

    /* send byte */
    /* signal pulse width 500ns */
    drv_generic_parport_toggle(SIGNAL_EX, 1, 500);

    /* wait for command completion (e.g. 100 us) */
    udelay(100);
}


static int drv_Sample_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;


    /* opening the parallel port is a bit more tricky: */
    /* you have to do all the bit-banging yourself... */

    if (drv_generic_parport_open(section, Name) != 0) {
	error("%s: could not initialize parallel port!", Name);
	return -1;
    }

    /* read the wiring from config */
    if ((SIGNAL_EX = drv_generic_parport_wire_ctrl("EX", "STROBE")) == 0xff)
	return -1;
    if ((SIGNAL_RS = drv_generic_parport_wire_ctrl("RS", "INIT")) == 0xff)
	return -1;

    /* clear all signals */
    drv_generic_parport_control(SIGNAL_EX | SIGNAL_RS, 0);

    /* set direction: write */
    drv_generic_parport_direction(0);

    return 0;
}


static int drv_Sample_close(void)
{
    /* close whatever port you've opened */
    drv_generic_parport_close();
    drv_generic_serial_close();

    return 0;
}


/* dummy function that sends something to the display */
static void drv_Sample_send(const char *data, const unsigned int len)
{
    unsigned int i;

    /* send data to the serial port is easy... */
    drv_generic_serial_write(data, len);

    /* sending data to the parallel port is a bit more tricky... */
    for (i = 0; i < len; i++) {
	drv_Sample_bang(*data++);
    }
}


/* text mode displays only */
static void drv_Sample_clear(void)
{
    char cmd[1];

    /* do whatever is necessary to clear the display */
    /* assume 0x01 to be a 'clear display' command */
    cmd[0] = 0x01;
    drv_Sample_send(cmd, 1);
}


/* text mode displays only */
static void drv_Sample_write(const int row, const int col, const char *data, int len)
{
    char cmd[3];

    /* do the cursor positioning here */
    /* assume 0x02 to be a 'Goto' command */
    cmd[0] = 0x02;
    cmd[1] = row;
    cmd[2] = col;
    drv_Sample_send(cmd, 3);

    /* send string to the display */
    drv_Sample_send(data, len);

}

/* text mode displays only */
static void drv_Sample_defchar(const int ascii, const unsigned char *matrix)
{
    char cmd[10];
    int i;

    /* call the 'define character' function */
    /* assume 0x03 to be the 'defchar' command */
    cmd[0] = 0x03;
    cmd[1] = ascii;

    /* send bitmap to the display */
    for (i = 0; i < 8; i++) {
	cmd[i + 2] = *matrix++;
    }
    drv_Sample_send(cmd, 10);
}


/* for graphic displays only */
static void drv_Sample_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    /* drv_generic_graphic_black() returns 1 if pixel is black */
	    /* drv_generic_graphic_gray() returns a gray value 0..255 */
	    /* drv_generic_graphic_rgb() returns a RGB color */
	    if (drv_generic_graphic_black(r, c)) {
		/* set bit */
	    } else {
		/* clear bit */
	    }
	}
    }
}


/* remove unless you have GPO's */
static int drv_Sample_GPO(const int num, const int val)
{
    char cmd[4];

    /* assume 0x42 to be the 'GPO' command */
    cmd[0] = 0x42;
    cmd[1] = num;
    cmd[2] = (val > 0) ? 1 : 0;

    drv_Sample_send(cmd, 3);

    return 0;
}


/* example function used in a plugin */
static int drv_Sample_contrast(int contrast)
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
    drv_Sample_send(cmd, 2);

    return contrast;
}


/* start text mode display */
static int drv_Sample_start(const char *section)
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

    /* number of GPO's; remove if you don't have them */
    GPOS = 8;

    /* open communication with the display */
    if (drv_Sample_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    /* assume 0x00 to be a 'reset' command */
    cmd[0] = 0x00;
    drv_Sample_send(cmd, 0);

    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_Sample_contrast(contrast);
    }

    drv_Sample_clear();		/* clear display */

    return 0;
}


/* start graphic display */
static int drv_Sample_start2(const char *section)
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

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    /* you surely want to allocate a framebuffer or something... */

    /* open communication with the display */
    if (drv_Sample_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    /* assume 0x00 to be a 'reset' command */
    cmd[0] = 0x00;
    drv_Sample_send(cmd, 1);

    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_Sample_contrast(contrast);
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_Sample_contrast(R2N(arg1));
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
int drv_Sample_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_Sample_init(const char *section, const int quiet)
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
    drv_generic_text_real_write = drv_Sample_write;
    drv_generic_text_real_defchar = drv_Sample_defchar;

    /* remove unless you have GPO's */
    drv_generic_gpio_real_set = drv_Sample_GPO;


    /* start display */
    if ((ret = drv_Sample_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.bwct.de")) {
	    sleep(3);
	    drv_Sample_clear();
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


    /* initialize generic GPIO driver */
    /* remove unless you have GPO's */
    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

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


/* initialize driver & display */
/* use this function for a graphic display */
int drv_Sample_init2(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_Sample_blit;

    /* remove unless you have GPO's */
    drv_generic_gpio_real_set = drv_Sample_GPO;

    /* start display */
    if ((ret = drv_Sample_start2(section)) != 0)
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

    return 0;
}



/* close driver & display */
/* use this function for a text display */
int drv_Sample_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* remove unless you have GPO's */
    drv_generic_gpio_quit();

    /* clear display */
    drv_Sample_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_Sample_close();

    return (0);
}

/* close driver & display */
/* use this function for a graphic display */
int drv_Sample_quit2(const int quiet)
{

    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    /* remove unless you have GPO's */
    drv_generic_gpio_quit();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    debug("closing connection");
    drv_Sample_close();

    return (0);
}


/* use this one for a text display */
DRIVER drv_Sample = {
  name:Name,
  list:drv_Sample_list,
  init:drv_Sample_init,
  quit:drv_Sample_quit,
};


/* use this one for a graphic display */
DRIVER drv_Sample2 = {
  name:Name,
  list:drv_Sample_list,
  init:drv_Sample_init2,
  quit:drv_Sample_quit2,
};
