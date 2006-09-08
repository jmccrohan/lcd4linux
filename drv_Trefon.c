/* $Id: drv_Trefon.c,v 1.7 2006/09/08 19:00:46 reinelt Exp $
 *
 * driver for TREFON USB LCD displays - http://www.trefon.de
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
 * $Log: drv_Trefon.c,v $
 * Revision 1.7  2006/09/08 19:00:46  reinelt
 * give up after 10 write errors to serial device
 *
 * Revision 1.6  2006/01/30 06:25:54  reinelt
 * added CVS Revision
 *
 * Revision 1.5  2005/08/21 08:18:56  reinelt
 * CrystalFontz ACK processing
 *
 * Revision 1.4  2005/08/20 10:10:13  reinelt
 * TREFON patch from Stephan Trautvetter:
 * drv_TF_init: CHAR0 set to 0 instead of 1
 * drv_TF_write: combine the GOTO and the data into one packet
 * drv_TF_write: add GOTO-Case for resolutions 8x1/20x4 characters
 * drv_TF_start: test for existing resolutions from TREFON USB-LCDs implemented
 * the use of 'asc255bug 1' is recommendable
 *
 * Revision 1.3  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.2  2005/04/24 05:27:09  reinelt
 * Trefon Backlight added
 *
 * Revision 1.1  2005/04/24 04:33:46  reinelt
 * driver for TREFON USB LCD's added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Trefon
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


#define LCD_USB_VENDOR 0xfff0
#define LCD_USB_DEVICE 0xfffe

#define PKT_START     0x02
#define PKT_BACKLIGHT 0x01
#define PKT_DATA      0x02
#define PKT_CTRL      0x06
#define PKT_END       0xff

static char Name[] = "TREFON";

static usb_dev_handle *lcd;
static int interface;

extern int usb_debug;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_TF_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    lcd = NULL;

    info("%s: scanning USB for TREFON LCD...", Name);

    usb_debug = 0;

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == LCD_USB_VENDOR) && (dev->descriptor.idProduct == LCD_USB_DEVICE)) {
		info("%s: found TREFON USB LCD on bus %s device %s", Name, bus->dirname, dev->filename);
		lcd = usb_open(dev);
		if (usb_set_configuration(lcd, 1) < 0) {
		    error("%s: usb_set_configuration() failed!", Name);
		    return -1;
		}
		interface = 0;
		if (usb_claim_interface(lcd, interface) < 0) {
		    error("%s: usb_claim_interface() failed!", Name);
		    return -1;
		}
		return 0;
	    }
	}
    }
    return -1;
}


static int drv_TF_close(void)
{
    usb_release_interface(lcd, interface);
    usb_close(lcd);

    return 0;
}


static void drv_TF_send(char *data, int size)
{
    char buffer[64];

    /* the controller always wants a 64-byte packet */
    memset(buffer, 0, 64);
    memcpy(buffer, data, size);

    // Endpoint hardcoded to 2
    usb_bulk_write(lcd, 2, buffer, 64, 2000);
}


static void drv_TF_command(const unsigned char cmd)
{
    char buffer[4] = { PKT_START, PKT_CTRL, 0, PKT_END };
    buffer[2] = cmd;
    drv_TF_send(buffer, 4);
}


static void drv_TF_clear(void)
{
    drv_TF_command(0x01);
}


static void drv_TF_write(const int row, const int col, const char *data, const int len)
{
    char buffer[64];
    char *p;
    int pos = 0;

    if (DCOLS == 8 && DROWS == 1) {	// 8x1 Characters
	pos = row * 0x40 + col;
    } else if (DCOLS == 16 && DROWS == 2) {	// 16x2 Characters
	pos = row * 0x40 + col;
    } else if (DCOLS == 20 && DROWS == 4) {	// 20x4 Characters
	pos = row * 0x20 + col;
    } else {
	error("%s: internal error: DCOLS=%d DROWS=%d", Name, DCOLS, DROWS);
	return;
    }

    // combine the GOTO and the data into one packet
    p = buffer;
    *p++ = PKT_START;
    *p++ = PKT_CTRL;		// Goto
    *p++ = 0x80 | pos;
    *p++ = PKT_DATA;		// Data
    *p++ = (char) len;
    for (pos = 0; pos < len; pos++) {
	*p++ = *data++;
    }
    *p++ = PKT_END;

    drv_TF_send(buffer, len + 5);
}


static void drv_TF_defchar(const int ascii, const unsigned char *matrix)
{

    char buffer[14];
    char *p;
    int i;

    p = buffer;
    *p++ = PKT_START;
    *p++ = PKT_CTRL;
    *p++ = 0x40 | 8 * ascii;
    *p++ = PKT_DATA;
    *p++ = 8;			/* data length */
    for (i = 0; i < 8; i++) {
	*p++ = *matrix++ & 0x1f;
    }
    *p++ = PKT_END;

    drv_TF_send(buffer, 14);
}


static int drv_TF_backlight(int backlight)
{
    char buffer[4] = { PKT_START, PKT_BACKLIGHT, 0, PKT_END };

    if (backlight < 0)
	backlight = 0;
    if (backlight > 1)
	backlight = 1;

    buffer[2] = backlight;
    drv_TF_send(buffer, 4);

    return backlight;
}


// test for existing resolutions from TREFON USB-LCDs (TEXT-Mode only)
int drv_TF_valid_resolution(int rows, int cols)
{

    if (rows == 1 && cols == 8) {
	return 0;
    } else if (rows == 2 && cols == 16) {
	return 0;
    } else if (rows == 4 && cols == 20) {
	return 0;
    }
    return -1;
}


static int drv_TF_start(const char *section, const int quiet)
{
    int backlight;
    int rows = -1, cols = -1;
    char *s;

    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || drv_TF_valid_resolution(rows, cols) < 0) {
	error("%s: bad %s.Size '%s' (only 8x1/16x2/20x4) from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }

    DROWS = rows;
    DCOLS = cols;

    if (drv_TF_open() < 0) {
	error("%s: could not find a TREFON USB LCD", Name);
	return -1;
    }

    if (cfg_number(section, "Backlight", 1, 0, 1, &backlight) > 0) {
	drv_TF_backlight(backlight);
    }

    drv_TF_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.trefon.de")) {
	    sleep(3);
	    drv_TF_clear();
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

    backlight = drv_TF_backlight(R2N(arg1));
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
int drv_TF_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_TF_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Revision: 1.7 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 1;			/* ASCII of first user-defineable char */
    GOTO_COST = 64;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_TF_write;
    drv_generic_text_real_defchar = drv_TF_defchar;


    /* start display */
    if ((ret = drv_TF_start(section, quiet)) != 0)
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
    AddFunction("LCD::backlight", 1, plugin_backlight);

    return 0;
}


/* close driver & display */
int drv_TF_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_TF_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing USB connection");
    drv_TF_close();

    return (0);
}


DRIVER drv_Trefon = {
  name:Name,
  list:drv_TF_list,
  init:drv_TF_init,
  quit:drv_TF_quit,
};
