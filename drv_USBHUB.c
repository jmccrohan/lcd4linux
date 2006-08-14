/* $Id: drv_USBHUB.c,v 1.3 2006/08/14 05:54:04 reinelt Exp $
 *
 * new style driver for USBLCD displays
 *
 * Copyright (C) 2006 Ernst Bachmann <e.bachmann@xebec.de>
 * Copyright (C) 2004,2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Based on the USBLCD driver Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_USBHUB.c,v $
 * Revision 1.3  2006/08/14 05:54:04  reinelt
 * minor warnings fixed, CFLAGS changed (no-strict-aliasing)
 *
 * Revision 1.2  2006/08/13 06:46:51  reinelt
 * T6963 soft-timing & enhancements; indent
 *
 * Revision 1.1  2006/08/08 19:35:22  reinelt
 * USBHUB driver from Ernst Bachmann
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_USBHUB
 *
 */

#include "config.h"

#ifdef HAVE_USB_H
# include <usb.h>
#else
# error The USB-HUB driver only makes sense with USB support
#endif

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "drv.h"
#include "drv_generic_gpio.h"



#define HUB_CONTROL_PORT 0x23
#define HUB_SET_FEATURE 3
#define HUB_SET_INDICATOR 22

static char Name[] = "USBHUB";

/* TODO: Better not specify defaults here, 
 * instead look for the first suitable HUB arround if
 * no Vendor/Product specified in config.
 */

static int hubVendor = 0x0409;
static int hubProduct = 0x0058;

static usb_dev_handle *hub = NULL;

typedef struct _usb_hub_descriptor {
    u_int8_t bLength;
    u_int8_t bDescriptorType;
    u_int8_t nNbrPorts;
    u_int8_t wHubCharacteristicLow;
    u_int8_t wHubCharacteristicHigh;
    u_int8_t bPwrOn2PwrGood;
    u_int8_t bHubContrCurrent;
    u_int8_t deviceRemovable;
    u_int8_t PortPwrCtrlMask[8];
} usb_hub_descriptor;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


static int drv_UH_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;

    hub = NULL;

    info("%s: scanning for an USB HUB (0x%04x:0x%04x)...", Name, hubVendor, hubProduct);

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == hubVendor) && (dev->descriptor.idProduct == hubProduct)) {

		unsigned int v = dev->descriptor.bcdDevice;

		info("%s: found USBHUB V%1d%1d.%1d%1d on bus %s device %s", Name,
		     (v & 0xF000) >> 12, (v & 0xF00) >> 8, (v & 0xF0) >> 4, (v & 0xF), bus->dirname, dev->filename);

		if (dev->descriptor.bDeviceClass != USB_CLASS_HUB) {
		    error("%s: the specified device claims to be no HUB");
		    return -1;
		}

		hub = usb_open(dev);
		if (!hub) {
		    error("%s: usb_open() failed!", Name);
		    return -1;
		}
		return 0;
	    }
	}
    }
    error("%s: could not find a USB HUB", Name);
    return -1;
}


static int drv_UH_close(void)
{
    debug("closing USB handle");

    usb_close(hub);

    return 0;
}


/*
 * Set the Indicator status on port "num+1" to val.
 * according to the USB Specification, the following values would be allowed:
 *
 *   0 : Automatic color (display link state etc)
 *   1 : Amber
 *   2 : Green
 *   3 : Off
 *   4..255: Reserved
 *
 */

static int drv_UH_set(const int num, const int val)
{
    int ret;

    if (!hub)
	return -1;

    if (val < 0 || val > 3) {
	info("%s: value %d out of range (0..3)", Name, val);
	return -1;
    }

    if ((ret = usb_control_msg(hub,
			       HUB_CONTROL_PORT,
			       HUB_SET_FEATURE, HUB_SET_INDICATOR, (val << 8) | (num + 1), NULL, 0, 1000)) != 0) {
	info("%s: usb_control_msg failed with %d", Name, ret);
	return -1;
    }

    return 0;
}


static int drv_UH_start(const char *section, const __attribute__ ((unused)) int quiet)
{
    char *buf;

    usb_hub_descriptor hub_desc;
    int ret;


    buf = cfg_get(section, "Vendor", NULL);
    if (buf) {
	if (!*buf) {
	    error("%s: Strange Vendor Specification");
	    return -1;
	}
	if (sscanf(buf, "0x%x", &hubVendor) != 1) {
	    error("%s: Strange Vendor Specification: [%s]", buf);
	    return -1;
	}
    }

    buf = cfg_get(section, "Product", NULL);
    if (buf) {
	if (!*buf) {
	    error("%s: Strange Product Specification");
	    return -1;
	}
	if (sscanf(buf, "0x%x", &hubProduct) != 1) {
	    error("%s: Strange Product Specification: [%s]", buf);
	    return -1;
	}
    }

    if (drv_UH_open() < 0) {
	return -1;
    }


    if ((ret = usb_control_msg(hub,
			       USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_DEVICE,
			       USB_REQ_GET_DESCRIPTOR, USB_DT_HUB << 8, 0, (char *) &hub_desc, sizeof(hub_desc),
			       1000)) <= 8) {
	error("%s: hub_get_descriptor failed with %d", Name, ret);
	drv_UH_close();
	return -1;
    }
    GPOS = hub_desc.nNbrPorts;
    debug("%s: HUB claims to have %d ports. Configuring them as GPOs", Name, GPOS);
    if (!(hub_desc.wHubCharacteristicLow & 0x80)) {
	error("%s: HUB claims to have no Indicator LEDs (Characteristics 0x%04x). Bailing out.", Name,
	      (hub_desc.wHubCharacteristicHigh << 8) | hub_desc.wHubCharacteristicLow);
	/* The HUB Tells us that there are no LEDs to control. Breaking? Maybe don't trust it and continue anyways? */
	drv_UH_close();
	return -1;

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
int drv_UH_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_UH_init(const char *section, const int quiet)
{
    int ret;
    int i;

    info("%s: %s", Name, "$Revision: 1.3 $");



    /* start display */
    if ((ret = drv_UH_start(section, quiet)) != 0)
	return ret;


    /* real worker functions */
    drv_generic_gpio_real_set = drv_UH_set;


    /* initialize generic GPIO driver */
    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

    /* register gpio widget, done already by generic_gpio */

    /* register plugins */
    /* none at the moment... */

    /* greeting */
    if (!quiet) {
	/* Light all LEDS green for a greeting */
	for (i = 0; i < GPOS; ++i) {
	    drv_UH_set(i, 2);
	}
	sleep(1);
	for (i = 0; i < GPOS; ++i) {
	    drv_UH_set(i, 3);	// OFF
	}
    }


    return 0;
}


/* close driver & display */
int drv_UH_quit(const int quiet)
{
    int i;
    debug("%s: shutting down.", Name);

    /* say goodbye... */
    if (!quiet) {
	/* Light all LEDS amber for a goodbye */
	for (i = 0; i < GPOS; ++i) {
	    drv_UH_set(i, 1);
	}
	sleep(1);

    }

    drv_generic_gpio_quit();

    drv_UH_close();

    info("%s: shutdown complete.", Name);
    return 0;
}


DRIVER drv_USBHUB = {
  name:Name,
  list:drv_UH_list,
  init:drv_UH_init,
  quit:drv_UH_quit,
};
