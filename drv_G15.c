/* $Id: drv_G15.c,v 1.4 2006/01/30 06:25:49 reinelt Exp $
 *
 * Driver for Logitech G-15 keyboard LCD screen
 *
 * Copyright (C) 2006 Dave Ingram <dave@partis-project.net>
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_G15.c,v $
 * Revision 1.4  2006/01/30 06:25:49  reinelt
 * added CVS Revision
 *
 * Revision 1.3  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 * Revision 1.2  2006/01/21 17:43:25  reinelt
 * minor cosmetic fixes
 *
 * Revision 1.1  2006/01/21 13:26:44  reinelt
 * Logitech G-15 keyboard LCD driver from Dave Ingram
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_G15
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
#include "drv_generic_graphic.h"

#define G15_VENDOR 0x046d
#define G15_DEVICE 0xc222

#if 0
#define DEBUG(x) debug("%s(): %s", __FUNCTION__, x);
#else
#define DEBUG(x)
#endif

static char Name[] = "G-15";

static usb_dev_handle *g15_lcd;

static unsigned char *g15_image;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_G15_open()
{
    struct usb_bus *bus;
    struct usb_device *dev;
    char dname[32] = { 0 };

    g15_lcd = NULL;

    info("%s: Scanning USB for G-15 keyboard...", Name);

    usb_init();
    usb_set_debug(0);
    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((g15_lcd = usb_open(dev))) {
		if ((dev->descriptor.idVendor == G15_VENDOR) && (dev->descriptor.idProduct == G15_DEVICE)) {

		    /* detach from the kernel if we need to */
		    int retval = usb_get_driver_np(g15_lcd, 0, dname, 31);
		    if (retval == 0 && strcmp(dname, "usbhid") == 0) {
			usb_detach_kernel_driver_np(g15_lcd, 0);
		    }
		    usb_set_configuration(g15_lcd, 1);
		    usleep(100);
		    usb_claim_interface(g15_lcd, 0);
		    return 0;
		} else {
		    usb_close(g15_lcd);
		}
	    }
	}
    }

    return -1;
}


static int drv_G15_close(void)
{
    usb_release_interface(g15_lcd, 0);
    if (g15_lcd)
	usb_close(g15_lcd);

    return 0;
}


static void drv_G15_update_img()
{
    int i, j, k;
    unsigned char *output = malloc(160 * 43 * sizeof(unsigned char));

    DEBUG("entered");
    if (!output)
	return;

    DEBUG("memory allocated");
    memset(output, 0, 160 * 43);
    DEBUG("memory set");
    output[0] = 0x03;
    DEBUG("first output set");

    for (k = 0; k < 6; k++) {
	for (i = 0; i < 160; i++) {
	    int maxj = (k == 5) ? 3 : 8;
	    for (j = 0; j < maxj; j++) {
		if (g15_image[(k * 1280) + i + (j * 160)])
		    output[32 + i + (k * 160)] |= (1 << j);
	    }
	}
    }

    DEBUG("output array prepared");

    usb_bulk_write(g15_lcd, 0x02, (char *) output, 992, 1000);
    usleep(300);

    DEBUG("data written to LCD");

    free(output);

    DEBUG("memory freed");
    DEBUG("left");
}



/* for graphic displays only */
static void drv_G15_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    DEBUG("entered");

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    g15_image[r * 160 + c] = drv_generic_graphic_gray(r, c);
	}
    }

    DEBUG("updating image");

    drv_G15_update_img();

    DEBUG("left");
}


/* start graphic display */
static int drv_G15_start(const char *section)
{
    char *s;

    DEBUG("entered");

    /* read display size from config */
    DROWS = 160;
    DCOLS = 43;

    DEBUG("display size set");

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

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    DEBUG("Finished config stuff");

    DEBUG("allocating image buffer");
    /* you surely want to allocate a framebuffer or something... */
    g15_image = malloc(160 * 43 * sizeof(unsigned char));
    if (!g15_image)
	return -1;
    DEBUG("allocated");
    memset(g15_image, 0, 160 * 43);
    DEBUG("zeroed");

    /* open communication with the display */
    DEBUG("opening display...");
    if (drv_G15_open() < 0) {
	DEBUG("opening failed");
	return -1;
    }
    DEBUG("display open");

    /* reset & initialize display */
    DEBUG("clearing display");
    drv_G15_update_img();
    DEBUG("done");

    /*
       if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
       drv_G15_contrast(contrast);
       }
     */

    DEBUG("left");

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_gpio_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_G15_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_G15_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Revision: 1.4 $");

    DEBUG("entered");

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_G15_blit;

    /* start display */
    if ((ret = drv_G15_start(section)) != 0)
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

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_graphic_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_graphic_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_graphic_bar_draw;
    widget_register(&wc);

    DEBUG("left");

    return 0;
}



/* close driver & display */
int drv_G15_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    DEBUG("clearing display");
    /* clear display */
    drv_generic_graphic_clear();

    DEBUG("saying goodbye");
    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    DEBUG("generic_graphic_quit()");
    drv_generic_graphic_quit();

    DEBUG("closing connection");
    drv_G15_close();

    DEBUG("freeing image alloc");
    free(g15_image);

    return (0);
}


DRIVER drv_G15 = {
  name:Name,
  list:drv_G15_list,
  init:drv_G15_init,
  quit:drv_G15_quit,
};
