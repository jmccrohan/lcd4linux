/* $Id: drv_X11.c,v 1.1 2004/02/24 05:55:04 reinelt Exp $
 *
 * new style X11 Driver for LCD4Linux 
 *
 * Copyright 1999-2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old XWindow.c which is
 * Copyright 2000 Herbert Rosmanith <herp@wildsau.idv.uni-linz.ac.at>
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
 * $Log: drv_X11.c,v $
 * Revision 1.1  2004/02/24 05:55:04  reinelt
 *
 * X11 driver ported
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
#include "timer.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_graphic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[]="X11";

static char *fg_col,*bg_col,*hg_col;

static int pixel  = -1;	// pointsize in pixel
static int pgap   =  0;	// gap between points
static int rgap   =  0;	// row gap between lines
static int cgap   =  0;	// column gap between characters
static int border =  0;	// window border

static int dimx, dimy;	// total window dimension in pixel

static unsigned char *drv_X11_FB=NULL;

static Display *dp;
static int sc, dd;
static Window w, rw;
static Visual *vi;
static GC fg_gc, bg_gc, hg_gc;
static Colormap cm;
static XColor fg_xc, bg_xc, hg_xc;
static Pixmap pm;


#if 0
static LCD Lcd;

static unsigned char *LCDpixmap2;
static int DROWS=-1,DCOLS=-1;		/*DROWS+DCOLS without background*/
static int XRES=-1,YRES=-1;		/*XRES+YRES (same as self->...)*/
static int icons;                       /* number of user-defined icons */
static int async_update();		/*PROTO*/
static pid_t async_updater_pid=1;
static int semid=-1;
static int shmid=-1;
static int ppid;			/*parent pid*/

#endif

// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_X11_blit(int row, int col, int height, int width)
{
  int r, c;
  int dirty = 0;

  for (r=row; r<row+height && r<DROWS; r++) {
    int y = border + (r/YRES)*rgap + r*(pixel+pgap);
    for (c=col; c<col+width && c<DCOLS; c++) {
      int x = border + (c/XRES)*cgap + c*(pixel+pgap);
      unsigned char p = drv_generic_graphic_FB[r*LCOLS+c];
      if (drv_X11_FB[r*DCOLS+c] != p) {
	XFillRectangle( dp, w, p? fg_gc:hg_gc, x, y, pixel, pixel);
	drv_X11_FB[r*DCOLS+c] = p;
	dirty=1;
      }
    }
  }
  if (dirty) {
    XSync(dp, False);
  }
}


static void drv_X11_expose(int x, int y, int width, int height)
{
  /*
   * theory of operation:
   * instead of the old, fully-featured but complicated update
   * region calculation, we do an update of the whole display,
   * but check before every pixel if the pixel region is inside
   * the update region.
   */
  
  int r,  c;
  int x0, y0;
  int x1, y1;
  
  x0 = x-pixel;
  x1 = x+pixel+width;
  y0 = y-pixel;
  y1 = y+pixel+height;

  for (r=0; r<DROWS; r++) {
    int yc = border + (r/YRES)*rgap + r*(pixel+pgap);
    if (yc<y0 || yc>y1) continue;
    for (c=0; c<DCOLS; c++) {
      int xc = border + (c/XRES)*cgap + c*(pixel+pgap);
      if (xc<x0 || xc>x1) continue;
      XFillRectangle( dp, w, drv_generic_graphic_FB[r*LCOLS+c]? fg_gc:hg_gc, xc, yc, pixel, pixel);
    }
  }
  XSync(dp, False);
}


static void drv_X11_timer (void *notused)
{
  XEvent ev;
  
  if (XCheckWindowEvent(dp, w, ExposureMask, &ev)==0) return;
  if (ev.type==Expose) {
    drv_X11_expose (ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
  }
}


static int drv_X11_start (char *section)
{
  char *s;
  XSetWindowAttributes wa;
  XSizeHints sh;
  XColor dummy;
  XEvent ev;


  // read display size from config
  if (sscanf(s=cfg_get(section, "Size", "120x32"), "%dx%d", &DCOLS, &DROWS)!=2 || DCOLS<1 || DROWS<1) {
    error ("%s: bad Size '%s' from %s", Name, s, cfg_source());
    return -1;
  }
  
  if (sscanf(s=cfg_get(section, "font", "5x8"), "%dx%d", &XRES, &YRES)!=2 || XRES<1|| YRES<1) {
    error ("%s: bad font '%s' from %s", Name, s, cfg_source());
    return -1;
  }
  
  if (sscanf(s=cfg_get(section, "pixel", "4+1"), "%d+%d", &pixel, &pgap)!=2 || pixel<1 || pgap<0) {
    error ("%s: bad pixel '%s' from %s", Name, s, cfg_source());
    return -1;
  }
  
  if (sscanf(s=cfg_get(section, "gap", "-1x-1"), "%dx%d", &cgap, &rgap)!=2 || cgap<-1 || rgap<-1) {
    error ("%s: bad gap '%s' from %s", Name, s, cfg_source());
    return -1;
  }
  if (rgap<0) rgap=pixel+pgap;
  if (cgap<0) cgap=pixel+pgap;
  
  if (cfg_number(section, "border", 0, 0, 1000000, &border)<0) return -1;

  fg_col=cfg_get(section, "foreground", "#000000");
  bg_col=cfg_get(section, "background", "#80d000");
  hg_col=cfg_get(section, "halfground", "#70c000");
  if (*fg_col=='\\') fg_col++;
  if (*bg_col=='\\') bg_col++;
  if (*hg_col=='\\') hg_col++;


  drv_X11_FB = malloc(DCOLS*DROWS);
  if (drv_X11_FB==NULL) {
    error ("%s: framebuffer could not be allocated: malloc() failed", Name);
    return -1;
  }

  // init with 255 so all 'halfground' pixels will be drawn
  memset(drv_X11_FB, 255, DCOLS*DROWS*sizeof(*drv_X11_FB));

  if ((dp=XOpenDisplay(NULL))==NULL) {
    error ("%s: can't open display", Name);
    return -1;
  }
  
  sc    = DefaultScreen(dp);
  fg_gc = DefaultGC(dp, sc);
  vi    = DefaultVisual(dp, sc);
  dd    = DefaultDepth(dp, sc);
  rw    = DefaultRootWindow(dp);
  cm    = DefaultColormap(dp, sc);

  if (XAllocNamedColor(dp, cm, fg_col, &fg_xc, &dummy) == False) {
    error ("%s: can't alloc foreground color '%s'", Name, fg_col);
    return -1;
  }
  
  if (XAllocNamedColor(dp, cm, bg_col, &bg_xc, &dummy)==False) {
    error ("%s: can't alloc background color '%s'", Name, bg_col);
    return -1;
  }
  
  if (XAllocNamedColor(dp, cm, hg_col, &hg_xc, &dummy)==False) {
    error ("%s: can't alloc halfground color '%s'", Name, hg_col);
    return -1;
  }
  
  dimx = DCOLS*pixel + (DCOLS-1)*pgap + (DCOLS/XRES-1)*cgap;
  dimy = DROWS*pixel + (DROWS-1)*pgap + (DROWS/YRES-1)*rgap;

  wa.event_mask=ExposureMask;

  w = XCreateWindow(dp, rw, 0, 0, dimx+2*border, dimy+2*border, 0, 0, InputOutput, vi, CWEventMask, &wa);

  pm = XCreatePixmap(dp, w, dimx, dimy, dd);

  sh.min_width  = sh.max_width  = dimx+2*border;
  sh.min_height = sh.max_height = dimy+2*border;
  sh.flags = PPosition|PSize|PMinSize|PMaxSize;
  
  XSetWMProperties(dp, w, NULL, NULL, NULL, 0, &sh, NULL, NULL);


  XSetForeground(dp, fg_gc, fg_xc.pixel);
  XSetBackground(dp, fg_gc, bg_xc.pixel);

  bg_gc = XCreateGC(dp, w, 0, NULL);
  XSetForeground(dp, bg_gc, bg_xc.pixel);
  XSetBackground(dp, bg_gc, fg_xc.pixel);

  hg_gc = XCreateGC(dp, w, 0, NULL);
  XSetForeground(dp, hg_gc, hg_xc.pixel);
  XSetBackground(dp, hg_gc, fg_xc.pixel);

  XFillRectangle(dp, pm, bg_gc, 0, 0, dimx+2*border, dimy+2*border);
  XSetWindowBackground(dp, w, bg_xc.pixel);
  XClearWindow(dp, w);

  XStoreName(dp, w, "LCD4Linux");
  XMapWindow(dp, w);

  XFlush(dp);

  while(1) {
    XNextEvent(dp,&ev);
    if (ev.type==Expose && ev.xexpose.count==0)
      break;
  }
  
  // regularly process X events
  // Fixme: make 20msec configurable
  timer_add (drv_X11_timer, NULL, 20, 0);

  return 0;
}


#if 0
static int init_x(int rows,int cols,int XRES,int YRES) 
{
  return 0;
}


int xlcdinit(LCD *Self) 
{
  char *s;

  if (sscanf(s=cfg_get(NULL, "size", "20x4"),"%dx%d",&DCOLS,&DROWS)!=2
      || DROWS<1 || DCOLS<1) {
    error ("X11: bad size '%s'",s);
    return -1;
  }
  if (sscanf(s=cfg_get(NULL, "font", "5x8"),"%dx%d",&XRES,&YRES)!=2
      || XRES<5 || YRES>10) {
    error ("X11: bad font '%s'",s);
    return -1;
  }
  if (sscanf(s=cfg_get(NULL, "pixel", "4+1"),"%d+%d",&pixel,&pgap)!=2
      || pixel<1 || pgap<0) {
    error ("X11: bad pixel '%s'",s);
    return -1;
  }
  if (sscanf(s=cfg_get(NULL, "gap", "-1x-1"),"%dx%d",&cgap,&rgap)!=2
      || cgap<-1 || rgap<-1) {
    error ("X11: bad gap '%s'",s);
    return -1;
  }
  if (rgap<0) rgap=pixel+pgap;
  if (cgap<0) cgap=pixel+pgap;

  if (cfg_number(NULL, "border", 0, 0, 1000000, &border)<0) return -1;

  fg_col=cfg_get(NULL, "foreground", "#000000");
  bg_col=cfg_get(NULL, "background", "#80d000");
  hg_col=cfg_get(NULL, "halfground", "#70c000");
  if (*fg_col=='\\') fg_col++;
  if (*bg_col=='\\') bg_col++;
  if (*hg_col=='\\') hg_col++;

  if (pix_init(DROWS,DCOLS,XRES,YRES)==-1) return -1;

  if (cfg_number(NULL, "Icons", 0, 0, 8, &icons) < 0) return -1;
  if (icons>0) {
    info ("allocating %d icons", icons);
    icon_init(DROWS, DCOLS, XRES, YRES, 8, icons, pix_icon);
  }
  
  if (init_x(DROWS,DCOLS,XRES,YRES)==-1) return -1;
  init_signals();
  if (init_shm(DROWS*DCOLS*XRES*YRES,&LCDpixmap2)==-1) return -1;
  memset(LCDpixmap2,0xff,DROWS*YRES*DCOLS*XRES);
  if (init_thread(DROWS*DCOLS*XRES*YRES)==-1) return -1;
  Self->DROWS=DROWS;
  Self->DCOLS=DCOLS;
  Self->XRES=XRES;
  Self->YRES=YRES;
  Self->icons=icons;
  Lcd=*Self;

  pix_clear();
  return 0;
}


int xlcdflush() {
  int dirty;
  int row,col;
  
  acquire_lock();
  dirty=0;
  for(row=0;row<DROWS*YRES;row++) {
    int y=border+(row/YRES)*rgap+row*(pixel+pgap);
    for(col=0;col<DCOLS*XRES;col++) {
      int x=border+(col/XRES)*cgap+col*(pixel+pgap);
      int p=row*DCOLS*XRES+col;
      if (LCDpixmap[p]^LCDpixmap2[p]) {
	XFillRectangle(dp,w,LCDpixmap[p]?fg_gc:hg_gc,x,y,pixel,pixel);
	LCDpixmap2[p]=LCDpixmap[p];
	dirty=1;
      }
    }
  }
  if (dirty) XSync(dp,False);
  release_lock();
  return 0;
}


/*
 * this one should only be called from the updater-thread
 * no user serviceable parts inside
 */

static void update(int x,int y,int width,int height)
{
  /*
   * theory of operation:
   * instead of the old, fully-featured but complicated update
   * region calculation, we do an update of the whole display,
   * but check before every pixel if the pixel region is inside
   * the update region.
   */

  int x0, y0;
  int x1, y1;
  int row, col;
  int dirty;
  
  x0=x-pixel;
  y0=y-pixel;
  x1=x+pixel+width;
  y1=y+pixel+height;
  
  dirty=0;
  for(row=0; row<DROWS; row++) {
    int y = border + (row/YRES)*rgap + row*(pixel+pgap);
    if (y<y0 || y>y1) continue;
    for(col=0; col<DCOLS; col++) {
      int x = border + (col/XRES)*cgap + col*(pixel+pgap);
      int p;
      if (x<x0 || x>x1) continue;
      p=row*DCOLS*XRES+col;
      XFillRectangle(dp,w,LCDpixmap2[p]?fg_gc:hg_gc,x,y,pixel,pixel);
      dirty=1;
    }
  }
  if (dirty) XSync(dp,False);
}


static int async_update() 
{
  XSetWindowAttributes wa;
  XEvent ev;

  if ((dp=XOpenDisplay(NULL))==NULL)
    return -1;
  wa.event_mask=ExposureMask;
  XChangeWindowAttributes(dp,w,CWEventMask,&wa);
  for(;;) {
    XWindowEvent(dp,w,ExposureMask,&ev);
    if (ev.type==Expose) {
      acquire_lock();
      update(ev.xexpose.x,ev.xexpose.y,
	     ev.xexpose.width,ev.xexpose.height);
      release_lock();
    }
  }
}
#endif


// ****************************************
// ***            plugins               ***
// ****************************************

// none at the moment...


// ****************************************
// ***        widget callbacks          ***
// ****************************************


// using drv_generic_graphic_draw(W)
// using drv_generic_graphic_icon_draw(W)
// using drv_generic_graphic_bar_draw(W)


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_X11_list (void)
{
  printf ("any");
  return 0;
}


// initialize driver & display
int drv_X11_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // real worker functions
  drv_generic_graphic_real_blit   = drv_X11_blit;
  
  // start display
  if ((ret=drv_X11_start (section))!=0)
    return ret;
  
  // initialize generic graphic driver
  if ((ret=drv_generic_graphic_init(section, Name))!=0)
    return ret;
  
  // initially expose window
  drv_X11_expose (0, 0, dimx+2*border, dimy+2*border);
  
  // register text widget
  wc=Widget_Text;
  wc.draw=drv_generic_graphic_draw;
  widget_register(&wc);
  
  // register icon widget
  wc=Widget_Icon;
  wc.draw=drv_generic_graphic_icon_draw;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_generic_graphic_bar_draw;
  widget_register(&wc);
  
  // register plugins
  // none at the moment...
  
  
  return 0;
}


// close driver & display
int drv_X11_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_graphic_quit();
  
  if (drv_X11_FB) {
    free (drv_X11_FB);
    drv_X11_FB=NULL;
  }
  
  return (0);
}


DRIVER drv_X11 = {
  name: Name,
  list: drv_X11_list,
  init: drv_X11_init,
  quit: drv_X11_quit, 
};


