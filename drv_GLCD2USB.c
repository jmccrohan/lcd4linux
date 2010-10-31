/* $Id$
 * $URL$
 *
 * GLCD2USB driver for LCD4Linux 
 * (see http://www.harbaum.org/till/glcd2usb for hardware)
 *
 * Copyright (C) 2007 Till Harbaum <till@harbaum.org>
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
 * struct DRIVER drv_GLCD2USB
 *
 */

/*
 * Options:
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
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
#include "drv_generic_graphic.h"
#include "drv_generic_keypad.h"

/* Numeric constants for 'reportType' parameters */
#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3

/* These are the error codes which can be returned by functions of this
 * module.
 */
#define USB_ERROR_NONE      0
#define USB_ERROR_ACCESS    1
#define USB_ERROR_NOTFOUND  2
#define USB_ERROR_BUSY      16
#define USB_ERROR_IO        5

/* ------------------------------------------------------------------------ */

#include "glcd2usb.h"

/* ------------------------------------------------------------------------- */

#define USBRQ_HID_GET_REPORT    0x01
#define USBRQ_HID_SET_REPORT    0x09

usb_dev_handle *dev = NULL;

/* USB message buffer */
static union {
    unsigned char bytes[132];
    display_info_t display_info;
} buffer;

/* ------------------------------------------------------------------------- */


#define IDENT_VENDOR_NUM        0x1c40
#define IDENT_VENDOR_STRING     "www.harbaum.org/till/glcd2usb"
#define IDENT_PRODUCT_NUM       0x0525
#define IDENT_PRODUCT_STRING    "GLCD2USB"

/* early versions used the ftdi vendor id */
#define IDENT_VENDOR_NUM_OLD    0x0403
#define IDENT_PRODUCT_NUM_OLD   0xc634

static char Name[] = IDENT_PRODUCT_STRING;

/* ------------------------------------------------------------------------- */

static int usbGetString(usb_dev_handle * dev, int index, char *buf, int buflen)
{
    char buffer[256];
    int rval, i;

    if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
				(USB_DT_STRING << 8) + index, 0x0409, buffer, sizeof(buffer), 1000)) < 0)
	return rval;

    /* not a string */
    if (buffer[1] != USB_DT_STRING)
	return 0;

    /* string would have been longer than buffer is */
    if ((unsigned char) buffer[0] < rval)
	rval = (unsigned char) buffer[0];

    /* 16 bit unicode -> 8 bit ascii */
    rval /= 2;

    /* lossy conversion to ISO Latin1 */
    for (i = 1; i < rval; i++) {
	if (i > buflen)		/* destination buffer overflow */
	    break;

	buf[i - 1] = buffer[2 * i];

	if (buffer[2 * i + 1] != 0)	/* outside of ISO Latin1 range */
	    buf[i - 1] = '?';
    }

    /* terminate string */
    buf[i - 1] = 0;

    return i - 1;
}

/* ------------------------------------------------------------------------- */

int usbOpenDevice(usb_dev_handle ** device, int vendor, char *vendorName, int product, char *productName)
{
    struct usb_bus *bus;
    struct usb_device *dev;
    usb_dev_handle *handle = NULL;
    int errorCode = USB_ERROR_NOTFOUND;
    static int didUsbInit = 0;

    if (!didUsbInit) {
	usb_init();
	didUsbInit = 1;
    }

    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product) {
		char string[256];
		int len;
		handle = usb_open(dev);	/* we need to open the device in order to query strings */
		if (!handle) {
		    errorCode = USB_ERROR_ACCESS;
		    error("%s Warning: cannot open USB device: %s", Name, usb_strerror());
		    continue;
		}
		if (vendorName == NULL && productName == NULL) {	/* name does not matter */
		    break;
		}
		/* now check whether the names match: */
		len = usbGetString(handle, dev->descriptor.iManufacturer, string, sizeof(string));
		if (len < 0) {
		    errorCode = USB_ERROR_IO;
		    error("%s: Cannot query manufacturer for device: %s", Name, usb_strerror());
		} else {
		    errorCode = USB_ERROR_NOTFOUND;
		    if (strcmp(string, vendorName) == 0) {
			len = usbGetString(handle, dev->descriptor.iProduct, string, sizeof(string));
			if (len < 0) {
			    errorCode = USB_ERROR_IO;
			    fprintf(stderr, "Warning: cannot query product for device: %s\n", usb_strerror());
			} else {
			    errorCode = USB_ERROR_NOTFOUND;
			    if (strcmp(string, productName) == 0)
				break;
			}
		    }
		}
		usb_close(handle);
		handle = NULL;
	    }
	}
	if (handle)
	    break;
    }

    if (handle != NULL) {
	int rval, retries = 3;
	if (usb_set_configuration(handle, 1)) {
	    fprintf(stderr, "Warning: could not set configuration: %s\n", usb_strerror());
	}
	/* now try to claim the interface and detach the kernel HID driver on
	 * linux and other operating systems which support the call.
	 */
	while ((rval = usb_claim_interface(handle, 0)) != 0 && retries-- > 0) {
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	    if (usb_detach_kernel_driver_np(handle, 0) < 0) {
		fprintf(stderr, "Warning: could not detach kernel HID driver: %s\n", usb_strerror());
	    }
#endif
	}
#ifndef __APPLE__
	if (rval != 0)
	    fprintf(stderr, "Warning: could not claim interface\n");
#endif
	/* Continue anyway, even if we could not claim the interface. Control transfers
	 * should still work.
	 */
	errorCode = 0;
	*device = handle;
    }
    return errorCode;
}

/* ------------------------------------------------------------------------- */

void usbCloseDevice(usb_dev_handle * device)
{
    if (device != NULL)
	usb_close(device);
}

/* ------------------------------------------------------------------------- */

int usbSetReport(usb_dev_handle * device, int reportType, unsigned char *buffer, int len)
{
    int bytesSent;

    /* the write command needs some tweaking regarding allowed report lengths */
    if (buffer[0] == GLCD2USB_RID_WRITE) {
	int i = 0, allowed_lengths[] = { 4 + 4, 8 + 4, 16 + 4, 32 + 4, 64 + 4, 128 + 4 };

	if (len > 128 + 4)
	    error("%s: %d bytes usb report is too long \n", Name, len);

	while (allowed_lengths[i] != (128 + 4) && allowed_lengths[i] < len)
	    i++;

	len = allowed_lengths[i];
	buffer[0] = GLCD2USB_RID_WRITE + i;
    }

    bytesSent = usb_control_msg(device, USB_TYPE_CLASS | USB_RECIP_INTERFACE |
				USB_ENDPOINT_OUT, USBRQ_HID_SET_REPORT,
				reportType << 8 | buffer[0], 0, (char *) buffer, len, 1000);
    if (bytesSent != len) {
	if (bytesSent < 0)
	    error("%s: Error sending message: %s", Name, usb_strerror());
	return USB_ERROR_IO;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

int usbGetReport(usb_dev_handle * device, int reportType, int reportNumber, unsigned char *buffer, int *len)
{
    *len = usb_control_msg(device, USB_TYPE_CLASS | USB_RECIP_INTERFACE |
			   USB_ENDPOINT_IN, USBRQ_HID_GET_REPORT,
			   reportType << 8 | reportNumber, 0, (char *) buffer, *len, 1000);
    if (*len < 0) {
	error("%s: Error sending message: %s", Name, usb_strerror());
	return USB_ERROR_IO;
    }
    return 0;
}

char *usbErrorMessage(int errCode)
{
    static char buffer[80];

    switch (errCode) {
    case USB_ERROR_ACCESS:
	return "Access to device denied";
    case USB_ERROR_NOTFOUND:
	return "The specified device was not found";
    case USB_ERROR_BUSY:
	return "The device is used by another application";
    case USB_ERROR_IO:
	return "Communication error with device";
    default:
	sprintf(buffer, "Unknown USB error %d", errCode);
	return buffer;
    }
    return NULL;		/* not reached */
}

static char *video_buffer = NULL;
static char *dirty_buffer = NULL;

static void drv_GLCD2USB_blit(const int row, const int col, const int height, const int width)
{
    int r, c, err, i, j;

    /* update offscreen buffer */
    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    int x, y, bit;

	    /* these assignments are display layout dependent */
	    x = c;
	    y = r / 8;
	    bit = r % 8;

	    i = video_buffer[x + DCOLS * y];

	    if (drv_generic_graphic_black(r, c))
		video_buffer[x + DCOLS * y] |= 1 << bit;
	    else
		video_buffer[x + DCOLS * y] &= ~(1 << bit);

	    if (video_buffer[x + DCOLS * y] != i)
		dirty_buffer[x + DCOLS * y] |= 1 << bit;
	}
    }

#if 0
    /* display what's in the buffer (for debugging) */
    for (r = 0; r < DROWS; r++) {
	for (c = 0; c < DCOLS; c++) {
	    if (video_buffer[c + DCOLS * (r / 8)] & (1 << (r % 8)))
		putchar('#');
	    else
		putchar(' ');
	}
	putchar('\n');
    }
#endif

    /* short gaps of unchanged bytes in fact increase the communication */
    /* overhead. so we eliminate them here */
    for (j = -1, i = 0; i < DROWS * DCOLS / 8; i++) {
	if (dirty_buffer[i] && j >= 0 && i - j <= 4) {
	    /* found a clean gap <= 4 bytes: mark it dirty */
	    for (r = j; r < i; r++)
		dirty_buffer[r] = 1;
	}

	/* if this is dirty, drop the saved position */
	if (dirty_buffer[i])
	    j = -1;

	/* save position of this clean entry if there's no position saved yet */
	if (!dirty_buffer[i] && j < 0)
	    j = i;
    }

    /* and do the actual data transmission */
    buffer.bytes[0] = 0;
    for (i = 0; i < DROWS * DCOLS / 8; i++) {
	if (dirty_buffer[i]) {
	    /* starting a new run? */
	    if (!buffer.bytes[0]) {
		buffer.bytes[0] = GLCD2USB_RID_WRITE;
		buffer.bytes[1] = i % 256;	// offset
		buffer.bytes[2] = i / 256;
		buffer.bytes[3] = 0;	// length
	    }
	    buffer.bytes[4 + buffer.bytes[3]++] = video_buffer[i];
	}

	/* this part of the buffer is not dirty or we are at end */
	/* of buffer or the buffer is fill: send data then */
	if ((!dirty_buffer[i]) || (i == DROWS * DCOLS / 8 - 1) || (buffer.bytes[3] == 128)) {
	    /* is there data to be sent in the buffer? */
	    if (buffer.bytes[0] && buffer.bytes[3]) {
		if ((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, buffer.bytes[3] + 4)) != 0)
		    error("%s: Error sending display contents: %s", Name, usbErrorMessage(err));

		buffer.bytes[0] = 0;
	    }
	}

	/* this entry isn't dirty anymore */
	dirty_buffer[i] = 0;
    }
}

static void drv_GLCD2USB_timer(void __attribute__ ((unused)) * notused)
{
    /* request button state */
    static unsigned int last_but = 0;
    int err = 0, len = 2;

    if ((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, GLCD2USB_RID_GET_BUTTONS, buffer.bytes, &len)) != 0) {
	fprintf(stderr, "Error getting button state: %s\n", usbErrorMessage(err));
	return;
    }

    int i;

    /* check if button state changed */
    if (buffer.bytes[1] ^ last_but) {

	/* send single keypad events for all changed buttons */
	for (i = 0; i < 4; i++)
	    if ((buffer.bytes[1] & (1 << i)) ^ (last_but & (1 << i)))
		drv_generic_keypad_press(((buffer.bytes[1] & (1 << i)) ? 0x80 : 0) | i);
    }

    last_but = buffer.bytes[1];
}

static int drv_GLCD2USB_start(const char *section)
{
    char *s;
    int err = 0, len;

    if (sscanf(s = cfg_get(section, "font", "6x8"), "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad %s.Font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if ((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING,
			     IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING)) != 0) {
	if ((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM_OLD, IDENT_VENDOR_STRING,
				 IDENT_PRODUCT_NUM_OLD, IDENT_PRODUCT_STRING)) != 0) {
	    error("%s: opening GLCD2USB device: %s", Name, usbErrorMessage(err));
	    return -1;
	}
    }

    info("%s: Found device", Name);

    /* query display parameters */
    memset(&buffer, 0, sizeof(buffer));

    len = sizeof(display_info_t);
    if ((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, GLCD2USB_RID_GET_INFO, buffer.bytes, &len)) != 0) {

	error("%s: query display parameters: %s", Name, usbErrorMessage(err));
	usbCloseDevice(dev);
	return -1;
    }

    if (len < (int) sizeof(buffer.display_info)) {
	error("%s: Not enough bytes in display info report (%d instead of %d)",
	      Name, len, (int) sizeof(buffer.display_info));
	usbCloseDevice(dev);
	return -1;
    }

    info("%s: display name = %s", Name, buffer.display_info.name);
    info("%s: display resolution = %d * %d", Name, buffer.display_info.width, buffer.display_info.height);
    info("%s: display flags: %x", Name, buffer.display_info.flags);

    /* TODO: check for supported features */


    /* save display size */
    DCOLS = buffer.display_info.width;
    DROWS = buffer.display_info.height;

    /* allocate a offscreen buffer */
    video_buffer = malloc(DCOLS * DROWS / 8);
    dirty_buffer = malloc(DCOLS * DROWS / 8);
    memset(video_buffer, 0, DCOLS * DROWS / 8);
    memset(dirty_buffer, 0, DCOLS * DROWS / 8);

    /* get access to display */
    buffer.bytes[0] = GLCD2USB_RID_SET_ALLOC;
    buffer.bytes[1] = 1;	/* 1=alloc, 0=free */
    if ((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, 2)) != 0) {
	error("%s: Error allocating display: %s", Name, usbErrorMessage(err));
	usbCloseDevice(dev);
	return -1;
    }

    /* regularly request key state. can be quite slow since the device */
    /* buffers button presses internally */
    timer_add(drv_GLCD2USB_timer, NULL, 100, 0);

    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_graphic_draw(W) */
/* using drv_generic_graphic_icon_draw(W) */
/* using drv_generic_graphic_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_GLCD2USB_list(void)
{
    printf("GLCD2USB homebrew USB interface for graphic displays");
    return 0;
}

static int drv_GLCD2USB_keypad(const int num)
{
    const int keys[] = { WIDGET_KEY_LEFT, WIDGET_KEY_RIGHT,
	WIDGET_KEY_CONFIRM, WIDGET_KEY_CANCEL
    };

    int val;

    /* check for key press event */
    if (num & 0x80)
	val = WIDGET_KEY_PRESSED;
    else
	val = WIDGET_KEY_RELEASED;

    return val | keys[num & 0x03];
}

/* initialize driver & display */
int drv_GLCD2USB_init(const char *section, const __attribute__ ((unused))
		      int quiet)
{
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_GLCD2USB_blit;
    drv_generic_keypad_real_press = drv_GLCD2USB_keypad;

    /* start display */
    if ((ret = drv_GLCD2USB_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    /* register plugins */
    /* none at the moment... */


    return 0;
}


/* close driver & display */
int drv_GLCD2USB_quit(const __attribute__ ((unused))
		      int quiet)
{
    int err;

    info("%s: shutting down.", Name);
    drv_generic_graphic_quit();

    /* release access to display */

    buffer.bytes[0] = GLCD2USB_RID_SET_ALLOC;
    buffer.bytes[1] = 0;	/* 1=alloc, 0=free */
    if ((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, 2)) != 0) {
	error("%s Error freeing display: %s", Name, usbErrorMessage(err));
    }

    /* clean up */
    if (dev != NULL)
	usbCloseDevice(dev);

    if (video_buffer != NULL) {
	free(video_buffer);
	free(dirty_buffer);
    }

    return (0);
}


DRIVER drv_GLCD2USB = {
    .name = Name,
    .list = drv_GLCD2USB_list,
    .init = drv_GLCD2USB_init,
    .quit = drv_GLCD2USB_quit,
};
