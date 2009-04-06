/* $Id: drv_Sample.c 975 2009-01-18 11:16:20Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_Sample.c $
 *
 * Shuttle SG33G5M VFD lcd4linux driver
 *
 * Copyright (C) 2009 Matthieu Crapet <mcrapet@gmail.com>
 * based on the USBLCD driver.
 *
 * Shuttle SG33G5M VFD (20x1 character display. Each character cell is 5x8 pixels)
 * - The display is driven by Princeton Technologies PT6314 VFD controller
 * - Cypress CY7C63723C (receives USB commands and talk to VFD controller)
 *
 * LCD "prococol" : each message has a length of 8 bytes
 * - 1 nibble: command (0x1, 0x3, 0x7, 0x9, 0xD)
 *     - 0x1 : clear text and icons (len=1)
 *     - 0x7 : icons (len=4)
 *     - 0x9 : text (len=7)
 *     - 0xD : set clock data (len=7)
 *     - 0x3 : display clock (internal feature) (len=1)
 * - 1 nibble: message length (0-7)
 * - 7 bytes : message data
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
 * struct DRIVER drv_ShuttleVFD
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_USB_H
#include <usb.h>
#else
#error "ShuttleVFD: libusb required"
#endif

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"		// for DIRECTION
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_gpio.h"


/*
 * Some hardware definitions
 */
// VFD USB properties
#define SHUTTLE_VFD_VENDOR_ID   0x051C
#define SHUTTLE_VFD_PRODUCT_ID1 0x0003
#define SHUTTLE_VFD_PRODUCT_ID2 0x0005	// IR-receiver included
#define SHUTTLE_VFD_INTERFACE_NUM    1

// VFD physical dimensions
#define SHUTTLE_VFD_WIDTH           20
#define SHUTTLE_VFD_HEIGHT           1

// VFD USB control message
#define SHUTTLE_VFD_PACKET_SIZE         8
#define SHUTTLE_VFD_DATA_SIZE           (SHUTTLE_VFD_PACKET_SIZE-1)
#define SHUTTLE_VFD_SUCCESS_SLEEP_USEC  25600


/* Global static data */
static char Name[] = "ShuttleVFD";
static usb_dev_handle *lcd;
static unsigned char buffer[SHUTTLE_VFD_PACKET_SIZE];

/* Issues with the display module:
 *  - Can't set cursor position. Must save full buffer here.
 *  - Can't get icons status (on or off). Must save status here.
 *  - Clear command also clear text AND icons.
 */
static unsigned char fb[SHUTTLE_VFD_WIDTH * SHUTTLE_VFD_HEIGHT];
static unsigned icons;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/* look for device on USB bus */
static int drv_ShuttleVFD_open(void)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    int vendor_id = SHUTTLE_VFD_VENDOR_ID;
    int interface = SHUTTLE_VFD_INTERFACE_NUM;

    lcd = NULL;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus != NULL; bus = bus->next) {
	for (dev = bus->devices; dev != NULL; dev = dev->next) {
	    if (dev->descriptor.idVendor == vendor_id && ((dev->descriptor.idProduct == SHUTTLE_VFD_PRODUCT_ID1) ||
							  (dev->descriptor.idProduct == SHUTTLE_VFD_PRODUCT_ID2))) {

		unsigned int v = dev->descriptor.bcdDevice;

		info("%s: found ShuttleVFD V%1d%1d.%1d%1d on bus %s device %s", Name,
		     (v & 0xF000) >> 12, (v & 0xF00) >> 8, (v & 0xF0) >> 4, (v & 0xF), bus->dirname, dev->filename);

		lcd = usb_open(dev);
	    }
	}
    }

    if (lcd != NULL) {
	if (usb_claim_interface(lcd, interface) < 0) {
	    usb_close(lcd);
	    error("%s: usb_claim_interface() failed!", Name);
	    error("%s: root permissions maybe required?", Name);
	    return -1;
	}
    } else {
	error("%s: could not find ShuttleVFD", Name);
	return -1;
    }

    return 0;
}


static int drv_ShuttleVFD_close(void)
{
    int interface = SHUTTLE_VFD_INTERFACE_NUM;

    usb_release_interface(lcd, interface);
    usb_close(lcd);
    return 0;
}


static void drv_ShuttleVFD_send(unsigned char packet[SHUTTLE_VFD_PACKET_SIZE])
{
    if (usb_control_msg(lcd, 0x21,	// requesttype
			0x09,	// request
			0x0200,	// value
			0x0001,	// index
			(char *) packet, SHUTTLE_VFD_PACKET_SIZE, 100) == SHUTTLE_VFD_PACKET_SIZE) {

	udelay(SHUTTLE_VFD_SUCCESS_SLEEP_USEC);
    } else {
	debug("usb_control_msg failed");
    }
}


/* Clear full display and icons. */
static void drv_ShuttleVFD_clear(void)
{
    // Update local framebuffer mirror
    memset(fb, ' ', SHUTTLE_VFD_HEIGHT * SHUTTLE_VFD_WIDTH);

    buffer[0] = (1 << 4) + 1;
    buffer[1] = 0x1;
    drv_ShuttleVFD_send(buffer);
}


static void drv_ShuttleVFD_reset_cursor(void)
{
    buffer[0] = (1 << 4) + 1;
    buffer[1] = 0x2;
    drv_ShuttleVFD_send(buffer);
}


/* text mode displays only */
static void drv_ShuttleVFD_write(const int row, const int col, const char *data, int len)
{
    unsigned char *p;
    int i;

    // Update local framebuffer mirror
    memcpy(fb + (row * SHUTTLE_VFD_WIDTH) + col, data, len);

    p = fb;
    len = SHUTTLE_VFD_WIDTH;

    drv_ShuttleVFD_reset_cursor();

    while (len > 0) {
	if (len > 7)
	    buffer[0] = (9 << 4) + 7;
	else
	    buffer[0] = (9 << 4) + len;

	for (i = 0; i < 7 && len--; i++) {
	    buffer[i + 1] = *p++;
	}

	drv_ShuttleVFD_send(buffer);
    }
}


static void drv_ShuttleVFD_defchar(const int ascii, const unsigned char *matrix)
{
    (void) matrix;
    debug("%s: not available (ascii=%d)", Name, ascii);
}


static int drv_ShuttleVFD_start(const char *section)
{
    char *port;

    port = cfg_get(section, "Port", NULL);

    if (port == NULL || *port == '\0') {
	error("%s: no '%s.Port' entry from %s", Name, section, cfg_source());
	return -1;
    }

    if (strcasecmp(port, "libusb") != 0) {
	error("%s: libusb expected", Name);
	error("%s: compile lcd4linux with libusb support!", Name);
	return -1;
    }

    DROWS = SHUTTLE_VFD_HEIGHT;
    DCOLS = SHUTTLE_VFD_WIDTH;

    /* open communication with the display */
    if (drv_ShuttleVFD_open() < 0) {
	return -1;
    }

    drv_ShuttleVFD_clear();	/* clear display */
    return 0;
}


/* VFD Icons. Add +1 in lcd4linux.conf (GPO1..GPIO27)
 *  0: television
 *  1: cd/dvd
 *  2: music
 *  3: radio
 *  4: clock
 *  5: pause
 *  6: play
 *  7: record
 *  8: rewind
 *  9: camera
 * 10: mute
 * 11: repeat
 * 12: reverse
 * 13: fastforward
 * 14: stop
 * 15: volume 1
 * 16: volume 2
 * ...
 * 25: volume 11
 * 26: volume 12
 */
static int drv_ShuttleVFD_icons_set(const int num, const int val)
{
    unsigned long value;

    if (num < 0 || num >= 27) {
	info("%s: num %d out of range (1..27)", Name, num);
	return -1;
    }
    // Special case for volume (icon n°16)
    if (num >= 15)
	value = (num - 15 + 1) << 15;
    else
	value = 1 << num;

    if (val > 0)
	icons |= value;
    else
	icons &= ~value;

    buffer[0] = (7 << 4) + 4;
    buffer[1] = (value >> 15) & 0x1F;
    buffer[2] = (value >> 10) & 0x1F;
    buffer[3] = (value >> 5) & 0x1F;
    buffer[4] = value & 0x1F;	// each data byte is stored on 5 bits
    drv_ShuttleVFD_send(buffer);

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none yet ! */


/****************************************/
/***        widget callbacks          ***/
/****************************************/

/* using drv_generic_text_draw(W) */
/* using drv_generic_gpio_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* supported Shuttle models */
int drv_ShuttleVFD_list(void)
{
    printf("Shuttle SG33G5M, Shuttle PF27 upgrade kit");
    return 0;
}


/* initialize driver & text display */
int drv_ShuttleVFD_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev: 975 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 2;		/* number of bytes a goto command requires */
    GPOS = 15 + 12;		/* Fancy icons on top of display */

    /* real worker functions */
    drv_generic_text_real_write = drv_ShuttleVFD_write;
    drv_generic_text_real_defchar = drv_ShuttleVFD_defchar;
    drv_generic_gpio_real_set = drv_ShuttleVFD_icons_set;

    /* start display */
    if ((ret = drv_ShuttleVFD_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "Shuttle")) {
	    sleep(3);
	    drv_ShuttleVFD_clear();
	}
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic GPIO driver */
    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    return 0;
}


/* close driver & text display */
int drv_ShuttleVFD_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    /* clear display */
    drv_ShuttleVFD_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_generic_text_quit();
    drv_generic_gpio_quit();

    debug("closing connection");
    drv_ShuttleVFD_close();

    return (0);
}


DRIVER drv_ShuttleVFD = {
    .name = Name,
    .list = drv_ShuttleVFD_list,
    .init = drv_ShuttleVFD_init,
    .quit = drv_ShuttleVFD_quit,
};
