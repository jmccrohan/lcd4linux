/* $Id$
 * $URL$
 *
 * new style X11 Driver for LCD4Linux 
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
#include "rgb.h"
#include "drv.h"
#include "widget.h"
#include "widget_keypad.h"
#include "drv_generic_graphic.h"
#include "drv_generic_keypad.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[] = "X11";

static int pixel = -1;		/* pointsize in pixel */
static int pgap = 0;		/* gap between points */
static int rgap = 0;		/* row gap between lines */
static int cgap = 0;		/* column gap between characters */
static int border = 0;		/* window border */
static int buttons = 0;		/* number of keypad buttons */
static int btnwidth = 0;
static int btnheight = 0;

static int dimx, dimy;		/* total window dimension in pixel */
static Atom wmDeleteMessage;

static RGBA *drv_X11_FB = NULL;	/* framebuffer */

static RGBA BP_COL = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0x00 };	/* pixel background color */
static RGBA BR_COL = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0x00 };	/* border color */

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

static void drv_X11_color(RGBA c, int brightness)
{
    xc.red = brightness * c.R;
    xc.green = brightness * c.G;
    xc.blue = brightness * c.B;
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

    for (r = row; r < row + height; r++) {
	int y = border + (r / YRES) * rgap + r * (pixel + pgap);
	for (c = col; c < col + width; c++) {
	    int x = border + (c / XRES) * cgap + c * (pixel + pgap);
	    RGBA p1 = drv_X11_FB[r * DCOLS + c];
	    RGBA p2 = drv_generic_graphic_rgb(r, c);
	    if (p1.R != p2.R || p1.G != p2.G || p1.B != p2.B) {
		XClearArea(dp, w, x, y, pixel, pixel, 1);
		drv_X11_FB[r * DCOLS + c] = p2;
	    }
	}
    }
}


static int drv_X11_brightness(int brightness)
{
    static int Brightness = 255;

    /* -1 is used to query the current brightness */
    if (brightness == -1)
	return Brightness;

    if (brightness < 0)
	brightness = 0;
    if (brightness > 255)
	brightness = 255;

    if (Brightness != brightness) {

	float dim = brightness / 255.0;

	debug("%s: set brightness to %d%%", Name, (int) (dim * 100));

	BL_COL.R = BP_COL.R * dim;
	BL_COL.G = BP_COL.G * dim;
	BL_COL.B = BP_COL.B * dim;

	/* set new background */
	drv_X11_color(BR_COL, brightness);
	XSetWindowBackground(dp, w, xc.pixel);

	/* redraw every LCD pixel */
	drv_X11_blit(0, 0, LROWS, LCOLS);

	/* remember new brightness */
	Brightness = brightness;
    }

    return Brightness;
}


static int drv_X11_keypad(const int num)
{
    int val = WIDGET_KEY_PRESSED;

    switch (num) {
    case 1:
	val += WIDGET_KEY_UP;
	break;
    case 2:
	val += WIDGET_KEY_DOWN;
	break;
    case 3:
	val += WIDGET_KEY_LEFT;
	break;
    case 4:
	val += WIDGET_KEY_RIGHT;
	break;
    case 5:
	val += WIDGET_KEY_CONFIRM;
	break;
    case 6:
	val += WIDGET_KEY_CANCEL;
	break;
    default:
	error("%s: unknown keypad value %d", Name, num);
    }

    return val;
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
    XFontStruct *xfs;
    int xoffset = border + (DCOLS / XRES) * cgap + DCOLS * (pixel + pgap);
    int yoffset = border + (DROWS / YRES) * rgap;
    int yk;
    char *s;
    char unknownTxt[10];
    XRectangle rect[DROWS * DCOLS];
    int nrect = 0;
    RGBA col;
    RGBA lastCol = { 0, 0, 0, 0 };
    int hasLastCol = 0;

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
	    col = drv_generic_graphic_rgb(r, c);
	    if (hasLastCol) {
		/* if the color of this pixel is different to the last pixels draw the old ones */
		if (col.R != lastCol.R || col.G != lastCol.G || col.B != lastCol.B) {
		    drv_X11_color(lastCol, 255);
		    XFillRectangles(dp, w, gc, rect, nrect);
		    nrect = 0;
		    lastCol = col;
		}
		rect[nrect].x = xc;
		rect[nrect].y = yc;
		rect[nrect].width = pixel;
		rect[nrect].height = pixel;
		nrect++;
	    } else {
		/* 1st shot: no old color */
		drv_X11_color(col, 255);
		XFillRectangle(dp, w, gc, xc, yc, pixel, pixel);
		lastCol = col;
		hasLastCol = 1;
	    }
	}
    }
    /* draw the last block of rectangles */
    drv_X11_color(lastCol, 255);
    XFillRectangles(dp, w, gc, rect, nrect);

    /* Keypad on the right side */
    if (x1 >= xoffset) {
	xfs = XQueryFont(dp, XGContextFromGC(DefaultGC(dp, 0)));
	if (drv_X11_brightness(-1) > 127) {
	    drv_X11_color(FG_COL, 255);
	} else {
	    drv_X11_color(BG_COL, 255);
	}
	for (r = 0; r < buttons; r++) {
	    yk = yoffset + r * (btnheight + pgap);
	    switch (r) {
	    case 0:
		s = "Up";
		break;
	    case 1:
		s = "Down";
		break;
	    case 2:
		s = "Left";
		break;
	    case 3:
		s = "Right";
		break;
	    case 4:
		s = "Confirm";
		break;
	    case 5:
		s = "Cancel";
		break;
	    default:
		snprintf(unknownTxt, sizeof(unknownTxt), "#%d??", r);
		s = unknownTxt;
	    }
	    XDrawRectangle(dp, w, gc, xoffset, yk, btnwidth, btnheight - 2);
	    XDrawString(dp, w, gc,
			xoffset + btnwidth / 2 - (xfs->max_bounds.width * strlen(s)) / 2,
			yk + btnheight / 2 + xfs->max_bounds.ascent / 2, s, strlen(s));
	}
    }
}


static void drv_X11_timer( __attribute__ ((unused))
			  void *notused)
{
    XEvent ev;
    XRectangle exp;
    int xoffset = border + (DCOLS / XRES) * cgap + DCOLS * (pixel + pgap);
    int yoffset = border + (DROWS / YRES) * rgap;
    static int btn = 0;

    if (XCheckWindowEvent(dp, w, ExposureMask | ButtonPressMask | ButtonReleaseMask, &ev) == 0)
	return;

    switch (ev.type) {

    case Expose:
	/* collect all expose events in eventqueue */
	exp.x = ev.xexpose.x;
	exp.y = ev.xexpose.y;
	exp.width = ev.xexpose.width;
	exp.height = ev.xexpose.height;
	while (XCheckWindowEvent(dp, w, ExposureMask, &ev)) {
	    if (ev.xexpose.x < exp.x) {
		exp.width += exp.x - ev.xexpose.x;
		exp.x -= exp.x - ev.xexpose.x;
	    }
	    if (ev.xexpose.y < exp.y) {
		exp.height += exp.y - ev.xexpose.y;
		exp.y -= exp.y - ev.xexpose.y;
	    }
	    if (ev.xexpose.x + ev.xexpose.width > exp.x + exp.width) {
		exp.width += ev.xexpose.x + ev.xexpose.width - (exp.x + exp.width);
	    }
	    if (ev.xexpose.y + ev.xexpose.height > exp.y + exp.height) {
		exp.height += ev.xexpose.y + ev.xexpose.height - (exp.y + exp.height);
	    }
	}
	drv_X11_expose(exp.x, exp.y, exp.width, exp.height);
	break;

    case ButtonPress:
	if (ev.xbutton.x >= xoffset && ev.xbutton.x <= xoffset + btnwidth
	    && ev.xbutton.y >= yoffset && ev.xbutton.y <= yoffset + buttons * btnheight + (buttons - 1) * pgap) {
	    btn = (ev.xbutton.y - yoffset) / (btnheight + pgap) + 1;	/* btn 0 is unused */
	    debug("button %d pressed", btn);
	    drv_X11_color(BG_COL, 255);
	    XFillRectangle(dp, w, gc, xoffset + 1, yoffset + (btn - 1) * (btnheight + pgap) + 1, btnwidth - 1,
			   btnheight - 2 - 1);
	    drv_generic_keypad_press(btn);
	}
	break;

    case ButtonRelease:
	if (ev.xbutton.x >= xoffset && ev.xbutton.x <= xoffset + btnwidth
	    && ev.xbutton.y >= yoffset && ev.xbutton.y <= yoffset + buttons * btnheight + (buttons - 1) * pgap) {
	    btn = (ev.xbutton.y - yoffset) / (btnheight + pgap) + 1;	/* btn 0 is unused */
	    debug("button %d released", btn);
	    XClearArea(dp, w, xoffset, yoffset + (btn - 1) * (btnheight + pgap), btnwidth, btnheight - 2,
		       1 /* true */ );
	}
	break;

    case ClientMessage:
	if ((Atom) (ev.xclient.data.l[0]) == wmDeleteMessage) {
	    info("%s: Window closed by WindowManager, quit.", Name);
	    exit(0);
	} else {
	    debug("%s: Got client message 0x%lx %lx %lx %lx %lx", Name, ev.xclient.data.l[0],
		  ev.xclient.data.l[1], ev.xclient.data.l[2], ev.xclient.data.l[3], ev.xclient.data.l[4]);
	}

    default:
	debug("%s: unknown XEvent %d", Name, ev.type);
    }
}


static int drv_X11_start(const char *section)
{
    int i;
    char *s;
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

    /* special case for the X11 driver: 
     * the border color may be different from the backlight color
     * the backlicht color is the color of an inactive pixel
     * the border color is the color of the border and gaps between pixels
     * for the brightness pugin we need a copy of BL_COL, we call it BP_COL
     */
    s = cfg_get(section, "basecolor", "000000ff");
    if (color2RGBA(s, &BP_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Name, s);
    }
    free(s);
    BL_COL = BP_COL;

    BR_COL = BP_COL;
    if ((s = cfg_get(section, "bordercolor", NULL)) != NULL) {
	if (color2RGBA(s, &BR_COL) < 0) {
	    error("%s: ignoring illegal color '%s'", Name, s);
	}
	free(s);
    }

    /* virtual keyboard: number of buttons (0..6) */
    if (cfg_number(section, "buttons", 0, 0, 6, &buttons) < 0)
	return -1;

    drv_X11_FB = malloc(DCOLS * DROWS * sizeof(*drv_X11_FB));
    if (drv_X11_FB == NULL) {
	error("%s: framebuffer could not be allocated: malloc() failed", Name);
	return -1;
    }

    for (i = 0; i < DCOLS * DROWS; i++) {
	drv_X11_FB[i] = NO_COL;
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
    if (buttons != 0) {
	btnwidth = (DCOLS * pixel + (DCOLS - 1) * pgap) / 10;
	btnheight = (DROWS * pixel + (DROWS - 1) * pgap) / buttons;
    }

    wa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask;

    sh.min_width = sh.max_width = dimx + 2 * border + btnwidth;
    sh.min_height = sh.max_height = dimy + 2 * border;
    sh.flags = PPosition | PSize | PMinSize | PMaxSize;

    if (sh.min_width > DisplayWidth(dp, sc) || sh.min_height > DisplayHeight(dp, sc)) {
	error("%s: Warning: X11-Window with dimensions (%d,%d) is greater than display (%d,%d)!",
	      Name, sh.min_width, sh.min_height, DisplayWidth(dp, sc), DisplayHeight(dp, sc));
	if (sh.min_width > 32767 || sh.min_height > 32676) {
	    /* XProtocol data size exceeded */
	    exit(1);
	}
    }
    w = XCreateWindow(dp, rw, 0, 0, sh.min_width, sh.min_height, 0, 0, InputOutput, vi, CWEventMask, &wa);

    pm = XCreatePixmap(dp, w, dimx, dimy, dd);

    XSetWMProperties(dp, w, NULL, NULL, NULL, 0, &sh, NULL, NULL);
    wmDeleteMessage = XInternAtom(dp, "WM_DELETE_WINDOW", False);
    /* XSetWMProtocols(dp, w, &wmDeleteMessage, 1); *to be tested* */

    drv_X11_color(BR_COL, 255);
    XSetWindowBackground(dp, w, xc.pixel);
    XClearWindow(dp, w);

    /* set brightness (after first background painting) */
    if (cfg_number(section, "Brightness", 255, 0, 255, &i) > 0) {
	drv_X11_brightness(i);
    }

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

static void plugin_brightness(RESULT * result, const int argc, RESULT * argv[])
{
    double brightness;

    switch (argc) {
    case 0:
	brightness = drv_X11_brightness(-1);
	SetResult(&result, R_NUMBER, &brightness);
	break;
    case 1:
	brightness = drv_X11_brightness(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &brightness);
	break;
    default:
	error("%s.brightness(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
}


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_X11_list(void)
{
    printf("any X11 server");
    return 0;
}


/* initialize driver & display */
int drv_X11_init(const char *section, const int quiet)
{
    RGBA bl_col;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* start display */
    if ((ret = drv_X11_start(section)) != 0)
	return ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_X11_blit;
    drv_generic_keypad_real_press = drv_X11_keypad;

    /* initialize generic graphic driver */
    /* save BL_COL which may already be dimmed */
    bl_col = BL_COL;
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;
    BL_COL = bl_col;

    /* initialize generic key pad driver */
    if ((ret = drv_generic_keypad_init(section, Name)) != 0)
	return ret;

    drv_generic_graphic_clear();

    /* initially expose window */
    drv_X11_expose(0, 0, dimx + 2 * border, dimy + 2 * border);

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    drv_X11_expose(0, 0, dimx + 2 * border, dimy + 2 * border);
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    /* register plugins */
    AddFunction("LCD::brightness", -1, plugin_brightness);


    return 0;
}


/* close driver & display */
int drv_X11_quit(const __attribute__ ((unused))
		 int quiet)
{

    info("%s: shutting down.", Name);
    drv_generic_graphic_quit();
    drv_generic_keypad_quit();

    if (drv_X11_FB) {
	free(drv_X11_FB);
	drv_X11_FB = NULL;
    }

    return (0);
}


DRIVER drv_X11 = {
    .name = Name,
    .list = drv_X11_list,
    .init = drv_X11_init,
    .quit = drv_X11_quit,
};
