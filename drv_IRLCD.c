/* $Id$
 * $URL$
 *
 **************************************************************************
 * Driver for IRLCD : simple USB1.1 LCD + IR Receiver based on ATtiny2313 *
 **************************************************************************
 * Hardware from http://www.xs4all.nl/~dicks/avr/usbtiny/index.html :     *
 *        [USBtiny LIRC compatible IR receiver and LCD controller]        *
 * Ths driver is based on LCD2USB software by Till Harbaum, adapted to    *
 * USBTiny protocol by Jean-Philippe Civade                               *
 **************************************************************************
 *
 * The IR receiving par is compatible with IgrPlug protocol
 * and can be used from LIRC.
 *
 * Copyright (C) 2008 Jean-Philippe Civade <jp@civade.com> (for porting to IRLCD)
 * Copyright (C) 2005 Till Harbaum <till@harbaum.org> (for LCD2USB friver)
 * Copyright (C) 2005, 2006, 2007, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_IRLCD
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <usb.h>

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

#include "drv_generic_text.h"

/* vid/pid of IRLCD */
#define LCD_USB_VENDOR 0x03EB	/* for Atmel device */
#define LCD_USB_DEVICE 0x0002

/* Protocole IR/LCD */
#define LCD_INSTR      20
#define LCD_DATA       21

static char Name[] = "IRLCD";
static char *device_id = NULL, *bus_id = NULL;

static usb_dev_handle *lcd;

extern int got_signal;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_IRLCD_open(char *bus_id, char *device_id)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    lcd = NULL;

    info("%s: scanning USB for IRLCD interface ...", Name);

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
			info("%s: found IRLCD interface on bus %s device %s", Name, bus->dirname, dev->filename);
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


static int drv_IRLCD_close(void)
{
    usb_release_interface(lcd, 0);
    usb_close(lcd);

    return 0;
}


/* Send a buffer to lcd via a control message */
static int drv_IRLCD_send(int request, unsigned char *buffer, int size)
{
    if (usb_control_msg(lcd, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,	/* bRequestType */
			request,	/* bRequest (LCD_INSTR / LCD_DATA) */
			0,	/* wValue (0) */
			0,	/* wIndex (0) */
			(char *) buffer,	/* pointer to destination buffer */
			size,	/* wLength */
			1000) < 0) {	/* Timeout in millisectonds */
	error("%s: USB request failed! Trying to reconnect device.", Name);

	usb_release_interface(lcd, 0);
	usb_close(lcd);

	/* try to close and reopen connection */
	if (drv_IRLCD_open(bus_id, device_id) < 0) {
	    error("%s: could not re-detect IRLCD USB LCD", Name);
	    got_signal = -1;
	    return -1;
	}
	/* and try to re-send command */
	if (usb_control_msg(lcd, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,	/* bRequestType  */
			    request,	/* bRequest (LCD_INSTR / LCD_DATA) */
			    0,	/* wValue (0) */
			    0,	/* wIndex (0) */
			    (char *) buffer,	/* pointer to destination buffer */
			    size,	/* wLength */
			    1000) < 0) {	/* Timeout in millisectonds */
	    error("%s: retried USB request failed, aborting!", Name);
	    got_signal = -1;
	    return -1;
	}

	info("%s: Device successfully reconnected.", Name);
    }

    return 0;
}


/* text mode displays only */
static void drv_IRLCD_clear(void)
{
    unsigned char cmd[1];

    cmd[0] = 0x01;		/* clear */
    drv_IRLCD_send(LCD_INSTR, cmd, 1);
    cmd[0] = 0x03;		/* home */
    drv_IRLCD_send(LCD_INSTR, cmd, 1);
}


/* text mode displays only */
static void drv_IRLCD_write(const int row, const int col, const char *data, int len)
{
    unsigned char cmd[1];
    static int pos;

    /* for 2 lines display */
    pos = (row % 2) * 64 + (row / 2) * 20 + col;

    /* do the cursor positioning here */
    cmd[0] = 0x80 | pos;

    /* do positionning */
    drv_IRLCD_send(LCD_INSTR, cmd, 1);

    /* send string to the display */
    drv_IRLCD_send(LCD_DATA, (unsigned char *) data, len);

}

/* text mode displays only */
static void drv_IRLCD_defchar(const int ascii, const unsigned char *matrix)
{
    unsigned char cmd[10];
    int i;

    /* Write to CGRAM */
    cmd[0] = 0x40 | 8 * ascii;
    drv_IRLCD_send(LCD_INSTR, cmd, 1);


    /* send bitmap to the display */
    for (i = 0; i < 8; i++) {
	cmd[i] = matrix[i] & 0x1f;
    }
    drv_IRLCD_send(LCD_DATA, cmd, 8);
}


/* start text mode display */
static int drv_IRLCD_start(const char *section)
{
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

    if (drv_IRLCD_open(bus_id, device_id) < 0) {
	error("%s: could not find a IRLC USB LCD", Name);
	return -1;
    }

    /* reset & initialize display */
    drv_IRLCD_clear();		/* clear display */

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/
/* no plugins capabilities */

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
int drv_IRLCD_list(void)
{
    printf("USBtiny LCD controller");
    return 0;
}

/* initialize driver & display */
int drv_IRLCD_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_IRLCD_write;
    drv_generic_text_real_defchar = drv_IRLCD_defchar;

    /* start display */
    if ((ret = drv_IRLCD_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, " www.civade.com")) {
	    sleep(3);
	    drv_IRLCD_clear();
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

    return 0;
}


/* close driver & display */
/* use this function for a text display */
int drv_IRLCD_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_IRLCD_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing usb connection");
    drv_IRLCD_close();

    return (0);
}

/* use this one for a text display */
DRIVER drv_IRLCD = {
    .name = Name,
    .list = drv_IRLCD_list,
    .init = drv_IRLCD_init,
    .quit = drv_IRLCD_quit,
};
