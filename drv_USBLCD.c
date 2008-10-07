/* $Id$
 * $URL$
 *
 * new style driver for USBLCD displays
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old-style USBLCD driver which is 
 * Copyright (C) 2002 Robin Adams, Adams IT Services <info@usblcd.de>
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
 * struct DRIVER drv_USBLCD
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

#ifdef HAVE_USB_H
#include <usb.h>
#endif

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


#define USBLCD_VENDOR  0x10D2
#define USBLCD_VENDOR2 0x1212
#define USBLCD_DEVICE  0x0001

#define IOC_GET_HARD_VERSION 1
#define IOC_GET_DRV_VERSION  2


static char Name[] = "USBLCD";

static char *Port = NULL;
static int use_libusb = 0;
static int usblcd_file;
static char *Buffer;
static char *BufPtr;


#ifdef HAVE_USB_H

static usb_dev_handle *lcd;
static int interface;

#endif



/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

#ifdef HAVE_USB_H

static int drv_UL_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    lcd = NULL;

    info("%s: scanning for USBLCD...", Name);

    usb_set_debug(0);

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if (((dev->descriptor.idVendor == USBLCD_VENDOR) ||
		 (dev->descriptor.idVendor == USBLCD_VENDOR2)) && (dev->descriptor.idProduct == USBLCD_DEVICE)) {

		unsigned int v = dev->descriptor.bcdDevice;

		info("%s: found USBLCD V%1d%1d.%1d%1d on bus %s device %s", Name,
		     (v & 0xF000) >> 12, (v & 0xF00) >> 8, (v & 0xF0) >> 4, (v & 0xF), bus->dirname, dev->filename);

		interface = 0;
		lcd = usb_open(dev);
		if (usb_claim_interface(lcd, interface) < 0) {
		    error("%s: usb_claim_interface() failed!", Name);
		    error("%s: maybe you have the usblcd module loaded?", Name);
		    return -1;
		}
		return 0;
	    }
	}
    }
    error("%s: could not find a USBLCD", Name);
    return -1;
}


static int drv_UL_close(void)
{
    usb_release_interface(lcd, interface);
    usb_close(lcd);

    return 0;
}

#endif


static void drv_UL_send(void)
{

#if 0
    struct timeval now, end;
    gettimeofday(&now, NULL);
#endif

    if (use_libusb) {
#ifdef HAVE_USB_H
	/* Fixme: Endpoint hardcoded to 1 ??? */
	usb_bulk_write(lcd, 1, Buffer, BufPtr - Buffer, 1000);
#endif
    } else {
	write(usblcd_file, Buffer, BufPtr - Buffer);
    }


#if 0
    gettimeofday(&end, NULL);
    debug("send %d bytes in %d usec (%d usec/byte)", BufPtr - Buffer,
	  (1000000 * (end.tv_sec - now.tv_sec) + end.tv_usec - now.tv_usec),
	  (1000000 * (end.tv_sec - now.tv_sec) + end.tv_usec - now.tv_usec) / (BufPtr - Buffer));
#endif

    BufPtr = Buffer;
}


static void drv_UL_command(const unsigned char cmd)
{
    *BufPtr++ = '\0';
    *BufPtr++ = cmd;
}


static void drv_UL_clear(void)
{
    drv_UL_command(0x01);	/* clear display */
    drv_UL_command(0x03);	/* return home */
    drv_UL_send();		/* flush buffer */
}


static void drv_UL_write(const int row, const int col, const char *data, int len)
{
    int pos;

    /* 16x4 Displays use a slightly different layout */
    if (DCOLS == 16 && DROWS == 4) {
	pos = (row % 2) * 64 + (row / 2) * 16 + col;
    } else {
	pos = (row % 2) * 64 + (row / 2) * 20 + col;
    }

    drv_UL_command(0x80 | pos);

    while (len--) {
	if (*data == 0)
	    *BufPtr++ = 0;
	*BufPtr++ = *data++;
    }

    drv_UL_send();
}

static void drv_UL_defchar(const int ascii, const unsigned char *matrix)
{
    int i;

    drv_UL_command(0x40 | 8 * ascii);

    for (i = 0; i < 8; i++) {
	if ((*matrix & 0x1f) == 0)
	    *BufPtr++ = 0;
	*BufPtr++ = *matrix++ & 0x1f;
    }

    drv_UL_send();
}


static int drv_UL_start(const char *section, const int quiet)
{
    int rows = -1, cols = -1;
    int major, minor;
    char *port, *s;
    char buf[128];

    if (Port) {
	free(Port);
	Port = NULL;
    }

    if ((port = cfg_get(section, "Port", NULL)) == NULL || *port == '\0') {
	error("%s: no '%s.Port' entry from %s", Name, section, cfg_source());
	return -1;
    }

    if (strcasecmp(port, "libusb") == 0) {
#ifdef HAVE_USB_H
	use_libusb = 1;
	debug("using libusb");
#else
	error("%s: cannot use 'libusb' port.", Name);
	error("%s: lcd4linux was compiled without libusb support!", Name);
	return -1;
#endif
    } else {
	if (port[0] == '/') {
	    Port = strdup(port);
	} else {
	    int len = 5 + strlen(port) + 1;
	    Port = malloc(len);
	    qprintf(Port, len, "/dev/%s", port);
	}
	debug("using device %s ", Port);
    }

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

    if (use_libusb) {

#ifdef HAVE_USB_H
	if (drv_UL_open() < 0) {
	    return -1;
	}
#endif

    } else {

	/* open port */
	usblcd_file = open(Port, O_WRONLY);
	if (usblcd_file == -1) {
	    error("%s: open(%s) failed: %s", Name, Port, strerror(errno));
	    return -1;
	}

	/* get driver version */
	memset(buf, 0, sizeof(buf));
	if (ioctl(usblcd_file, IOC_GET_DRV_VERSION, buf) != 0) {
	    error("%s: ioctl() failed, could not get Driver Version!", Name);
	    return -1;
	}
	info("%s: Driver Version: %s", Name, buf);

	if (sscanf(buf, "USBLCD Driver Version %d.%d", &major, &minor) != 2) {
	    error("%s: could not read Driver Version!", Name);
	    return -1;
	}
	if (major != 1) {
	    error("%s: Driver Version %d not supported!", Name, major);
	    return -1;
	}

	memset(buf, 0, sizeof(buf));
	if (ioctl(usblcd_file, IOC_GET_HARD_VERSION, buf) != 0) {
	    error("%s: ioctl() failed, could not get Hardware Version!", Name);
	    return -1;
	}
	info("%s: Hardware Version: %s", Name, buf);

	if (sscanf(buf, "%d.%d", &major, &minor) != 2) {
	    error("%s: could not read Hardware Version!", Name);
	    return -1;
	}

	if (major != 1) {
	    error("%s: Hardware Version %d not supported!", Name, major);
	    return -1;
	}
    }

    /* Init the command buffer */
    Buffer = (char *) malloc(1024);
    if (Buffer == NULL) {
	error("%s: coommand buffer could not be allocated: malloc() failed", Name);
	return -1;
    }
    BufPtr = Buffer;

    /* initialize display */
    drv_UL_command(0x29);	/* 8 Bit mode, 1/16 duty cycle, 5x8 font */
    drv_UL_command(0x08);	/* Display off, cursor off, blink off */
    drv_UL_command(0x0c);	/* Display on, cursor off, blink off */
    drv_UL_command(0x06);	/* curser moves to right, no shift */

    drv_UL_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "http://www.usblcd.de")) {
	    sleep(3);
	    drv_UL_clear();
	}
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


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
int drv_UL_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_UL_init(const char *section, const int quiet)
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
    drv_generic_text_real_write = drv_UL_write;
    drv_generic_text_real_defchar = drv_UL_defchar;


    /* start display */
    if ((ret = drv_UL_start(section, quiet)) != 0)
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
    /* none at the moment... */


    return 0;
}


/* close driver & display */
int drv_UL_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    /* flush buffer */
    drv_UL_send();

    drv_generic_text_quit();

    /* clear display */
    drv_UL_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    if (use_libusb) {
#ifdef HAVE_USB_H
	drv_UL_close();
#endif
    } else {
	debug("closing port %s", Port);
	close(usblcd_file);
    }

    if (Buffer) {
	free(Buffer);
	Buffer = NULL;
	BufPtr = Buffer;
    }

    return (0);
}


DRIVER drv_USBLCD = {
    .name = Name,
    .list = drv_UL_list,
    .init = drv_UL_init,
    .quit = drv_UL_quit,
};
