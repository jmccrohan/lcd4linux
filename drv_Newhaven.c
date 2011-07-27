/* $Id$
 * $URL$
 *
 * Newhaven lcd4linux driver
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * Copyright (C) 2011 Rusty Clarkson <rusty@sutus.com>
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
 * *** Newhaven NHD‐0420D3Z‐FL‐GBW ***
 * A 20x4 LCD (Liquid Crystal Display).
 *
 * The display supports text mode provide 4 rows of 20 characters. This display
 * also supports many functions. The documentation for this display can be found
 * online easily. http://www.newhavendisplay.com/ would be a good place to start.
 * This driver was designed with the NHD-0420D3Z-FL-GBW in mind, but I'm sure
 * similar Newhaven displays will work with little to no effort.
 *
 * This driver is released under the GPL.
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Newhaven
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
//#include "drv_generic_serial.h"

/* i2c bus? */
#ifdef WITH_I2C
#include "drv_generic_i2c.h"
#endif


static char Name[] = "Newhaven";


/* Newhaven Constants */

/* Line 1 */
#define NEWHAVEN_R1_C1 0x00
/* Line 2 */
#define NEWHAVEN_R2_C1 0x40
/* Line 3 */
#define NEWHAVEN_R3_C1 0x14
/* Line 4 */
#define NEWHAVEN_R4_C1 0x54

#define NEWHAVEN_ROW_MAX 4
#define NEWHAVEN_COL_MAX 20

#define NEWHAVEN_COMMAND 0xFE

#define NEWHAVEN_DISPLAY_ON 0x41
#define NEWHAVEN_DISPLAY_OFF 0x42

#define NEWHAVEN_BAUD_RATE_SET 0x61
#define NEWHAVEN_BAUD_RATE_SHOW 0x71
#define NEWHAVEN_I2C_ADDR_SET 0x62
#define NEWHAVEN_I2C_ADDR_SHOW 0x72
#define NEWHAVEN_I2C_ADDR_DEFAULT 0x28
#define NEWHAVEN_DISPLAY_FIRMWARE_VER 0x70

#define NEWHAVEN_CONTRAST_SET 0x52
#define NEWHAVEN_CONTRAST_MIN 1
#define NEWHAVEN_CONTRAST_MAX 50
#define NEWHAVEN_CONTRAST_DEFAULT 25
#define NEWHAVEN_BRIGHTNESS_SET 0x53
#define NEWHAVEN_BRIGHTNESS_MIN 1
#define NEWHAVEN_BRIGHTNESS_MAX 8
#define NEWHAVEN_BRIGHTNESS_DEFAULT 6

#define NEWHAVEN_CLEAR_SCREEN 0x51
#define NEWHAVEN_DISPLAY_SHIFT_LEFT 0x55
#define NEWHAVEN_DISPLAY_SHIFT_RIGHT 0x56

#define NEWHAVEN_CURSOR_SET_POS 0x45
#define NEWHAVEN_CURSOR_HOME 0x46
#define NEWHAVEN_CURSOR_UNDERLINE_ON 0x47
#define NEWHAVEN_CURSOR_UNDERLINE_OFF 0x48
#define NEWHAVEN_CURSOR_LEFT_ONE 0x49
#define NEWHAVEN_CURSOR_RIGHT_ONE 0x4A
#define NEWHAVEN_CURSOR_BLINKING_ON 0x4B
#define NEWHAVEN_CURSOR_BLINKING_OFF 0x4C
#define NEWHAVEN_BACKSPACE 0x4E

#define NEWHAVEN_LOAD_CUSTOM_CHAR 0x54


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_Newhaven_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    //if (drv_generic_serial_open(section, Name, 0) < 0)
    //return -1;

    /* open i2c port */

    if (drv_generic_i2c_open(section, Name) < 0)
	return -1;

    return 0;
}


static int drv_Newhaven_close(void)
{
    /* close whatever port you've opened */
    //drv_generic_serial_close();
    drv_generic_i2c_close();

    return 0;
}


/* dummy function that sends data to the display */
static void drv_Newhaven_sendData(const char *data, const unsigned int len)
{
    unsigned int i;

    /* send data to the serial port is easy... */
    //drv_generic_serial_write(data, len);

    /* sending data using other methods is a bit more tricky... */
    for (i = 0; i < len; i++) {
	/* send data to the i2c port */
	drv_generic_i2c_byte(data[i]);
    }

}


/* dummy function that sends commands to the display */
static void drv_Newhaven_sendCommand(const char command, const char *data, const unsigned int len)
{

    /* send commands to the serial port */
    //drv_generic_serial_write(NEWHAVEN_COMMAND, 1);
    //drv_generic_serial_write(command, 1);

    /* send commands to the i2c port */
    drv_generic_i2c_byte(NEWHAVEN_COMMAND);
    drv_generic_i2c_byte(command);

    /* send data */
    drv_Newhaven_sendData(data, len);

}


/* text mode displays only */
static void drv_Newhaven_clear(void)
{
    char cmd;

    /* do whatever is necessary to clear the display */
    cmd = NEWHAVEN_CLEAR_SCREEN;
    drv_Newhaven_sendCommand(cmd, NULL, 0);
}


/* Turn display on */
static void drv_Newhaven_display_on(void)
{
    char cmd;

    /* do whatever is necessary to turn on the display */
    cmd = NEWHAVEN_DISPLAY_ON;
    drv_Newhaven_sendCommand(cmd, NULL, 0);
}


/* Turn display off */
static void __attribute__ ((unused)) drv_Newhaven_display_off(void)
{
    char cmd;

    /* do whatever is necessary to turn off the display */
    cmd = NEWHAVEN_DISPLAY_OFF;
    drv_Newhaven_sendCommand(cmd, NULL, 0);
}


/* text mode displays only */
static void drv_Newhaven_write(const int row, const int col, const char *data, int len)
{
    char cmd;
    char wData[1];

    /* do the cursor positioning here */
    cmd = NEWHAVEN_CURSOR_SET_POS;
    if (col < 0 || col > DCOLS - 1)
	error("%s: Invalid col, %d. Valid cols 0-%d.", Name, col, DCOLS - 1);
    if (len > DCOLS - (col + 1))
	error("%s: Data length, %d, longer than columns left in row, %d.", Name, len, DCOLS - (col + 1));
    switch (row) {
    case 0:
	wData[0] = NEWHAVEN_R1_C1 + col;
	break;
    case 1:
	wData[0] = NEWHAVEN_R2_C1 + col;
	break;
    case 2:
	wData[0] = NEWHAVEN_R3_C1 + col;
	break;
    case 3:
	wData[0] = NEWHAVEN_R4_C1 + col;
	break;
    default:
	error("%s: Invalid row, %d. Valid rows 0-%d.", Name, row, DROWS - 1);
	return;
    }
    drv_Newhaven_sendCommand(cmd, wData, 1);

    /* send string to the display */
    drv_Newhaven_sendData(data, len);

}

/* text mode displays only */
static void drv_Newhaven_defchar(const int ascii, const unsigned char *matrix)
{
    char cmd;
    char wData[9];
    int i;

    /* call the 'define character' function */
    cmd = NEWHAVEN_LOAD_CUSTOM_CHAR;
    wData[0] = ascii;

    /* send bitmap to the display */
    for (i = 0; i < 8; i++) {
	wData[i + 1] = *matrix++;
    }
    drv_Newhaven_sendCommand(cmd, wData, 9);
}


/* Set the contrast of the display */
static int drv_Newhaven_contrast(int contrast)
{
    char cmd;
    char wData[1];

    /* adjust limits according to the display */
    if (contrast < NEWHAVEN_CONTRAST_MIN)
	contrast = NEWHAVEN_CONTRAST_MIN;
    if (contrast > NEWHAVEN_CONTRAST_MAX)
	contrast = NEWHAVEN_CONTRAST_MAX;

    /* call a 'contrast' function */
    cmd = NEWHAVEN_CONTRAST_SET;
    wData[0] = contrast;
    drv_Newhaven_sendCommand(cmd, wData, 1);

    return contrast;
}


/* Set the brightness of the display */
static int drv_Newhaven_brightness(int brightness)
{
    char cmd;
    char wData[1];

    /* adjust limits according to the display */
    if (brightness < NEWHAVEN_BRIGHTNESS_MIN)
	brightness = NEWHAVEN_BRIGHTNESS_MIN;
    if (brightness > NEWHAVEN_BRIGHTNESS_MAX)
	brightness = NEWHAVEN_BRIGHTNESS_MAX;

    /* call a 'brightness' function */
    cmd = NEWHAVEN_BRIGHTNESS_SET;
    wData[0] = brightness;
    drv_Newhaven_sendCommand(cmd, wData, 1);

    return brightness;
}


/* start text mode display */
static int drv_Newhaven_start(const char *section)
{
    int contrast, brightness;
    int rows = -1, cols = -1;
    char *s;

    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1 || rows > NEWHAVEN_ROW_MAX
	|| cols > NEWHAVEN_COL_MAX) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }

    DROWS = rows;
    DCOLS = cols;

    /* open communication with the display */
    if (drv_Newhaven_open(section) < 0) {
	return -1;
    }

    /* reset & initialize display */
    drv_Newhaven_display_on();

    if (cfg_number
	(section, "Contrast", NEWHAVEN_CONTRAST_DEFAULT, NEWHAVEN_CONTRAST_MIN, NEWHAVEN_CONTRAST_MAX, &contrast) > 0) {
	drv_Newhaven_contrast(contrast);
    }

    if (cfg_number
	(section, "Brightness", NEWHAVEN_BRIGHTNESS_DEFAULT, NEWHAVEN_BRIGHTNESS_MIN, NEWHAVEN_BRIGHTNESS_MAX,
	 &brightness) > 0) {
	drv_Newhaven_brightness(brightness);
    }

    drv_Newhaven_clear();	/* clear display */
    sleep(1);

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_Newhaven_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}

static void plugin_brightness(RESULT * result, RESULT * arg1)
{
    double brightness;

    brightness = drv_Newhaven_brightness(R2N(arg1));
    SetResult(&result, R_NUMBER, &brightness);
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
int drv_Newhaven_list(void)
{
    printf("%s driver", Name);
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_Newhaven_init(const char *section, const int quiet)
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
    drv_generic_text_real_write = drv_Newhaven_write;
    drv_generic_text_real_defchar = drv_Newhaven_defchar;


    /* start display */
    if ((ret = drv_Newhaven_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.bwct.de")) {
	    sleep(3);
	    drv_Newhaven_clear();
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
    AddFunction("LCD::brightness", 1, plugin_brightness);

    return 0;
}



/* close driver & display */
/* use this function for a text display */
int drv_Newhaven_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_Newhaven_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_Newhaven_close();

    return (0);
}


/* use this one for a text display */
DRIVER drv_Newhaven = {
    .name = Name,
    .list = drv_Newhaven_list,
    .init = drv_Newhaven_init,
    .quit = drv_Newhaven_quit,
};
