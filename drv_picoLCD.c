/* $Id$
 * $URL$
 *
 * driver for picoLCD displays from mini-box.com
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Copyright (C) 2007 Nicu Pavel, Mini-Box.com <npavel@mini-box.com>
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
 * struct DRIVER drv_picoLCD
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
#include "drv_generic_gpio.h"
#include "drv_generic_keypad.h"



#define picoLCD_VENDOR  0x04d8
#define picoLCD_DEVICE  0x0002

static char Name[] = "picoLCD";

static int use_libusb = 1;
static unsigned int gpo = 0;

static char *Buffer;
static char *BufPtr;

#ifdef HAVE_USB_H

static usb_dev_handle *lcd;
static int interface;

extern int usb_debug;

#endif



/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

#ifdef HAVE_USB_H

static int drv_pL_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;
    char buf[1024];
    int ret;

    lcd = NULL;

    info("%s: scanning for picoLCD...", Name);

    usb_debug = 0;

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == picoLCD_VENDOR) && (dev->descriptor.idProduct == picoLCD_DEVICE)) {

		info("%s: found picoLCD on bus %s device %s", Name, bus->dirname, dev->filename);

		interface = 0;
		lcd = usb_open(dev);

		ret = usb_get_driver_np(lcd, 0, buf, sizeof(buf));

		if (ret == 0) {
		    info("interface 0 already claimed attempting to detach it\n");
		    ret = usb_detach_kernel_driver_np(lcd, 0);
		    printf("usb_detach_kernel_driver_np returned %d\n", ret);
		}

		if (usb_claim_interface(lcd, interface) < 0) {
		    error("%s: usb_claim_interface() failed!", Name);
		    return -1;
		}
		return 0;
	    }
	}
    }
    error("%s: could not find a picoLCD", Name);
    return -1;
}


static int drv_pL_close(void)
{
    usb_release_interface(lcd, interface);
    usb_close(lcd);

    return 0;
}

#endif


static void drv_pL_send(void)
{

#if 0
    struct timeval now, end;
    gettimeofday(&now, NULL);
#endif

    if (use_libusb) {
#ifdef HAVE_USB_H
	usb_bulk_write(lcd, USB_ENDPOINT_OUT + 1, Buffer, BufPtr - Buffer, 1000);
#endif
    }
#if 0
    gettimeofday(&end, NULL);
    debug("send %d bytes in %d usec (%d usec/byte)", BufPtr - Buffer,
	  (1000000 * (end.tv_sec - now.tv_sec) + end.tv_usec - now.tv_usec),
	  (1000000 * (end.tv_sec - now.tv_sec) + end.tv_usec - now.tv_usec) / (BufPtr - Buffer));
#endif

    BufPtr = Buffer;
}


static void drv_pL_command(const unsigned char cmd)
{
    *BufPtr++ = cmd;
}


static void drv_pL_clear(void)
{
    drv_pL_command(0x94);	/* clear display */
    drv_pL_send();		/* flush buffer */
}

static int drv_pL_contrast(int contrast)
{

    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    drv_pL_command(0x92);
    drv_pL_command(contrast);

    drv_pL_send();

    return contrast;
}


static int drv_pL_backlight(int backlight)
{
    if (backlight < 0)
	backlight = 0;
    if (backlight > 1)
	backlight = 1;

    drv_pL_command(0x91);
    drv_pL_command(backlight);

    drv_pL_send();

    return backlight;
}

static int drv_pL_gpo(int num, int val)
{
    if (num < 0)
	num = 0;
    if (num > 7)
	num = 7;

    if (val < 0)
	val = 0;
    if (val > 1)
	val = 1;

    /* set led bit to 1 or 0 */
    if (val)
	gpo |= 1 << num;
    else
	gpo &= ~(1 << num);

    drv_pL_command(0x81);
    drv_pL_command(gpo);

    return val;
}


static void drv_pL_write(const int row, const int col, const char *data, int len)
{

    drv_pL_command(0x98);
    drv_pL_command(row);
    drv_pL_command(col);
    drv_pL_command(len);

    while (len--) {
	if (*data == 0)
	    *BufPtr++ = 0;
	*BufPtr++ = *data++;
    }

    drv_pL_send();
}

static void drv_pL_defchar(const int ascii, const unsigned char *matrix)
{
    int i;

    drv_pL_command(0x9c);
    drv_pL_command(ascii);

    for (i = 0; i < 8; i++) {
	if ((*matrix & 0x1f) == 0)
	    *BufPtr++ = 0;
	*BufPtr++ = *matrix++ & 0x1f;
    }

    drv_pL_send();
}


static int drv_pL_start(const char *section, const int quiet)
{
    int rows = -1, cols = -1;
    int value;
    char *s;

#ifdef HAVE_USB_H
    use_libusb = 1;
    debug("using libusb");
#else
    error("%s: lcd4linux was compiled without libusb support!", Name);
    return -1;
#endif

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


#ifdef HAVE_USB_H
    if (drv_pL_open() < 0) {
	return -1;
    }
#endif


    /* Init the command buffer */
    Buffer = (char *) malloc(1024);
    if (Buffer == NULL) {
	error("%s: coommand buffer could not be allocated: malloc() failed", Name);
	return -1;
    }
    BufPtr = Buffer;

    if (cfg_number(section, "Contrast", 0, 0, 255, &value) > 0) {
	info("Setting contrast to %d\n", value);
	drv_pL_contrast(value);
    }

    if (cfg_number(section, "Backlight", 0, 0, 1, &value) > 0) {
	info("Setting backlight to %d\n", value);
	drv_pL_backlight(value);
    }

    drv_pL_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "http://www.picolcd.com")) {
	    sleep(3);
	    drv_pL_clear();
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

    contrast = drv_pL_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_pL_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}

static void plugin_gpo(RESULT * result, RESULT * argv[])
{
    double gpo;
    gpo = drv_pL_gpo(R2N(argv[0]), R2N(argv[1]));
    SetResult(&result, R_NUMBER, &gpo);
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
int drv_pL_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_pL_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GPOS = 8;
    INVALIDATE = 1;
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_pL_write;
    drv_generic_text_real_defchar = drv_pL_defchar;
    drv_generic_gpio_real_set = drv_pL_gpo;

    /* start display */
    if ((ret = drv_pL_start(section, quiet)) != 0)
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

    drv_generic_text_bar_add_segment(0, 0, 255, 32);

    /* GPO's init */
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
    AddFunction("LCD::contrast", -1, plugin_contrast);
    AddFunction("LCD::backlight", -1, plugin_backlight);
    AddFunction("LCD::gpo", -1, plugin_gpo);

    return 0;
}


/* close driver & display */
int drv_pL_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    /* flush buffer */
    drv_pL_send();

    drv_generic_text_quit();

    /* clear display */
    drv_pL_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    if (use_libusb) {
#ifdef HAVE_USB_H
	drv_pL_close();
#endif
    }

    if (Buffer) {
	free(Buffer);
	Buffer = NULL;
	BufPtr = Buffer;
    }

    return (0);
}


DRIVER drv_picoLCD = {
  name:Name,
  list:drv_pL_list,
  init:drv_pL_init,
  quit:drv_pL_quit,
};
