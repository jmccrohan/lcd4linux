/* $Id:  $
 * $URL: $
 *
 * driver for Attiny2313 USB LCD display interface
 * see http://ast.m-faq.de/USB-LCD/USB-LCD.htm for schematics
 *
 * Copyright 2005 Till Harbaum <till@harbaum.org>
 * Copyright 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_LCD2USB
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <usb.h>

#include "debug.h"
#include "cfg.h"
#include "timer.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "widget_keypad.h"
#include "drv.h"
#include "drv_generic_text.h"


#define LCD_USB_VENDOR    0x03eb
#define LCD_USB_DEVICE   0x7a53

#define CMD_LCD_ECHO           (0<<5)
#define CMD_LCD_INIT 		1
#define CMD_LCD_COMMAND 	2
#define CMD_LCD_DATA 		3
#define CMD_LCD_BLIGHT 		4


/* target is a bit map for CMD/DATA */
#define LCD_CTRL_0              (1<<0)
#define LCD_CTRL_1              (1<<1)
#define LCD_BOTH           (LCD_CTRL_0 | LCD_CTRL_1)


static char Name[] = "ASTUSB";
static char *device_id = NULL, *bus_id = NULL;

static usb_dev_handle *lcd;
static int controllers = 0;

extern int got_signal;

enum {
    LCD_INIT_INCREASE = 1 << 0,
    LCD_INIT_SHIFT = 1 << 1,
    LCD_INIT_CON = 1 << 2,
    LCD_INIT_BON = 1 << 3,
    LCD_INIT_DSHIFT = 1 << 4,
    LCD_INIT_RSHIFT = 1 << 5,
    LCD_INIT_2LINES = 1 << 6,
    LCD_INIT_FONT = 1 << 7
};

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_AST_HW_open(char *bus_id, char *device_id)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    lcd = NULL;

    info("%s: scanning USB for ASTUSB interface ...", Name);

    if (bus_id != NULL)
	info("%s: scanning for bus id: %s", Name, bus_id);

    if (device_id != NULL)
	info("%s: scanning for device id: %s", Name, device_id);

    usb_set_debug(0);

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	/* search this bus if no bus id was given or if this is the given bus id */
	if (!bus_id || (bus_id && !strcasecmp(bus->dirname, bus_id))) {

	    for (dev = bus->devices; dev; dev = dev->next) {
		/* search this device if no device id was given or if this is the given device id */
		if (!device_id || (device_id && !strcasecmp(dev->filename, device_id))) {

		    if ((dev->descriptor.idVendor == LCD_USB_VENDOR) && (dev->descriptor.idProduct == LCD_USB_DEVICE)) {
			info("%s: found ASTUSB interface on bus %s device %s", Name, bus->dirname, dev->filename);
			lcd = usb_open(dev);
			if (usb_claim_interface(lcd, 0) < 0) {
			    error("%s: usb_claim_interface() failed!", Name);
			    return -1;
			}
			return 0;
		    }
		}
	    }
	}
    }
    return -1;
}

static int drv_AST_HW_close(void)
{
    usb_release_interface(lcd, 0);
    usb_close(lcd);

    return 0;
}

static int drv_AST_HW_data(unsigned char *buffer, short length, int __attribute__ ((unused)) index)
{
    int request = CMD_LCD_DATA;
    int value = 0;

    if (usb_control_msg
	(lcd, USB_TYPE_VENDOR | USB_RECIP_DEVICE, request, value, 0 /*index */ , (char *) buffer, length, 1000) < 0) {
	error("%s: USB request failed! Trying to reconnect device.", Name);

	usb_release_interface(lcd, 0);
	usb_close(lcd);

	/* try to close and reopen connection */
	if (drv_AST_HW_open(bus_id, device_id) < 0) {
	    error("%s: could not re-detect ASTUSB USB LCD", Name);
	    got_signal = -1;
	    return -1;
	}
	/* and try to re-send command */
	if (usb_control_msg
	    (lcd, USB_TYPE_VENDOR | USB_RECIP_DEVICE, request, value, 0 /*index */ , (char *) buffer, length,
	     1000) < 0) {
	    error("%s: retried USB request failed, aborting!", Name);
	    got_signal = -1;
	    return -1;
	}

	info("%s: Device successfully reconnected.", Name);
    }

    return 0;
}

/* send a number of 16 bit words to the lcd2usb interface */
/* and verify that they are correctly returned by the echo */
/* command. This may be used to check the reliability of */
/* the usb interfacing */
#define ECHO_NUM 10


static int drv_AST_HW_cmd(unsigned char cmd, short value, int __attribute__ ((unused)) index)
{
    unsigned char buffer[2];
    int nBytes;

    /* send control request and accept return value */
    nBytes = usb_control_msg(lcd,
			     USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			     cmd, value, 0 /*index */ , (char *) buffer, sizeof(buffer), 1000);

    if (nBytes < 0) {
	error("%s: USB request failed!", Name);
	return -1;
    }

    return buffer[0] + 256 * buffer[1];
}

int drv_AST_HW_echo(void)
{
    int i, errors = 0;
    char val;
    char buffer[2];

    for (i = 0; i < ECHO_NUM; i++) {
	val = rand() & 0xff;	//8 bit number

	if (usb_control_msg(lcd,
			    USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			    CMD_LCD_ECHO, val, 0, buffer, sizeof(buffer), 1000) < 0) {
	    error("%s: USB request failed!", Name);
	    return -1;
	}

	if (val != buffer[1])
	    errors++;
    }

    if (errors) {
	error("%s: ERROR, %d out of %d echo transfers failed!", Name, errors, ECHO_NUM);
	return -1;
    } else
	info("%s: echo test successful", Name);


    return 0;
}


/* command format: 
 * 7 6 5 4 3 2 1 0
 * C C C T T R L L
 *
 * TT = target bit map 
 * R = reserved for future use, set to 0
 * LL = number of bytes in transfer - 1 
 */

short drv_AST_LCD_BL(int backlight)
{

    if (backlight < 0)
	backlight = 0;
    else if (backlight > 1)
	backlight = 1;

    drv_AST_HW_cmd(CMD_LCD_BLIGHT, backlight, 0);

    return backlight;
}


static void drv_AST_clear(void)
{
    drv_AST_HW_cmd(CMD_LCD_COMMAND, 0x01, LCD_BOTH & controllers);	/* clear display */
    drv_AST_HW_cmd(CMD_LCD_COMMAND, 0x03, LCD_BOTH & controllers);	/* return home */
}

static void drv_AST_write(const int row, const int col, const char *data, int len)
{
    int pos, ctrl = LCD_CTRL_0;
    int tmpRow = row;

    /* displays with more two rows and 20 columns have a logical width */
    /* of 40 chars and use more than one controller */
    if ((DROWS > 2) && (DCOLS > 20) && (row > 1)) {
	/* use second controller */
	tmpRow -= 2;
	ctrl = LCD_CTRL_1;
    }

    /* 16x4 Displays use a slightly different layout */
    if (DCOLS == 16 && DROWS == 4) {
	pos = (tmpRow % 2) * 64 + (tmpRow / 2) * 16 + col;
    } else {
	pos = (tmpRow % 2) * 64 + (tmpRow / 2) * 20 + col;
    }

    drv_AST_HW_cmd(CMD_LCD_COMMAND, 0x80 | pos, (ctrl & controllers));
    drv_AST_HW_data((unsigned char *) data, len, (ctrl & controllers));

}

static void drv_AST_defchar(const int ascii, const unsigned char *matrix)
{
    int i;
    unsigned char buffer[8];

    //build buffer
    for (i = 0; i < 8; i++) {
	buffer[i] = *matrix++ & 0x1f;
    }

    //send to HW
    drv_AST_HW_cmd(CMD_LCD_COMMAND, 0x40 | 8 * ascii, LCD_BOTH & controllers);
    drv_AST_HW_data((unsigned char *) buffer, sizeof(buffer), LCD_BOTH & controllers);
}

static int drv_AST_start(const char *section, const int quiet)
{
//    int contrast, brightness;
    int backlight;
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

    /* bus id and device id are strings and not just intergers, since */
    /* the windows port of libusb treats them as strings. And this way */
    /* we keep windows compatibility ... just in case ... */
    bus_id = cfg_get(section, "Bus", NULL);
    device_id = cfg_get(section, "Device", NULL);

    if (drv_AST_HW_open(bus_id, device_id) < 0) {
	error("%s: could not find a ASTUSB USB LCD", Name);
	return -1;
    }

    /* test interface reliability */
    drv_AST_HW_echo();

    /*low level init of init LCD */
    if (drv_AST_HW_cmd(CMD_LCD_INIT, LCD_INIT_INCREASE | (DROWS > 1 ? LCD_INIT_2LINES : 1), LCD_BOTH & controllers) < 0) {
	error("%s: could not init LCD", Name);
	return -1;
    }

    if (cfg_number(section, "Backlight", 0, 0, 1, &backlight) > 0) {
	info("%s: backlight %s", Name, backlight ? "enabled" : "disabled");
	drv_AST_LCD_BL(backlight);
    }

    drv_AST_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "AST USB-LCD")) {
	    sleep(3);
	    drv_AST_clear();
	}
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_AST_LCD_BL(R2N(arg1));
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
int drv_AST_list(void)
{
    printf("ASTUSB display interface");
    return 0;
}


/* initialize driver & display */
int drv_AST_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Rev: 975 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_AST_write;
    drv_generic_text_real_defchar = drv_AST_defchar;

    /* start display */
    if ((ret = drv_AST_start(section, quiet)) != 0)
	return ret;

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
    /* most displays have a full block on ascii 255, but some have kind of  */
    /* an 'inverted P'. If you specify 'asc255bug 1 in the config, this */
    /* char will not be used, but rendered by the bar driver */
    cfg_number(section, "asc255bug", 0, 0, 1, &asc255bug);
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    if (!asc255bug)
	drv_generic_text_bar_add_segment(255, 255, 255, 255);	/* ASCII 255 = block */


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
    // AddFunction("LCD::contrast", 1, plugin_contrast);
    // AddFunction("LCD::brightness", 1, plugin_brightness);
    AddFunction("LCD::backlight", 1, plugin_backlight);

    return 0;
}


/* close driver & display */
int drv_AST_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_AST_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("That's all, folks!", NULL);
    }

    drv_AST_LCD_BL(0);

    debug("closing USB connection");
    drv_AST_HW_close();

    return (0);
}


DRIVER drv_ASTUSB = {
    .name = Name,
    .list = drv_AST_list,
    .init = drv_AST_init,
    .quit = drv_AST_quit,
};
