/* $Id$
 * $URL$
 *
 * driver for Futaba MDM166A Graphic(96x16) vf-displays
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Copyright (C) 2011 Andreas Brachold <anbr at users.sourceforge.net>
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
 * struct DRIVER drv_MDM166A
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libusb-1.0/libusb.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "timer.h"
#include "drv.h"
#include "drv_generic_graphic.h"
#include "drv_generic_gpio.h"

// Values for transaction's data packet.
static const int CONTROL_REQUEST_TYPE_OUT =
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;

// From the HID spec:
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;

static const int MAX_CONTROL_OUT_TRANSFER_SIZE = 62;

static const int INTERFACE_NUMBER = 0;
static const int TIMEOUT_MS = 5000;

// Defines from display datasheet
static const int VENDOR_ID = 0x019c2;
static const int PRODUCT_ID = 0x06a11;

static const unsigned char ICON_PLAY = 0x00;	//Play
static const unsigned char ICON_PAUSE = 0x01;	//Pause
static const unsigned char ICON_RECORD = 0x02;	//Record
static const unsigned char ICON_MESSAGE = 0x03;	//Message symbol (without the inner @)
static const unsigned char ICON_MSGAT = 0x04;	//Message @
static const unsigned char ICON_MUTE = 0x05;	//Mute
static const unsigned char ICON_WLAN1 = 0x06;	//WLAN (tower base)
static const unsigned char ICON_WLAN2 = 0x07;	//WLAN strength (1 of 3)
static const unsigned char ICON_WLAN3 = 0x08;	//WLAN strength (2 of 3)
static const unsigned char ICON_WLAN4 = 0x09;	//WLAN strength (3 of 3)
static const unsigned char ICON_VOLUME = 0x0A;	//Volume (the word)
static const unsigned char ICON_VOL1 = 0x0B;	//Volume level 1 of 14
static const unsigned char ICON_VOL2 = 0x0C;	//Volume level 2 of 14
static const unsigned char ICON_VOL3 = 0x0D;	//Volume level 3 of 14
static const unsigned char ICON_VOL4 = 0x0E;	//Volume level 4 of 14
static const unsigned char ICON_VOL5 = 0x0F;	//Volume level 5 of 14
static const unsigned char ICON_VOL6 = 0x10;	//Volume level 6 of 14
static const unsigned char ICON_VOL7 = 0x11;	//Volume level 7 of 14
static const unsigned char ICON_VOL8 = 0x12;	//Volume level 8 of 14
static const unsigned char ICON_VOL9 = 0x13;	//Volume level 9 of 14
static const unsigned char ICON_VOL10 = 0x14;	//Volume level 10 of 14
static const unsigned char ICON_VOL11 = 0x15;	//Volume level 11 of 14
static const unsigned char ICON_VOL12 = 0x16;	//Volume level 12 of 14
static const unsigned char ICON_VOL13 = 0x17;	//Volume level 13 of 14
static const unsigned char ICON_VOL14 = 0x18;	//Volume level 14 of 14
static const unsigned char ICON_LAST = 0x19;	//Marker size of icon

static const unsigned char STATE_OFF = 0x00;	//Symbol off
static const unsigned char STATE_ON = 0x01;	//Symbol on
static const unsigned char STATE_ONHIGH = 0x02;	//Symbol on, high intensity, can only be used with the volume symbols

static const unsigned char CMD_PREFIX = 0x1b;
static const unsigned char CMD_SETCLOCK = 0x00;	//Actualize the time of the display
static const unsigned char CMD_SMALLCLOCK = 0x01;	//Display small clock on display
static const unsigned char CMD_BIGCLOCK = 0x02;	//Display big clock on display
static const unsigned char CMD_SETSYMBOL = 0x30;	//Enable or disable symbol
static const unsigned char CMD_SETDIMM = 0x40;	//Set the dimming level of the display
static const unsigned char CMD_RESET = 0x50;	//Reset all configuration data to default and clear
static const unsigned char CMD_SETRAM = 0x60;	//Set the actual graphics RAM offset for next data write
static const unsigned char CMD_SETPIXEL = 0x70;	//Write pixel data to RAM of the display
static const unsigned char CMD_TEST1 = 0xf0;	//Show vertical test pattern
static const unsigned char CMD_TEST2 = 0xf1;	//Show horizontal test pattern

static const unsigned char TIME_12 = 0x00;	//12 hours format
static const unsigned char TIME_24 = 0x01;	//24 hours format

static const unsigned char BRIGHT_OFF = 0x00;	//Display off
static const unsigned char BRIGHT_DIMM = 0x01;	//Display dimmed

static int nSizeYb = 2;
static int SCREEN_H = 16;
static int SCREEN_W = 96;
static int NeedRefresh = 0;
static int minX = 1;
static int maxX = 0;
static unsigned int lastIconState = 0;

#if 1
#define DEBUG(x) debug("%s(): %s", __FUNCTION__, x);
#else
#define DEBUG(x)
#endif

static char Name[] = "MDM166A";
static unsigned char *mdm166a_framebuffer;

/* used to display white text on black background or inverse */
static unsigned char nDrawInverted = 1;

static struct libusb_device_handle *devh = NULL;
static int mdm166a_nQueue = 0;
static unsigned char *mdm166a_Queue;
static const int mdm166a_nQueueMax = 1024;


static const char *usberror(int ret)
{
    switch (ret) {
    case LIBUSB_SUCCESS:
	return "Success (no error).";

    case LIBUSB_ERROR_IO:
	return "Input/output error.";

    case LIBUSB_ERROR_INVALID_PARAM:
	return "Invalid parameter.";

    case LIBUSB_ERROR_ACCESS:
	return "Access denied (insufficient permissions).";

    case LIBUSB_ERROR_NO_DEVICE:
	return "No such device (it may have been disconnected).";

    case LIBUSB_ERROR_NOT_FOUND:
	return "Entity not found.";

    case LIBUSB_ERROR_BUSY:
	return "Resource busy.";

    case LIBUSB_ERROR_TIMEOUT:
	return "Operation timed out.";

    case LIBUSB_ERROR_OVERFLOW:
	return "Overflow.";

    case LIBUSB_ERROR_PIPE:
	return "Pipe error.";

    case LIBUSB_ERROR_INTERRUPTED:
	return "System call interrupted (perhaps due to signal).";

    case LIBUSB_ERROR_NO_MEM:
	return "Insufficient memory.";

    case LIBUSB_ERROR_NOT_SUPPORTED:
	return "Operation not supported or unimplemented on this platform.";

    case LIBUSB_ERROR_OTHER:
	return "Other error. ";
    }
    return "unknown error";
}

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_MDM166A_open(void)
{
    int result;
    int ready = -1;

    info("%s: scanning for display...", Name);

    //Initialize libusb
    result = libusb_init(NULL);
    if (result >= 0) {
	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (devh != NULL) {
	    // a targavfd has been detected.
	    // Detach the hidusb driver from the HID to enable using libusb.
	    libusb_detach_kernel_driver(devh, INTERFACE_NUMBER);
	    {
		result = libusb_claim_interface(devh, INTERFACE_NUMBER);
		if (result >= 0) {
		    ready = 0;
		} else {
		    error("%s: libusb_claim_interface error! %s (%d)", Name, usberror(result), result);
		}
	    }
	} else {
	    error("%s: Unable to find the device!", Name);
	}
    } else {
	error("%s: Unable to initialize libusb! %s (%d)", Name, usberror(result), result);
    }
    if (ready != 0) {
	if (devh)
	    libusb_release_interface(devh, 0);
	devh = NULL;
    }
    return ready;
}

static int drv_MDM166A_close(void)
{
    if (devh != NULL) {
	int result = libusb_release_interface(devh, 0);
	if (result < 0) {
	    error("%s: libusb_release_interface failed! %s (%d)", Name, usberror(result), result);
	}
	libusb_close(devh);
	devh = NULL;
    }
    // Deinitialize libusb 
    libusb_exit(NULL);

    return 0;
}

static int drv_MDM166A_QueueFlush()
{
    int sent, i, frame;
    unsigned char buf[MAX_CONTROL_OUT_TRANSFER_SIZE + 1];

    int cnt = 0;
    int length = mdm166a_nQueue;

    while (length > 0) {

	frame = (length > MAX_CONTROL_OUT_TRANSFER_SIZE ? MAX_CONTROL_OUT_TRANSFER_SIZE : length);
	buf[0] = (unsigned char) frame;
	for (i = 0; i < MAX_CONTROL_OUT_TRANSFER_SIZE && length > 0; ++i) {
	    buf[i + 1] = mdm166a_Queue[cnt++];
	    length--;
	}
	sent = libusb_control_transfer(devh,
				       CONTROL_REQUEST_TYPE_OUT,
				       HID_SET_REPORT,
				       (HID_REPORT_TYPE_OUTPUT << 8) | 0x00,
				       INTERFACE_NUMBER, buf, frame + 1, TIMEOUT_MS);
	if (sent <= 0) {
	    error("%s: libusb_control_transfer failed : %s (%d)", Name, usberror(sent), sent);
	    mdm166a_nQueue = 0;
	    return -1;
	}
    }
    mdm166a_nQueue = 0;
    return 0;
}


static void drv_MDM166A_QueueCmd(unsigned char cmd)
{

    if (mdm166a_nQueue + 2 >= mdm166a_nQueueMax) {
	drv_MDM166A_QueueFlush();
    }
    mdm166a_Queue[mdm166a_nQueue++] = CMD_PREFIX;
    mdm166a_Queue[mdm166a_nQueue++] = cmd;
}

static void drv_MDM166A_QueueData(unsigned char data)
{

    if (mdm166a_nQueue + 1 >= mdm166a_nQueueMax) {
	drv_MDM166A_QueueFlush();
    }
    mdm166a_Queue[mdm166a_nQueue++] = data;
}

/* for graphic displays only */
static void drv_MDM166A_blit(const int row, const int col, const int height, const int width)
{
    int n, x, y, yb;
    unsigned char c;

    if (NeedRefresh == 0) {
	minX = width;
	maxX = 0;
    }

    for (y = row; y < row + height && y < SCREEN_H; ++y)
	for (x = col; x < col + width && x < SCREEN_W; ++x) {
	    yb = (y / 8);
	    n = x + (yb * SCREEN_W);

	    c = *(mdm166a_framebuffer + n);
	    if (drv_generic_graphic_black(y, x) ^ nDrawInverted)
		c |= 0x80 >> (y % 8);
	    else
		c &= ~(0x80 >> (y % 8));

	    if (c != *(mdm166a_framebuffer + n)) {
		*(mdm166a_framebuffer + n) = c;
		minX = (minX < x) ? minX : x;
		maxX = (maxX > (x + 1)) ? maxX : (x + 1);
		NeedRefresh = 1;
	    }
	}
}

static void drv_MDM166A_Flush()
{

    int n, x, yb;
    if (NeedRefresh) {

	maxX = (maxX < SCREEN_W) ? maxX : SCREEN_W;

	unsigned int nData = (maxX - minX) * nSizeYb;
	if (nData) {
	    // send data to display, controller
	    drv_MDM166A_QueueCmd(CMD_SETRAM);
	    drv_MDM166A_QueueData(minX * nSizeYb);
	    drv_MDM166A_QueueCmd(CMD_SETPIXEL);
	    drv_MDM166A_QueueData(nData);

	    for (x = minX; x < maxX; ++x)
		for (yb = 0; yb < nSizeYb; ++yb) {
		    n = x + (yb * SCREEN_W);
		    drv_MDM166A_QueueData((*(mdm166a_framebuffer + n)));
		}
	}
	drv_MDM166A_QueueFlush();
	NeedRefresh = 0;
    }
}

void drv_MDM166A_clear(void)
{
    debug("In %s", __FUNCTION__);

    drv_MDM166A_QueueCmd(CMD_RESET);
    drv_MDM166A_QueueFlush();
}

/**
 * Sets the brightness of the display.
 *
 * \param nBrightness The value the brightness (0 = off
 *                  1 = half brightness; 2 = highest brightness).
 */
void drv_MDM166A_QueueBrightness(int nBrightness)
{
    if (nBrightness < 0) {
	nBrightness = 0;
    } else if (nBrightness > 2) {
	nBrightness = 2;
    }
    drv_MDM166A_QueueCmd(CMD_SETDIMM);
    drv_MDM166A_QueueData((unsigned char) (nBrightness));
}

static int drv_MDM166A_Brightness(int nBrightness)
{
    debug("In %s", __FUNCTION__);

    int n = nBrightness;
    if (n < 0) {
	n = 0;
    } else if (n > 2) {
	n = 2;
    }
    drv_MDM166A_QueueBrightness(n);
    drv_MDM166A_QueueFlush();

    return n;
}

static void drv_MDM166A_icons(const int num, const int val)
{
    unsigned int state = lastIconState;
    if (val > 0)
	state |= 1 << num;
    else
	state &= ~(1 << num);

    if (state != lastIconState) {
	drv_MDM166A_QueueCmd(CMD_SETSYMBOL);
	drv_MDM166A_QueueData(num);
	drv_MDM166A_QueueData((val > 0) ? STATE_ON : STATE_OFF);
	drv_MDM166A_QueueFlush();
    }
    lastIconState = state;
}

static int drv_MDM166A_start(const char *section, const int quiet)
{
    int value = 0;
    char *s;

    if (sscanf(s = cfg_get(section, "Size", "96x16"), "%dx%d", &SCREEN_W, &SCREEN_H) != 2 || SCREEN_W < 1
	|| SCREEN_H < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "Font", "6x8"), "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad %s.Font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (cfg_number(section, "Inverted", 0, 0, 1, &value) > 0) {
	info("Setting display inverted to %d", value);
	if (value > 0)
	    nDrawInverted = 1;
	else
	    nDrawInverted = 0;
    }

    GPOS = ICON_LAST;		/* Icons on display */
    DROWS = SCREEN_H;
    DCOLS = SCREEN_W;
    nSizeYb = ((SCREEN_H + 7) / 8);

    /* Init the command queue */
    mdm166a_Queue = (unsigned char *) malloc(mdm166a_nQueueMax * sizeof(unsigned char));
    if (mdm166a_Queue == NULL) {
	error("%s: command queue could not be allocated: malloc() failed", Name);
	return -1;
    }

    /* Init framebuffer buffer */
    mdm166a_framebuffer = (unsigned char *) malloc(SCREEN_W * nSizeYb * sizeof(unsigned char));
    if (!mdm166a_framebuffer)
	return -1;

    memset(mdm166a_framebuffer, 0, SCREEN_W * nSizeYb);
    if (drv_MDM166A_open() < 0) {
	return -1;
    }

    drv_MDM166A_clear();	/* clear display */

    if (cfg_number(section, "Brightness", 1, 0, 2, &value) > 0) {
	info("Setting brightness to %d", value);
	drv_MDM166A_Brightness(value);
    }

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, SCREEN_W, SCREEN_H);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_MDM166A_clear();
	}
    }

    /* setup a timer that regularly redraws the display from the frame */
    timer_add(drv_MDM166A_Flush, NULL, 250, 0);

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_brightness(RESULT * result, RESULT * arg1)
{
    double brightness;

    brightness = drv_MDM166A_Brightness(R2N(arg1));
    SetResult(&result, R_NUMBER, &brightness);
}

static int drv_MDM166A_icons_set(const int num, const int val)
{

    //debug("%s: num %d set %d)", Name, num, val);
    if (num < 0 || num >= GPOS) {
	info("%s: num %d out of range (GPO1..%d)", Name, num, GPOS);
	return -1;
    }
    drv_MDM166A_icons(num, val);
    return 0;
}

/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_MDM166A_list(void)
{
    printf("MDM166A 96x16 Graphic LCD");
    return 0;
}

/* initialize driver & display */
int drv_MDM166A_init(const char *section, const int quiet)
{
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_MDM166A_blit;
    drv_generic_gpio_real_set = drv_MDM166A_icons_set;

    /* start display */
    if ((ret = drv_MDM166A_start(section, quiet)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    /* initialize generic GPIO driver */
    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

    /* register plugins */
    AddFunction("LCD::brightness", 1, plugin_brightness);

    return 0;
}


/* close driver & display */
int drv_MDM166A_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    /* clear display */
    drv_MDM166A_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_MDM166A_close();
    drv_generic_graphic_quit();

    if (mdm166a_Queue) {
	free(mdm166a_Queue);
	mdm166a_Queue = NULL;
    }

    if (mdm166a_framebuffer) {
	free(mdm166a_framebuffer);
	mdm166a_framebuffer = NULL;
    }

    return (0);
}


DRIVER drv_MDM166A = {
    .name = Name,
    .list = drv_MDM166A_list,
    .init = drv_MDM166A_init,
    .quit = drv_MDM166A_quit,
};
