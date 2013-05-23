/* $Id: drv_dpf.c 980 2009-01-28 21:18:52Z michux $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_dpf.c $
 *
 * Driver for hacked digital picture frames. Uses *NO* external libraries.
 * See http://dpf-ax.sourceforge.net/ for more information.
 *
 * Copyright (C) 2008 Jeroen Domburg <picframe@spritesmods.com>
 * Modified from sample code by:
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * 
 * Mods by <hackfin@section5.ch>
 * Complete rewrite 05/2013 by superelchi <superelchi AT wolke7.net>
 *
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
 * struct DRIVER drv_DPF
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

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

#include "drv_generic_graphic.h"

//###################################################################
// Start dpfcore4driver.h
// See http://dpf-ax.sourceforge.net/
//###################################################################

#define DPFAXHANDLE void *	// Handle needed for dpf_ax access
#define DPF_BPP 2		//bpp for dfp-ax is currently always 2!

/**
 * Open DPF device.
 * 
 * Device must be string in the form "usbX" or "dpfX", with X = 0 .. number of connected dpfs.
 * The open function will scan the USB bus and return a handle to access dpf #X.
 * If dpf #X is not found, returns NULL.
 *
 * \param dev	device name to open
 * \return		device handle or NULL
 */
DPFAXHANDLE dpf_ax_open(const char *device);

/**
 *  Close DPF device.
 */
void dpf_ax_close(DPFAXHANDLE h);

/** Blit data to screen.
 *
 * \param buf     buffer to 16 bpp RGB 565 image data
 * \param rect    rectangle tuple: [x0, y0, x1, y1]
 */
void dpf_ax_screen_blit(DPFAXHANDLE h, const unsigned char *buf, short rect[4]);

/** Set backlight brightness.
 * 
 * \param value       Backlight value 0..7 (0 = off, 7 = max brightness)
 */
void dpf_ax_setbacklight(DPFAXHANDLE h, int value);

/** Get screen width.
 * 
 * \return width in pixel
 */
int dpf_ax_getwidth(DPFAXHANDLE h);

/** Get screen height.
 * 
 * \return height in pixel
 */
int dpf_ax_getheight(DPFAXHANDLE h);

//###################################################################
// End dpfcore4driver.h
//###################################################################


static char Name[] = "DPF";


/*
 * Dpf status
 */
static struct {
    unsigned char *lcdBuf;	// Display data buffer
    unsigned char *xferBuf;	// USB transfer buffer
    DPFAXHANDLE dpfh;		// Handle for dpf access
    int pwidth;			// Physical display width
    int pheight;		// Physical display height

    // Flags to translate logical to physical orientation
    int isPortrait;
    int rotate90;
    int flip;

    // Current dirty rectangle
    int minx, maxx;
    int miny, maxy;

    // Config properties
    int orientation;
    int backlight;
} dpf;


// Convert RGBA pixel to RGB565 pixel(s)

#define _RGB565_0(p) (( ((p.R) & 0xf8)      ) | (((p.G) & 0xe0) >> 5))
#define _RGB565_1(p) (( ((p.G) & 0x1c) << 3 ) | (((p.B) & 0xf8) >> 3))

/*
 * Set one pixel in lcdBuf.
 * 
 * Respects orientation and updates dirty rectangle.
 *
 * in:	x, y - pixel coordinates
 * 	pix  - RGBA pixel value
 * out:	-
 */
static void drv_set_pixel(int x, int y, RGBA pix)
{
    int changed = 0;

    int sx = DCOLS;
    int sy = DROWS;
    int lx = x % sx;
    int ly = y % sy;

    if (dpf.flip) {
	// upside down orientation
	lx = DCOLS - 1 - lx;
	ly = DROWS - 1 - ly;
    }

    if (dpf.rotate90) {
	// wrong Orientation, rotate
	int i = ly;
	ly = dpf.pheight - 1 - lx;
	lx = i;
    }

    if (lx < 0 || lx >= (int) dpf.pwidth || ly < 0 || ly >= (int) dpf.pheight) {
	error("dpf: x/y out of bounds (x=%d, y=%d, rot=%d, flip=%d, lx=%d, ly=%d)\n", x, y, dpf.rotate90, dpf.flip, lx,
	      ly);
	return;
    }

    unsigned char c1 = _RGB565_0(pix);
    unsigned char c2 = _RGB565_1(pix);
    unsigned int i = (ly * dpf.pwidth + lx) * DPF_BPP;
    if (dpf.lcdBuf[i] != c1 || dpf.lcdBuf[i + 1] != c2) {
	dpf.lcdBuf[i] = c1;
	dpf.lcdBuf[i + 1] = c2;
	changed = 1;
    }

    if (changed) {
	if (lx < dpf.minx)
	    dpf.minx = lx;
	if (lx > dpf.maxx)
	    dpf.maxx = lx;
	if (ly < dpf.miny)
	    dpf.miny = ly;
	if (ly > dpf.maxy)
	    dpf.maxy = ly;
    }
}

/*
 * Send pixel data to dpf
 */
static void drv_dpf_blit(const int row, const int col, const int height, const int width)
{
    int x, y;

    // Set pixels one by one
    // Note: here is room for optimization :-)
    for (y = row; y < row + height; y++)
	for (x = col; x < col + width; x++)
	    drv_set_pixel(x, y, drv_generic_graphic_rgb(y, x));

    // If nothing has changed, skip transfer
    if (dpf.minx > dpf.maxx || dpf.miny > dpf.maxy)
	return;

    // Copy data in dirty rectangle from data buffer to temp transfer buffer
    unsigned int cpylength = (dpf.maxx - dpf.minx + 1) * DPF_BPP;
    unsigned char *ps = dpf.lcdBuf + (dpf.miny * dpf.pwidth + dpf.minx) * DPF_BPP;
    unsigned char *pd = dpf.xferBuf;
    for (y = dpf.miny; y <= dpf.maxy; y++) {
	memcpy(pd, ps, cpylength);
	ps += dpf.pwidth * DPF_BPP;
	pd += cpylength;
    }

    // Send the buffer
    short rect[4];
    rect[0] = dpf.minx;
    rect[1] = dpf.miny;
    rect[2] = dpf.maxx + 1;
    rect[3] = dpf.maxy + 1;
    dpf_ax_screen_blit(dpf.dpfh, dpf.xferBuf, rect);

    // Reset dirty rectangle
    dpf.minx = dpf.pwidth - 1;
    dpf.maxx = 0;
    dpf.miny = dpf.pheight - 1;
    dpf.maxy = 0;
}


/* start graphic display */
static int drv_dpf_start(const char *section)
{
    int i;
    char *dev;
    char *s;

    // Check if config is valid

    // Get the device
    dev = cfg_get(section, "Port", NULL);
    if (dev == NULL || *dev == '\0') {
	error("dpf: no '%s.Port' entry from %s", section, cfg_source());
	return -1;
    }
    // Get font
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

    /* we dont want fonts below 6 width */
    if (XRES < 6) {
	error("%s: bad Font '%s' width '%d' using minimum of 6)", Name, s, XRES);
	XRES = 6;
    }

    /* we dont want fonts below 8 height */
    if (YRES < 8) {
	error("%s: bad Font '%s' height '%d' using minimum of 8)", Name, s, YRES);
	YRES = 8;
    }
    // Get the logical orientation (0 = landscape, 1 = portrait, 2 = reverse landscape, 3 = reverse portrait)
    if (cfg_number(section, "Orientation", 0, 0, 3, &i) > 0)
	dpf.orientation = i;
    else
	dpf.orientation = 0;

    // Get the backlight value (0 = off, 7 = max brightness)
    if (cfg_number(section, "Backlight", 0, 0, 7, &i) > 0)
	dpf.backlight = i;
    else
	dpf.backlight = 7;

    /* open communication with the display */
    dpf.dpfh = dpf_ax_open(dev);
    if (dpf.dpfh == NULL) {
	error("dpf: cannot open dpf device %s", dev);
	return -1;
    }
    // Get dpfs physical dimensions
    dpf.pwidth = dpf_ax_getwidth(dpf.dpfh);
    dpf.pheight = dpf_ax_getheight(dpf.dpfh);


    // See, if we have to rotate the display
    dpf.isPortrait = dpf.pwidth < dpf.pheight;
    if (dpf.isPortrait) {
	if (dpf.orientation == 0 || dpf.orientation == 2)
	    dpf.rotate90 = 1;
    } else if (dpf.orientation == 1 || dpf.orientation == 3)
	dpf.rotate90 = 1;
    dpf.flip = (!dpf.isPortrait && dpf.rotate90);	// adjust to make rotate por/land = physical por/land
    if (dpf.orientation > 1)
	dpf.flip = !dpf.flip;

    // allocate display buffer + temp transfer buffer
    dpf.lcdBuf = malloc(dpf.pwidth * dpf.pheight * DPF_BPP);
    dpf.xferBuf = malloc(dpf.pwidth * dpf.pheight * DPF_BPP);

    // clear display buffer + set it to "dirty"
    memset(dpf.lcdBuf, 0, dpf.pwidth * dpf.pheight * DPF_BPP);	//Black
    dpf.minx = 0;
    dpf.maxx = dpf.pwidth - 1;
    dpf.miny = 0;
    dpf.maxy = dpf.pheight - 1;

    // set the logical width/height for lcd4linux
    DCOLS = ((!dpf.rotate90) ? dpf.pwidth : dpf.pheight);
    DROWS = ((!dpf.rotate90) ? dpf.pheight : dpf.pwidth);

    // Set backlight (brightness)
    dpf_ax_setbacklight(dpf.dpfh, dpf.backlight);

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    int b = R2N(arg1);
    if (b < 0)
	b = 0;
    if (b > 7)
	b = 7;

    dpf_ax_setbacklight(dpf.dpfh, b);
    SetResult(&result, R_NUMBER, &b);
}


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_dpf_list(void)
{
    printf("Hacked dpf-ax digital photo frame");
    return 0;
}


/* initialize driver & display */
int drv_dpf_init(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_dpf_blit;

    /* start display */
    if ((ret = drv_dpf_start(section)) != 0)
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

    /* register plugins */
    AddFunction("LCD::backlight", 1, plugin_backlight);

    return 0;
}


/* close driver & display */
int drv_dpf_quit(const int quiet)
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
    dpf_ax_close(dpf.dpfh);

    return (0);
}


DRIVER drv_DPF = {
    .name = Name,
    .list = drv_dpf_list,
    .init = drv_dpf_init,
    .quit = drv_dpf_quit,
};

//###################################################################
// Start dpfcore4driver.c
// See http://dpf-ax.sourceforge.net/
//###################################################################

#include <usb.h>

#define AX206_VID 0x1908	// Hacked frames USB Vendor ID
#define AX206_PID 0x0102	// Hacked frames USB Product ID

#define USBCMD_SETPROPERTY  0x01	// USB command: Set property
#define USBCMD_BLIT         0x12	// USB command: Blit to screen

/* Generic SCSI device stuff */

#define DIR_IN  0
#define DIR_OUT 1

/* The DPF context structure */
typedef
    struct dpf_context {
    usb_dev_handle *udev;
    unsigned int width;
    unsigned int height;
} DPFContext;

static int wrap_scsi(DPFContext * h, unsigned char *cmd, int cmdlen, char out,
		     unsigned char *data, unsigned long block_len);

/**
 * Open DPF device.
 * 
 * Device must be string in the form "usbX" or "dpfX", with X = 0 .. number of connected dpfs.
 * The open function will scan the USB bus and return a handle to access dpf #X.
 * If dpf #X is not found, returns NULL.
 *
 * \param dev	device name to open
 * \return		device handle or NULL
 */
DPFAXHANDLE dpf_ax_open(const char *dev)
{
    DPFContext *dpf;
    int index = -1;
    usb_dev_handle *u;

    if (dev && strlen(dev) == 4 && (strncmp(dev, "usb", 3) == 0 || strncmp(dev, "dpf", 3) == 0))
	index = dev[3] - '0';

    if (index < 0 || index > 9) {
	fprintf(stderr, "dpf_ax_open: wrong device '%s'. Please specify a string like 'usb0'\n", dev);
	return NULL;
    }

    usb_init();
    usb_find_busses();
    usb_find_devices();

    struct usb_bus *b = usb_get_busses();
    struct usb_device *d = NULL;
    int enumeration = 0;
    int found = 0;

    while (b && !found) {
	d = b->devices;
	while (d) {
	    if ((d->descriptor.idVendor == AX206_VID) && (d->descriptor.idProduct == AX206_PID)) {
		fprintf(stderr, "dpf_ax_open: found AX206 #%d\n", enumeration + 1);
		if (enumeration == index) {
		    found = 1;
		    break;
		} else
		    enumeration++;
	    }
	    d = d->next;
	}
	b = b->next;
    }

    if (!d) {
	fprintf(stderr, "dpf_ax_open: no matching USB device '%s' found!\n", dev);
	return NULL;
    }

    dpf = (DPFContext *) malloc(sizeof(DPFContext));
    if (!dpf) {
	fprintf(stderr, "dpf_ax_open: error allocation memory.\n");
	return NULL;
    }

    u = usb_open(d);
    if (u == NULL) {
	fprintf(stderr, "dpf_ax_open: failed to open usb device '%s'!\n", dev);
	free(dpf);
	return NULL;
    }

    if (usb_claim_interface(u, 0) < 0) {
	fprintf(stderr, "dpf_ax_open: failed to claim usb device!\n");
	usb_close(u);
	free(dpf);
	return NULL;
    }

    dpf->udev = u;

    static unsigned char buf[5];
    static unsigned char cmd[16] = {
	0xcd, 0, 0, 0,
	0, 2, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
    };
    cmd[5] = 2;			// get LCD parameters
    if (wrap_scsi(dpf, cmd, sizeof(cmd), DIR_IN, buf, 5) == 0) {
	dpf->width = (buf[0]) | (buf[1] << 8);
	dpf->height = (buf[2]) | (buf[3] << 8);
	fprintf(stderr, "dpf_ax_open: got LCD dimensions: %dx%d\n", dpf->width, dpf->height);
    } else {
	fprintf(stderr, "dpf_ax_open: error reading LCD dimensions!\n");
	dpf_ax_close(dpf);
	return NULL;
    }
    return (DPFAXHANDLE) dpf;
}

/**
 *  Close DPF device
 */

void dpf_ax_close(DPFAXHANDLE h)
{
    DPFContext *dpf = (DPFContext *) h;

    usb_release_interface(dpf->udev, 0);
    usb_close(dpf->udev);
    free(dpf);
}

/** Get screen width.
 * 
 * \return width in pixel
 */
int dpf_ax_getwidth(DPFAXHANDLE h)
{
    return ((DPFContext *) h)->width;
}

/** Get screen height.
 * 
 * \return height in pixel
 */
int dpf_ax_getheight(DPFAXHANDLE h)
{
    return ((DPFContext *) h)->height;
}

static
unsigned char g_excmd[16] = {
    0xcd, 0, 0, 0,
    0, 6, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

/** Blit data to screen.
 *
 * \param buf     buffer to 16 bpp RGB 565 image data
 * \param rect    rectangle tuple: [x0, y0, x1, y1]
 */
void dpf_ax_screen_blit(DPFAXHANDLE h, const unsigned char *buf, short rect[4])
{
    unsigned long len = (rect[2] - rect[0]) * (rect[3] - rect[1]);
    len <<= 1;
    unsigned char *cmd = g_excmd;

    cmd[6] = USBCMD_BLIT;
    cmd[7] = rect[0];
    cmd[8] = rect[0] >> 8;
    cmd[9] = rect[1];
    cmd[10] = rect[1] >> 8;
    cmd[11] = rect[2] - 1;
    cmd[12] = (rect[2] - 1) >> 8;
    cmd[13] = rect[3] - 1;
    cmd[14] = (rect[3] - 1) >> 8;
    cmd[15] = 0;

    wrap_scsi((DPFContext *) h, cmd, sizeof(g_excmd), DIR_OUT, (unsigned char *) buf, len);
}

/** Set backlight
 * 
 * \param value       Backlight value 0..7 (0 = off, 7 = max brightness)
 */
void dpf_ax_setbacklight(DPFAXHANDLE h, int b)
{
    unsigned char *cmd = g_excmd;

    if (b < 0)
	b = 0;
    if (b > 7)
	b = 7;

    cmd[6] = USBCMD_SETPROPERTY;
    cmd[7] = 0x01;		// PROPERTY_BRIGHTNESS
    cmd[8] = 0x00;		//PROPERTY_BRIGHTNESS >> 8;
    cmd[9] = b;
    cmd[10] = b >> 8;

    wrap_scsi((DPFContext *) h, cmd, sizeof(g_excmd), DIR_OUT, NULL, 0);
}


static unsigned char g_buf[] = {
    0x55, 0x53, 0x42, 0x43,	// dCBWSignature
    0xde, 0xad, 0xbe, 0xef,	// dCBWTag
    0x00, 0x80, 0x00, 0x00,	// dCBWLength
    0x00,			// bmCBWFlags: 0x80: data in (dev to host), 0x00: Data out
    0x00,			// bCBWLUN
    0x10,			// bCBWCBLength

    // SCSI cmd:
    0xcd, 0x00, 0x00, 0x00,
    0x00, 0x06, 0x11, 0xf8,
    0x70, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

#define ENDPT_OUT 1
#define ENDPT_IN 0x81

static int wrap_scsi(DPFContext * h, unsigned char *cmd, int cmdlen, char out,
		     unsigned char *data, unsigned long block_len)
{
    int len;
    int ret;
    static unsigned char ansbuf[13];	// Do not change size.

    g_buf[14] = cmdlen;
    memcpy(&g_buf[15], cmd, cmdlen);

    g_buf[8] = block_len;
    g_buf[9] = block_len >> 8;
    g_buf[10] = block_len >> 16;
    g_buf[11] = block_len >> 24;

    ret = usb_bulk_write(h->udev, ENDPT_OUT, (const char *) g_buf, sizeof(g_buf), 1000);
    if (ret < 0)
	return ret;

    if (out == DIR_OUT) {
	if (data) {
	    ret = usb_bulk_write(h->udev, ENDPT_OUT, (const char *) data, block_len, 3000);
	    if (ret != (int) block_len) {
		fprintf(stderr, "dpf_ax ERROR: bulk write.\n");
		return ret;
	    }
	}
    } else if (data) {
	ret = usb_bulk_read(h->udev, ENDPT_IN, (char *) data, block_len, 4000);
	if (ret != (int) block_len) {
	    fprintf(stderr, "dpf_ax ERROR: bulk read.\n");
	    return ret;
	}
    }
    // get ACK:
    len = sizeof(ansbuf);
    int retry = 0;
    int timeout = 0;
    do {
	timeout = 0;
	ret = usb_bulk_read(h->udev, ENDPT_IN, (char *) ansbuf, len, 5000);
	if (ret != len) {
	    fprintf(stderr, "dpf_ax ERROR: bulk ACK read.\n");
	    timeout = 1;
	}
	retry++;
    } while (timeout && retry < 5);
    if (strncmp((char *) ansbuf, "USBS", 4)) {
	fprintf(stderr, "dpf_ax ERROR: got invalid reply\n.");
	return -1;
    }
    // pass back return code set by peer:
    return ansbuf[12];
}

//###################################################################
// End dpfcore4driver.c
//###################################################################
