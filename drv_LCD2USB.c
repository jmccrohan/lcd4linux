/* $Id$
 * $URL$
 *
 * driver for USB2LCD display interface
 * see http://www.harbaum.org/till/lcd2usb for schematics
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
 * 
 * $Log: drv_LCD2USB.c,v $
 * Revision 1.11  2006/07/19 01:35:31  cmay
 * Renamed keypad direction names to avoid conflict with Curses library defs.
 * Added keypad support to Curses display driver.
 *
 * Revision 1.10  2006/04/09 14:17:50  reinelt
 * autoconf/library fixes, image and graphic display inversion
 *
 * Revision 1.9  2006/03/18 14:54:36  harbaum
 * Improved USB error recovery
 *
 * Revision 1.8  2006/02/22 15:59:39  cmay
 * removed KEYPADSIZE cruft per harbaum's suggestion
 *
 * Revision 1.7  2006/02/21 21:43:03  harbaum
 * Keypad support for LCD2USB
 *
 * Revision 1.6  2006/02/12 14:32:24  harbaum
 * Configurable bus/device id
 *
 * Revision 1.5  2006/02/09 20:32:49  harbaum
 * LCD2USB bus testing, version verification ...
 *
 * Revision 1.4  2006/01/30 20:21:51  harbaum
 * LCD2USB: Added support for displays with two controllers
 *
 * Revision 1.3  2006/01/30 06:25:52  reinelt
 * added CVS Revision
 *
 * Revision 1.2  2006/01/28 15:36:17  harbaum
 * Fix: string termination bug in eval()
 *
 * Revision 1.1  2006/01/26 19:26:27  harbaum
 * Added LCD2USB support
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
#include "drv_generic_keypad.h"

/* vid/pid donated by FTDI */
#define LCD_USB_VENDOR 0x0403
#define LCD_USB_DEVICE 0xC630

/* number of buttons on display */
#define L2U_BUTTONS        (2)

#define LCD_ECHO           (0<<5)
#define LCD_CMD            (1<<5)
#define LCD_DATA           (2<<5)
#define LCD_SET            (3<<5)
#define LCD_GET            (4<<5)

/* target is a bit map for CMD/DATA */
#define LCD_CTRL_0              (1<<3)
#define LCD_CTRL_1              (1<<4)
#define LCD_BOTH           (LCD_CTRL_0 | LCD_CTRL_1)

/* target is value to set */
#define LCD_SET_CONTRAST   (LCD_SET | (0<<3))
#define LCD_SET_BRIGHTNESS (LCD_SET | (1<<3))
#define LCD_SET_RESERVED0  (LCD_SET | (2<<3))
#define LCD_SET_RESERVED1  (LCD_SET | (3<<3))

/* target is value to get */
#define LCD_GET_FWVER      (LCD_GET | (0<<3))
#define LCD_GET_BUTTONS    (LCD_GET | (1<<3))
#define LCD_GET_CTRL       (LCD_GET | (2<<3))
#define LCD_GET_RESERVED1  (LCD_GET | (3<<3))

static char Name[] = "LCD2USB";
static char *device_id = NULL, *bus_id = NULL;

static usb_dev_handle *lcd;
static int controllers = 0;

extern int usb_debug;
extern int got_signal;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_L2U_open(char *bus_id, char *device_id)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    lcd = NULL;

    info("%s: scanning USB for LCD2USB interface ...", Name);

    if (bus_id != NULL)
	info("%s: scanning for bus id: %s", Name, bus_id);

    if (device_id != NULL)
	info("%s: scanning for device id: %s", Name, device_id);

    usb_debug = 0;

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
			info("%s: found LCD2USB interface on bus %s device %s", Name, bus->dirname, dev->filename);
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


static int drv_L2U_close(void)
{
    usb_release_interface(lcd, 0);
    usb_close(lcd);

    return 0;
}


static int drv_L2U_send(int request, int value, int index)
{
    if (usb_control_msg(lcd, USB_TYPE_VENDOR, request, value, index, NULL, 0, 1000) < 0) {
	error("%s: USB request failed! Trying to reconnect device.", Name);

	usb_release_interface(lcd, 0);
	usb_close(lcd);

	// try to close and reopen connection
	if (drv_L2U_open(bus_id, device_id) < 0) {
	    error("%s: could not re-detect LCD2USB USB LCD", Name);
	    got_signal = -1;
	    return -1;
	}
	// and try to re-send command
	if (usb_control_msg(lcd, USB_TYPE_VENDOR, request, value, index, NULL, 0, 1000) < 0) {
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
#define ECHO_NUM 100

int drv_L2U_echo(void)
{
    int i, nBytes, errors = 0;
    unsigned short val, ret;

    for (i = 0; i < ECHO_NUM; i++) {
	val = rand() & 0xffff;

	nBytes = usb_control_msg(lcd,
				 USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
				 LCD_ECHO, val, 0, (char *) &ret, sizeof(ret), 1000);

	if (nBytes < 0) {
	    error("%s: USB request failed!", Name);
	    return -1;
	}

	if (val != ret)
	    errors++;
    }

    if (errors) {
	error("%s: ERROR, %d out of %d echo transfers failed!", Name, errors, ECHO_NUM);
	return -1;
    } else
	info("%s: echo test successful", Name);

    return 0;
}

/* get a value from the lcd2usb interface */
static int drv_L2U_get(unsigned char cmd)
{
    unsigned char buffer[2];
    int nBytes;

    /* send control request and accept return value */
    nBytes = usb_control_msg(lcd,
			     USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			     cmd, 0, 0, (char *) buffer, sizeof(buffer), 1000);

    if (nBytes < 0) {
	error("%s: USB request failed!", Name);
	return -1;
    }

    return buffer[0] + 256 * buffer[1];
}

/* get lcd2usb interface firmware version */
static void drv_L2U_get_version(void)
{
    int ver = drv_L2U_get(LCD_GET_FWVER);

    if (ver != -1)
	info("%s: firmware version %d.%02d", Name, ver & 0xff, ver >> 8);
    else
	error("%s: unable to read firmware version", Name);
}

/* get the bit mask of installed LCD controllers (0 = no */
/* lcd found, 1 = single controller display, 3 = dual */
/* controller display */
static void drv_L2U_get_controllers(void)
{
    controllers = drv_L2U_get(LCD_GET_CTRL);

    if (controllers != -1) {
	if (controllers)
	    info("%s: installed controllers: %s%s", Name,
		 (controllers & 1) ? "CTRL0" : "", (controllers & 2) ? " CTRL1" : "");
	else
	    error("%s: no controllers found", Name);
    } else {
	error("%s: unable to read installed controllers", Name);
	controllers = 0;	// don't access any controllers 
    }

    // convert into controller map matching our protocol
    controllers = ((controllers & 1) ? LCD_CTRL_0 : 0) | ((controllers & 2) ? LCD_CTRL_1 : 0);
}

/* get state of the two optional buttons */
static unsigned long drv_L2U_get_buttons(void)
{
    int buttons = drv_L2U_get(LCD_GET_BUTTONS);

    if (buttons == -1) {
	error("%s: unable to read button state", Name);
	buttons = 0;
    }

    return buttons;
}


/* to increase performance, a little buffer is being used to */
/* collect command bytes of the same type before transmitting them */
#define BUFFER_MAX_CMD 4	/* current protocol supports up to 4 bytes */
int buffer_current_type = -1;	/* nothing in buffer yet */
int buffer_current_fill = 0;	/* -"- */
unsigned char buffer[BUFFER_MAX_CMD];

/* command format: 
 * 7 6 5 4 3 2 1 0
 * C C C T T R L L
 *
 * TT = target bit map 
 * R = reserved for future use, set to 0
 * LL = number of bytes in transfer - 1 
 */

/* flush command queue due to buffer overflow / content */
/* change or due to explicit request */
static void drv_L2U_flush(void)
{
    int request, value, index;

    /* anything to flush? ignore request if not */
    if (buffer_current_type == -1)
	return;

    /* build request byte */
    request = buffer_current_type | (buffer_current_fill - 1);

    /* fill value and index with buffer contents. endianess should IMHO not */
    /* be a problem, since usb_control_msg() will handle this. */
    value = buffer[0] | (buffer[1] << 8);
    index = buffer[2] | (buffer[3] << 8);

    if (controllers) {
	/* send current buffer contents */
	drv_L2U_send(request, value, index);
    }

    /* buffer is now free again */
    buffer_current_type = -1;
    buffer_current_fill = 0;
}

/* enqueue a command into the buffer */
static void drv_L2U_enqueue(int command_type, int value)
{
    if ((buffer_current_type >= 0) && (buffer_current_type != command_type))
	drv_L2U_flush();

    /* add new item to buffer */
    buffer_current_type = command_type;
    buffer[buffer_current_fill++] = value;

    /* flush buffer if it's full */
    if (buffer_current_fill == BUFFER_MAX_CMD)
	drv_L2U_flush();
}

static void drv_L2U_command(const unsigned char ctrl, const unsigned char cmd)
{
    drv_L2U_enqueue(LCD_CMD | (ctrl & controllers), cmd);
}


static void drv_L2U_clear(void)
{
    drv_L2U_command(LCD_BOTH, 0x01);	/* clear display */
    drv_L2U_command(LCD_BOTH, 0x03);	/* return home */
}

static void drv_L2U_write(int row, const int col, const char *data, int len)
{
    int pos, ctrl = LCD_CTRL_0;

    /* displays with more two rows and 20 columns have a logical width */
    /* of 40 chars and use more than one controller */
    if ((DROWS > 2) && (DCOLS > 20) && (row > 1)) {
	/* use second controller */
	row -= 2;
	ctrl = LCD_CTRL_1;
    }

    /* 16x4 Displays use a slightly different layout */
    if (DCOLS == 16 && DROWS == 4) {
	pos = (row % 2) * 64 + (row / 2) * 16 + col;
    } else {
	pos = (row % 2) * 64 + (row / 2) * 20 + col;
    }

    drv_L2U_command(ctrl, 0x80 | pos);

    while (len--) {
	drv_L2U_enqueue(LCD_DATA | (ctrl & controllers), *data++);
    }

    drv_L2U_flush();
}

static void drv_L2U_defchar(const int ascii, const unsigned char *matrix)
{
    int i;

    drv_L2U_command(LCD_BOTH, 0x40 | 8 * ascii);

    for (i = 0; i < 8; i++) {
	drv_L2U_enqueue(LCD_DATA | (LCD_BOTH & controllers), *matrix++ & 0x1f);
    }

    drv_L2U_flush();
}

static int drv_L2U_contrast(int contrast)
{
    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    drv_L2U_send(LCD_SET_CONTRAST, contrast, 0);

    return contrast;
}

static int drv_L2U_brightness(int brightness)
{
    if (brightness < 0)
	brightness = 0;
    if (brightness > 255)
	brightness = 255;

    drv_L2U_send(LCD_SET_BRIGHTNESS, brightness, 0);

    return brightness;
}

static void drv_L2U_timer(void __attribute__ ((unused)) * notused)
{
    static unsigned long last_but = 0;
    unsigned long new_but = drv_L2U_get_buttons();
    int i;

    /* check if button state changed */
    if (new_but ^ last_but) {

	/* send single keypad events for all changed buttons */
	for (i = 0; i < L2U_BUTTONS; i++)
	    if ((new_but & (1 << i)) ^ (last_but & (1 << i)))
		drv_generic_keypad_press(((new_but & (1 << i)) ? 0x80 : 0) | i);
    }

    last_but = new_but;
}

static int drv_L2U_keypad(const int num)
{
    int val = 0;

    /* check for key press event */
    if (num & 0x80)
	val = WIDGET_KEY_PRESSED;
    else
	val = WIDGET_KEY_RELEASED;

    if ((num & 0x7f) == 0)
	val += WIDGET_KEY_UP;

    if ((num & 0x7f) == 1)
	val += WIDGET_KEY_DOWN;

    return val;
}

static int drv_L2U_start(const char *section, const int quiet)
{
    int contrast, brightness;
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

    if (drv_L2U_open(bus_id, device_id) < 0) {
	error("%s: could not find a LCD2USB USB LCD", Name);
	return -1;
    }

    /* test interface reliability */
    drv_L2U_echo();

    /* get some infos from the interface */
    drv_L2U_get_version();
    drv_L2U_get_controllers();
    if (!controllers)
	return -1;

    /* regularly request key state */
    /* Fixme: make 100msec configurable */
    timer_add(drv_L2U_timer, NULL, 100, 0);

    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_L2U_contrast(contrast);
    }

    if (cfg_number(section, "Brightness", 0, 0, 255, &brightness) > 0) {
	drv_L2U_brightness(brightness);
    }

    drv_L2U_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.harbaum.org/till")) {
	    sleep(3);
	    drv_L2U_clear();
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

    contrast = drv_L2U_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}


static void plugin_brightness(RESULT * result, RESULT * arg1)
{
    double brightness;

    brightness = drv_L2U_brightness(R2N(arg1));
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
int drv_L2U_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_L2U_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Revision: 1.11 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 2;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_L2U_write;
    drv_generic_text_real_defchar = drv_L2U_defchar;
    drv_generic_keypad_real_press = drv_L2U_keypad;

    /* start display */
    if ((ret = drv_L2U_start(section, quiet)) != 0)
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

    /* initialize generic key pad driver */
    if ((ret = drv_generic_keypad_init(section, Name)) != 0)
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
    AddFunction("LCD::brightness", 1, plugin_brightness);

    return 0;
}


/* close driver & display */
int drv_L2U_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();
    drv_generic_keypad_quit();

    /* clear display */
    drv_L2U_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("That's all, folks!", NULL);
    }

    debug("closing USB connection");
    drv_L2U_close();

    return (0);
}


DRIVER drv_LCD2USB = {
  name:Name,
  list:drv_L2U_list,
  init:drv_L2U_init,
  quit:drv_L2U_quit,
};
