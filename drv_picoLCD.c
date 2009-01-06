/* $Id$
 * $URL$
 *
 * driver for picoLCD displays from mini-box.com
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
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
#include "drv_generic_gpio.h"
#include "drv_generic_keypad.h"



#define picoLCD_VENDOR  0x04d8
#define picoLCD_DEVICE  0x0002

static char Name[] = "picoLCD";

static unsigned int gpo = 0;

static char *Buffer;
static char *BufPtr;

static usb_dev_handle *lcd;



/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_pL_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;
    char driver[1024];
    char product[1024];
    char manufacturer[1024];
    char serialnumber[1024];
    int ret;

    lcd = NULL;

    info("%s: scanning for picoLCD...", Name);

    usb_set_debug(0);

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == picoLCD_VENDOR) && (dev->descriptor.idProduct == picoLCD_DEVICE)) {

		info("%s: found picoLCD on bus %s device %s", Name, bus->dirname, dev->filename);

		lcd = usb_open(dev);

		ret = usb_get_driver_np(lcd, 0, driver, sizeof(driver));

		if (ret == 0) {
		    info("%s: interface 0 already claimed by '%s'", Name, driver);
		    info("%s: attempting to detach driver...", Name);
		    if (usb_detach_kernel_driver_np(lcd, 0) < 0) {
			error("%s: usb_detach_kernel_driver_np() failed!", Name);
			return -1;
		    }
		}

		usb_set_configuration(lcd, 1);
		usleep(100);

		if (usb_claim_interface(lcd, 0) < 0) {
		    error("%s: usb_claim_interface() failed!", Name);
		    return -1;
		}

		usb_set_altinterface(lcd, 0);

		usb_get_string_simple(lcd, dev->descriptor.iProduct, product, sizeof(product));
		usb_get_string_simple(lcd, dev->descriptor.iManufacturer, manufacturer, sizeof(manufacturer));
		usb_get_string_simple(lcd, dev->descriptor.iSerialNumber, serialnumber, sizeof(serialnumber));

		info("%s: Manufacturer='%s' Product='%s' SerialNumber='%s'", Name, manufacturer, product, serialnumber);

		return 0;
	    }
	}
    }
    error("%s: could not find a picoLCD", Name);
    return -1;
}


static int drv_pL_close(void)
{
    usb_release_interface(lcd, 0);
    usb_close(lcd);

    return 0;
}


static void drv_pL_send(unsigned char *data, int size)
{
    usb_interrupt_write(lcd, USB_ENDPOINT_OUT + 1, (char *) data, size, 1000);
}

static int drv_pL_read(unsigned char *data, int size)
{
    return usb_interrupt_read(lcd, USB_ENDPOINT_OUT + 1, (char *) data, size, 1000);
}



static void drv_pL_clear(void)
{
    unsigned char cmd[1] = { 0x94 };	/* clear display */
    drv_pL_send(cmd, 1);
}

static int drv_pL_contrast(int contrast)
{
    unsigned char cmd[2] = { 0x92 };	/* set contrast */

    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    cmd[1] = contrast;
    drv_pL_send(cmd, 2);

    return contrast;
}


static int drv_pL_backlight(int backlight)
{
    unsigned char cmd[2] = { 0x91 };	/* set backlight */

    if (backlight < 0)
	backlight = 0;
    if (backlight > 1)
	backlight = 1;

    cmd[1] = backlight;
    drv_pL_send(cmd, 2);

    return backlight;
}

#define _USBLCD_MAX_DATA_LEN          24
#define IN_REPORT_KEY_STATE           0x11
static int drv_pL_gpi(__attribute__ ((unused)) int num)
{
    int ret;
    unsigned char read_packet[_USBLCD_MAX_DATA_LEN];
    ret = drv_pL_read(read_packet, _USBLCD_MAX_DATA_LEN);
    if ((ret > 0) && (read_packet[0] == IN_REPORT_KEY_STATE)) {
	debug("picoLCD: pressed key= 0x%02x\n", read_packet[1]);
	return read_packet[1];
    }
    return 0;
}

static int drv_pL_gpo(int num, int val)
{
    unsigned char cmd[2] = { 0x81 };	/* set GPO */

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

    cmd[1] = gpo;
    drv_pL_send(cmd, 2);

    return val;
}


static void drv_pL_write(const int row, const int col, const char *data, int len)
{
    unsigned char cmd[64];
    int i;

    cmd[0] = 0x98;		/* goto/write */
    cmd[1] = row;
    cmd[2] = col;
    cmd[3] = len;

    i = 4;
    while (len--) {
	cmd[i++] = *data++;
    }

    drv_pL_send(cmd, i);
}

static void drv_pL_defchar(const int ascii, const unsigned char *matrix)
{
    unsigned char cmd[10] = { 0x9c };	/* define character */
    int i;

    cmd[1] = ascii;
    for (i = 0; i < 8; i++) {
	cmd[i + 2] = *matrix++ & 0x1f;
    }

    drv_pL_send(cmd, 10);
}


static int drv_pL_start(const char *section, const int quiet)
{
    int rows = -1, cols = -1;
    int value;
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

    if (drv_pL_open() < 0) {
	return -1;
    }

    /* Init the command buffer */
    Buffer = (char *) malloc(1024);
    if (Buffer == NULL) {
	error("%s: coommand buffer could not be allocated: malloc() failed", Name);
	return -1;
    }
    BufPtr = Buffer;

    if (cfg_number(section, "Contrast", 0, 0, 255, &value) > 0) {
	info("Setting contrast to %d", value);
	drv_pL_contrast(value);
    }

    if (cfg_number(section, "Backlight", 0, 0, 1, &value) > 0) {
	info("Setting backlight to %d", value);
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
    GPIS = 1;
    INVALIDATE = 1;
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_pL_write;
    drv_generic_text_real_defchar = drv_pL_defchar;
    drv_generic_gpio_real_set = drv_pL_gpo;
    drv_generic_gpio_real_get = drv_pL_gpi;

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

    drv_generic_text_quit();

    /* clear display */
    drv_pL_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_pL_close();

    if (Buffer) {
	free(Buffer);
	Buffer = NULL;
	BufPtr = Buffer;
    }

    return (0);
}


DRIVER drv_picoLCD = {
    .name = Name,
    .list = drv_pL_list,
    .init = drv_pL_init,
    .quit = drv_pL_quit,
};
