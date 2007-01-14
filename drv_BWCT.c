/* $Id$
 * $URL$
 *
 * new style driver for BWCT USB LCD displays
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_BWCT
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
#include <sys/ioctl.h>
#include <sys/time.h>

#include <usb.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"


#define LCD_USB_VENDOR 0x03da
#define LCD_USB_DEVICE 0x0002

#define LCD_RESET    1
#define LCD_CMD      2
#define LCD_DATA     3
#define LCD_CONTRAST 4


static char Name[] = "BWCT";

static usb_dev_handle *lcd;
static int interface;

extern int usb_debug;
extern int got_signal;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_BW_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    lcd = NULL;

    info("%s: scanning USB for BWCT LCD...", Name);

    usb_debug = 0;

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    int c;
	    if (dev->descriptor.idVendor != LCD_USB_VENDOR)
		continue;
	    /* Loop through all of the configurations */
	    for (c = 0; c < dev->descriptor.bNumConfigurations; c++) {
		int i;
		for (i = 0; i < dev->config[c].bNumInterfaces; i++) {
		    int a;
		    for (a = 0; a < dev->config[c].interface[i].num_altsetting; a++) {
			if ((dev->descriptor.idProduct == LCD_USB_DEVICE) ||
			    ((dev->config[c].interface[i].altsetting[a].bInterfaceClass == 0xff) &&
			     (dev->config[c].interface[i].altsetting[a].bInterfaceSubClass == 0x01))) {
			    info("%s: found BWCT USB LCD on bus %s device %s", Name, bus->dirname, dev->filename);
			    interface = i;
			    lcd = usb_open(dev);
			    if (usb_claim_interface(lcd, interface) < 0) {
				error("%s: usb_claim_interface() failed!", Name);
				return -1;
			    }
			    return 0;
			}
		    }
		}
	    }
	}
    }
    return -1;
}


static int drv_BW_close(void)
{
    usb_release_interface(lcd, interface);
    usb_close(lcd);

    return 0;
}


static int drv_BW_send(int request, int value)
{
    static int errors = 0;

    if (errors > 20)
	return -1;

    if (usb_control_msg(lcd, USB_TYPE_VENDOR, request, value, interface, NULL, 0, 1000) < 0) {
	error("%s: USB request failed!", Name);
	if (++errors > 20) {
	    error("%s: too many USB errors, aborting.", Name);
	    got_signal = -1;
	}
	return -1;
    }
    errors = 0;
    return 0;
}


static void drv_BW_command(const unsigned char cmd)
{
    drv_BW_send(LCD_CMD, cmd);
}


static void drv_BW_clear(void)
{
    drv_BW_command(0x01);	/* clear display */
    drv_BW_command(0x03);	/* return home */
}


static void drv_BW_write(const int row, const int col, const char *data, int len)
{
    int pos;

    /* 16x4 Displays use a slightly different layout */
    if (DCOLS == 16 && DROWS == 4) {
	pos = (row % 2) * 64 + (row / 2) * 16 + col;
    } else {
	pos = (row % 2) * 64 + (row / 2) * 20 + col;
    }

    drv_BW_command(0x80 | pos);

    while (len--) {
	drv_BW_send(LCD_DATA, *data++);
    }
}

static void drv_BW_defchar(const int ascii, const unsigned char *matrix)
{
    int i;

    drv_BW_command(0x40 | 8 * ascii);

    for (i = 0; i < 8; i++) {
	drv_BW_send(LCD_DATA, *matrix++ & 0x1f);
    }
}


static int drv_BW_contrast(int contrast)
{
    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    drv_BW_send(LCD_CONTRAST, contrast);

    return contrast;
}


static int drv_BW_start(const char *section, const int quiet)
{
    int contrast;
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

    if (drv_BW_open() < 0) {
	error("%s: could not find a BWCT USB LCD", Name);
	return -1;
    }

    /* reset */
    drv_BW_send(LCD_RESET, 0);

    /* initialize display */
    drv_BW_command(0x29);	/* 8 Bit mode, 1/16 duty cycle, 5x8 font */
    drv_BW_command(0x08);	/* Display off, cursor off, blink off */
    drv_BW_command(0x0c);	/* Display on, cursor off, blink off */
    drv_BW_command(0x06);	/* curser moves to right, no shift */


    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_BW_contrast(contrast);
    }

    drv_BW_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.bwct.de")) {
	    sleep(3);
	    drv_BW_clear();
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

    contrast = drv_BW_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
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
int drv_BW_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_BW_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Revision: 1.6 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_BW_write;
    drv_generic_text_real_defchar = drv_BW_defchar;


    /* start display */
    if ((ret = drv_BW_start(section, quiet)) != 0)
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
    AddFunction("LCD::contrast", 1, plugin_contrast);

    return 0;
}


/* close driver & display */
int drv_BW_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_BW_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing USB connection");
    drv_BW_close();

    return (0);
}


DRIVER drv_BWCT = {
  name:Name,
  list:drv_BW_list,
  init:drv_BW_init,
  quit:drv_BW_quit,
};
