/* $Id$
 * $URL$
 *
 * new style X11 Driver for LCD4Linux 
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old XWindow.c which is
 * Copyright (C) 2000 Herbert Rosmanith <herp@wildsau.idv.uni-linz.ac.at>
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
 * struct DRIVER drv_X11
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
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "timer.h"
#include "plugin.h"
#include "drv.h"
#include "drv_generic_graphic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[] = "X11";

static int pixel = -1;		/* pointsize in pixel */
static int pgap = 0;		/* gap between points */
static int rgap = 0;		/* row gap between lines */
static int cgap = 0;		/* column gap between characters */
static int border = 0;		/* window border */

static int dimx, dimy;		/* total window dimension in pixel */

static RGBA *drv_X11_FB = NULL;

static Display *dp;
static int sc, dd;
static Window w, rw;
static Visual *vi;
static GC gc;
static Colormap cm;
static XColor xc;
static Pixmap pm;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_X11_color(RGBA c)
{
    xc.red = 255 * c.R;
    xc.green = 255 * c.G;
    xc.blue = 255 * c.B;
    xc.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(dp, cm, &xc) == False) {
	error("%s: XAllocColor(%02x%02x%02x) failed!", Name, c.R, c.G, c.B);
    }
    XSetForeground(dp, gc, xc.pixel);
    XSetBackground(dp, gc, xc.pixel);

}


static void drv_X11_blit(const int row, const int col, const int height, const int width)
{
    int r, c;
    int dirty = 0;

    for (r = row; r < row + height; r++) {
	int y = border + (r / YRES) * rgap + r * (pixel + pgap);
	for (c = col; c < col + width; c++) {
	    int x = border + (c / XRES) * cgap + c * (pixel + pgap);
	    RGBA p1 = drv_X11_FB[r * DCOLS + c];
	    RGBA p2 = drv_generic_graphic_rgb(r, c);
	    if (p1.R != p2.R || p1.G != p2.G || p1.B != p2.B) {
		drv_X11_color(p2);
		XFillRectangle(dp, w, gc, x, y, pixel, pixel);
		drv_X11_FB[r * DCOLS + c] = p2;
		dirty = 1;
	    }
	}
    }
    if (dirty) {
	XSync(dp, False);
    }
}


static void drv_X11_expose(const int x, const int y, const int width, const int height)
{
    /*
     * theory of operation:
     * instead of the old, fully-featured but complicated update
     * region calculation, we do an update of the whole display,
     * but check before every pixel if the pixel region is inside
     * the update region.
     */

    int r, c;
    int x0, y0;
    int x1, y1;

    x0 = x - pixel;
    x1 = x + pixel + width;
    y0 = y - pixel;
    y1 = y + pixel + height;

    for (r = 0; r < DROWS; r++) {
	int yc = border + (r / YRES) * rgap + r * (pixel + pgap);
	if (yc < y0 || yc > y1)
	    continue;
	for (c = 0; c < DCOLS; c++) {
	    int xc = border + (c / XRES) * cgap + c * (pixel + pgap);
	    if (xc < x0 || xc > x1)
		continue;
	    drv_X11_color(drv_generic_graphic_rgb(r, c));
	    XFillRectangle(dp, w, gc, xc, yc, pixel, pixel);
	}
    }
    XSync(dp, False);
}


static void drv_X11_timer( __attribute__ ((unused))
			  void *notused)
{
    XEvent ev;

    if (XCheckWindowEvent(dp, w, ExposureMask, &ev) == 0)
	return;
    if (ev.type == Expose) {
	drv_X11_expose(ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
    }
}


static int drv_X11_start(const char *section)
{
    int i;
    char *s;
    RGBA BC;
    XSetWindowAttributes wa;
    XSizeHints sh;
    XEvent ev;

    /* read display size from config */
    if (sscanf(s = cfg_get(section, "Size", "120x32"), "%dx%d", &DCOLS, &DROWS) != 2 || DCOLS < 1 || DROWS < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "font", "5x8"), "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad %s.font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "pixel", "4+1"), "%d+%d", &pixel, &pgap) != 2 || pixel < 1 || pgap < 0) {
	error("%s: bad %s.pixel '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "gap", "-1x-1"), "%dx%d", &cgap, &rgap) != 2 || cgap < -1 || rgap < -1) {
	error("%s: bad %s.gap '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (rgap < 0)
	rgap = pixel + pgap;
    if (cgap < 0)
	cgap = pixel + pgap;

    if (cfg_number(section, "border", 0, 0, -1, &border) < 0)
	return -1;

    s = cfg_get(section, "basecolor", "000000ff");
    if (color2RGBA(s, &BC) < 0) {
	error("%s: ignoring illegal color '%s'", Name, s);
    }
    free(s);

    drv_X11_FB = malloc(DCOLS * DROWS * sizeof(*drv_X11_FB));
    if (drv_X11_FB == NULL) {
	error("%s: framebuffer could not be allocated: malloc() failed", Name);
	return -1;
    }

    for (i = 0; i < DCOLS * DROWS; i++) {
	drv_X11_FB[i] = BC;
    }

    if ((dp = XOpenDisplay(NULL)) == NULL) {
	error("%s: can't open display", Name);
	return -1;
    }

    sc = DefaultScreen(dp);
    gc = DefaultGC(dp, sc);
    vi = DefaultVisual(dp, sc);
    dd = DefaultDepth(dp, sc);
    rw = DefaultRootWindow(dp);
    cm = DefaultColormap(dp, sc);

    dimx = DCOLS * pixel + (DCOLS - 1) * pgap + (DCOLS / XRES - 1) * cgap;
    dimy = DROWS * pixel + (DROWS - 1) * pgap + (DROWS / YRES - 1) * rgap;

    wa.event_mask = ExposureMask;

    w = XCreateWindow(dp, rw, 0, 0, dimx + 2 * border, dimy + 2 * border, 0, 0, InputOutput, vi, CWEventMask, &wa);

    pm = XCreatePixmap(dp, w, dimx, dimy, dd);

    sh.min_width = sh.max_width = dimx + 2 * border;
    sh.min_height = sh.max_height = dimy + 2 * border;
    sh.flags = PPosition | PSize | PMinSize | PMaxSize;

    XSetWMProperties(dp, w, NULL, NULL, NULL, 0, &sh, NULL, NULL);

    drv_X11_color(BC);

    XFillRectangle(dp, pm, gc, 0, 0, dimx + 2 * border, dimy + 2 * border);
    XSetWindowBackground(dp, w, xc.pixel);
    XClearWindow(dp, w);

    XStoreName(dp, w, "LCD4Linux");
    XMapWindow(dp, w);

    XFlush(dp);

    while (1) {
	XNextEvent(dp, &ev);
	if (ev.type == Expose && ev.xexpose.count == 0)
	    break;
    }

    /* regularly process X events */
    /* Fixme: make 20msec configurable */
    timer_add(drv_X11_timer, NULL, 20, 0);

    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_X11_list(void)
{
    printf("any");
    return 0;
}


/* initialize driver & display */
int drv_X11_init(const char *section, const int quiet)
{
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* start display */
    if ((ret = drv_X11_start(section)) != 0)
	return ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_X11_blit;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    drv_generic_graphic_clear();

    /* initially expose window */
    drv_X11_expose(0, 0, dimx + 2 * border, dimy + 2 * border);

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    /* register plugins */
    /* none at the moment... */


    return 0;
}


/* close driver & display */
int drv_X11_quit(const __attribute__ ((unused))
		 int quiet)
{

    info("%s: shutting down.", Name);
    drv_generic_graphic_quit();

    if (drv_X11_FB) {
	free(drv_X11_FB);
	drv_X11_FB = NULL;
    }

    return (0);
}


DRIVER drv_X11 = {
  name:Name,
  list:drv_X11_list,
  init:drv_X11_init,
  quit:drv_X11_quit,
};
