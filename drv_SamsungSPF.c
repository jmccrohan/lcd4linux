/* $Id: drv_SamsungSPF 975 2009-01-18 11:16:20Z michael $
   * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_SamsungSPF.c $
   *
   * SamsungSPF lcd4linux driver
   *
   * Copyright (C) 2012 Sascha Plazar <sascha@plazar.de>
   * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
   *
   * This driver is based on playusb.c created on Aug 2, 2010 by Andre Puschmann
   * which is in turn based on code from Grace Woo: 
   *    http://web.media.mit.edu/~gracewoo/stuff/picframe
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
   * struct DRIVER drv_SamsungSPF
   *
   */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <usb.h>
#include <jpeglib.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "timer.h"
#include "drv.h"

/* graphic display? */
#include "drv_generic_graphic.h"

// Drivername for verbose output
static char Name[] = "SamsungSPF";

struct SPFdev {
    const char type[64];
    const int vendorID;
    struct {
	const int storageMode;
	const int monitorMode;
    } productID;
    const unsigned int xRes;
    const unsigned int yRes;
};

static struct SPFdev spfDevices[] = {
    {
     .type = "SPF-75H",
     .vendorID = 0x04e8,
     .productID = {0x200e, 0x200f},
     .xRes = 800,
     .yRes = 480,
     },
    {
     .type = "SPF-85H",
     .vendorID = 0x04e8,
     .productID = {0x2012, 0x2013},
     .xRes = 800,
     .yRes = 600,
     },
    {
     .type = "SPF-107H",
     .vendorID = 0x04e8,
     .productID = {0x2027, 0x2028},
     .xRes = 1024,
     .yRes = 600,
     },
};

static int numFrames = sizeof(spfDevices) / sizeof(spfDevices[0]);

struct usb_device *myDev;
usb_dev_handle *myDevHandle;
struct SPFdev *myFrame;

typedef struct {
    unsigned char R, G, B;
} RGB;

static struct {
    RGB *buf;
    int dirty;
    int fbsize;
} image;

static struct {
    unsigned char *buf;
    unsigned long int size;
} jpegImage;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


/* please note that in-memory compression doesn't work satisfactory */
int convert2JPG()
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    int row_stride;		/* physical row width in buffer */
    JSAMPROW row_pointer[1];	/* pointer to a single row */

    /* Initialize compression frame */
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &jpegImage.buf, &jpegImage.size);

    cinfo.image_width = myFrame->xRes;
    cinfo.image_height = myFrame->yRes;
    cinfo.input_components = sizeof(RGB);
    cinfo.in_color_space = JCS_RGB;

    /* call some jpeg helpers */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 100, 1);	/*set the quality [0..100]  */
    jpeg_start_compress(&cinfo, 1);

    row_stride = cinfo.image_width;

    /* Convert line by line */
    while (cinfo.next_scanline < cinfo.image_height) {
	row_pointer[0] = (JSAMPROW) (image.buf + (cinfo.next_scanline * row_stride));
	jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Finish compression and free internal memory */
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return 0;
}


// Find specific Samsung device
static void drv_SamsungSPF_find()
{
    info("%s: Searching SPF.", Name);

    /* open USB device */
    struct usb_bus *bus;
    struct usb_device *dev;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    for (bus = usb_busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if (dev->descriptor.idVendor == myFrame->vendorID) {
		if (dev->descriptor.idProduct == myFrame->productID.storageMode) {

		    info("Samsung photoframe in Mass Storage mode found.");
		    myDev = dev;

		    return;
		} else if (dev->descriptor.idProduct == myFrame->productID.monitorMode) {

		    info("Samsung photoframe in Custom Product mode found.");
		    myDev = dev;

		    return;
		}
	    }
	}
    }

    free(bus);
    myDev = 0;
}


static int drv_SamsungSPF_open()
{
    if (!myDev) {
	error("%s: No device specified!", Name);
	return -1;
    }

    int res = -1;
    char buf[256];

    if (myDev->descriptor.idProduct == myFrame->productID.storageMode) {
	info("%s: Opening device and switching to monitor mode", Name);

	myDevHandle = usb_open(myDev);

	setuid(getuid());

	strcpy(buf, "** no string **");
	res = usb_get_string_simple(myDevHandle, myDev->descriptor.iManufacturer, buf, sizeof(buf));
	debug("usb_get_string_simple => %d, %s", res, buf);

	memset(buf, 0, 256);

	res = usb_control_msg(myDevHandle, USB_TYPE_STANDARD | USB_ENDPOINT_IN,
			      USB_REQ_GET_DESCRIPTOR, 0xfe, 0xfe, buf, 0xfe, 1000);
	/* usb_close( myDev ); */
	// Sleep some time before research
	sleep(1);
	drv_SamsungSPF_find();
    } else
	info("%s: No device in storage mode found", Name);

    if (myDev->descriptor.idProduct == myFrame->productID.storageMode) {
	error("%s: Was not able to switch to monitor mode!", Name);
	return -1;
    }

    if (myDev->descriptor.idProduct == myFrame->productID.monitorMode) {
	info("%s: Device '%s' is now in monitor mode.", Name, myFrame->type);
	myDevHandle = usb_open(myDev);
	return 0;
    }

    error("Unknown error: usb_control_msg() = %d", res);
    return -1;
}


/* dummy function that sends something to the display */
static int drv_SamsungSPF_send(char *data, unsigned int len)
{
    char usb_hdr[12] = { 0xa5, 0x5a, 0x18, 0x04, 0xff, 0xff,
	0xff, 0xff, 0x48, 0x00, 0x00, 0x00
    };
    char buffer[1] = { 0x0 };
    int usb_timeout = 1000;
    int usb_endpoint = 0x2;
    int ret;

    *(int *) (usb_hdr + 4) = len;

    debug("bytes_to_send: %d, offset: %d", len, 12);

    /* Send USB header */
    if ((ret = usb_bulk_write(myDevHandle, usb_endpoint, usb_hdr, 12, usb_timeout)) < 0) {
	error("%s: Error occurred while writing data to device.", Name);
	error("%s: usb_bulk_write returned: %d", Name, ret);
	return -1;
    }

    /* Send JPEG image */
    if ((ret = usb_bulk_write(myDevHandle, usb_endpoint, data, len, usb_timeout)) < 0) {
	error("%s: Error occurred while writing data to device.", Name);
	error("%s: usb_bulk_write returned: %d", Name, ret);
	return -1;
    }

    /* Finish transmission by sending zero */
    if ((ret = usb_bulk_write(myDevHandle, usb_endpoint, buffer, 1, usb_timeout)) < 0) {
	error("%s: Error occurred while writing data to device.", Name);
	error("%s: usb_bulk_write returned: %d", Name, ret);
	return -1;
    }

    return 0;
}


/* for graphic displays only */
static void drv_SamsungSPF_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    RGB p1 = image.buf[r * myFrame->xRes + c];
	    RGBA p2 = drv_generic_graphic_rgb(r, c);
	    if (p1.R != p2.R || p1.G != p2.G || p1.B != p2.B) {
		image.buf[r * myFrame->xRes + c].R = p2.R;
		image.buf[r * myFrame->xRes + c].G = p2.G;
		image.buf[r * myFrame->xRes + c].B = p2.B;
		image.dirty = 1;
	    }
	}
    }
}


static void drv_SamsungSPF_timer( __attribute__ ((unused))
				 void *notused)
{
    if (image.dirty) {
	debug("FB dirty, writing jpeg...");
	convert2JPG();

	/* Sent image to display */
	if ((drv_SamsungSPF_send((char *) jpegImage.buf, jpegImage.size)) != 0) {
	    error("%s: Error occurred while sending jpeg image to device.", Name);
	}
	/* Clean dirty bit */
	image.dirty = 0;

	/* Free JPEG buffer since a new is allocated each time an image is 
	   compressed */
	if (jpegImage.size)
	    free(jpegImage.buf);
	jpegImage.size = 0;
    }
}


/* start graphic display */
static int drv_SamsungSPF_start(const char *section)
{
    int timerInterval = 1000;
    char *s;

    cfg_number(section, "update", timerInterval, 0, -1, &timerInterval);
    debug("Updating display every %dms", timerInterval);

    DROWS = myFrame->yRes;
    DCOLS = myFrame->xRes;
    info("%s: Using SPF with %dx%d pixels.", Name, DCOLS, DROWS);

    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    if (XRES < 6 || YRES < 8) {
	error("%s: bad Font '%s' from %s (must be at least 6x8)", Name, s, cfg_source());
	return -1;
    }
    free(s);

    /* Allocate framebuffer */
    image.fbsize = myFrame->xRes * myFrame->yRes * sizeof(RGB);
    image.buf = malloc(image.fbsize);
    memset(image.buf, 128, image.fbsize);
    image.dirty = 0;

    /* JPEG buffer is allocated by jpeglib */
    jpegImage.buf = 0;
    jpegImage.size = 0;

    /* regularly transmit the image */
    timer_add(drv_SamsungSPF_timer, NULL, timerInterval, 0);

    return 0;
}



/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_SamsungSPF_list(void)
{
    int i;

    printf("SamsungSPF driver, supported models [");
    for (i = 0; i < numFrames; i++) {
	printf(spfDevices[i].type);
	if (i < numFrames - 1)
	    printf(", ");
    }
    printf("]\n");

    return 0;
}


/* initialize driver & display */
/* use this function for a graphic display */
int drv_SamsungSPF_init(const char *section, const int quiet)
{
    info("%s: Initializing SPF.", Name);

    char *s;
    int i;

    myDev = 0;
    myFrame = 0;

    // Look for model entry in config
    s = cfg_get(section, "Model", NULL);
    if (s == NULL || *s != '\0') {
	s = cfg_get(section, "Model", NULL);
	if (s == NULL || *s == '\0') {

	    drv_SamsungSPF_list();
	    error("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
	    return -1;
	}
    }
    // Look for specified device
    for (i = 0; i < numFrames; i++) {
	if (strcasecmp(s, spfDevices[i].type) == 0) {
	    myFrame = &spfDevices[i];
	    info("%s: Configured for model %s.", Name, spfDevices[i].type);
	    break;
	}
    }

    if (!myFrame) {
	drv_SamsungSPF_list();
	error("%s: unknown model '%s'!", Name, s);
	return -1;
    }

    free(s);

    /* try to open USB device */
    drv_SamsungSPF_find();
    if (!myDev) {
	error("%s: No Samsung '%s' found!", Name, myFrame->type);
	return -1;
    }

    /* open display and switch to monitor mode if necessary */
    if (drv_SamsungSPF_open() == -1)
	return -1;

    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_SamsungSPF_blit;

    /* start display */
    if ((ret = drv_SamsungSPF_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    return 0;
}


/* close driver & display */
/* use this function for a graphic display */
int drv_SamsungSPF_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    debug("closing connection");
    printf("%s: Closing driver...\n", Name);
    usb_close(myDevHandle);
    free(myDev);
    free(myDevHandle);

    return (0);
}


/* use this one for a graphic display */
DRIVER drv_SamsungSPF = {
    .name = Name,
    .list = drv_SamsungSPF_list,
    .init = drv_SamsungSPF_init,
    .quit = drv_SamsungSPF_quit,
};
