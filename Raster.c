/* $Id: Raster.c,v 1.26 2003/09/09 06:54:43 reinelt Exp $
 *
 * driver for raster formats
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: Raster.c,v $
 * Revision 1.26  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.25  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.24  2003/08/17 12:11:58  reinelt
 * framework for icons prepared
 *
 * Revision 1.23  2003/07/29 04:56:13  reinelt
 * disable Raster driver automagically if gd.h not found
 *
 * Revision 1.22  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.21  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.20  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.19  2001/09/10 13:55:53  reinelt
 * M50530 driver
 *
 * Revision 1.18  2001/03/16 16:40:17  ltoetsch
 * implemented time bar
 *
 * Revision 1.17  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.16  2001/03/02 17:18:52  reinelt
 *
 * let configure find gd.h
 *
 * Revision 1.15  2001/03/02 10:18:03  ltoetsch
 * added /proc/apm battery stat
 *
 * Revision 1.14  2001/03/01 22:33:50  reinelt
 *
 * renamed Raster_flush() to PPM_flush()
 *
 * Revision 1.13  2001/03/01 15:11:30  ltoetsch
 * added PNG,Webinterface
 *
 * Revision 1.12  2001/03/01 11:08:16  reinelt
 *
 * reworked configure to allow selection of drivers
 *
 * Revision 1.11  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.10  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.9  2000/04/03 04:01:31  reinelt
 *
 * if 'gap' is specified as -1, a gap of (pixelsize+pixelgap) is selected automatically
 *
 * Revision 1.8  2000/03/28 07:22:15  reinelt
 *
 * version 0.95 released
 * X11 driver up and running
 * minor bugs fixed
 *
 * Revision 1.7  2000/03/26 20:00:44  reinelt
 *
 * README.Raster added
 *
 * Revision 1.6  2000/03/26 19:03:52  reinelt
 *
 * more Pixmap renaming
 * quoting of '#' in config file
 *
 * Revision 1.5  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.4  2000/03/26 12:55:03  reinelt
 *
 * enhancements to the PPM driver
 *
 * Revision 1.3  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.2  2000/03/24 11:36:56  reinelt
 *
 * new syntax for raster configuration
 * changed XRES and YRES to be configurable
 * PPM driver works nice
 *
 * Revision 1.1  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD Raster[]
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WITH_PNG
#ifdef HAVE_GD_GD_H
#include <gd/gd.h>
#else
#ifdef HAVE_GD_H
#include <gd.h>
#else
#error "gd.h not found!"
#error "cannot compile PNG driver"
#endif
#endif
#endif

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "pixmap.h"

static LCD Lcd;

static int pixel=-1;
static int pgap=0;
static int rgap=0;
static int cgap=0;
static int border=0;

static unsigned int foreground=0;
static unsigned int halfground=0;
static unsigned int background=0;


#ifdef WITH_PPM
int PPM_flush (void)
{
  static int seq=0;
  static unsigned char *bitbuf=NULL;
  static unsigned char *rowbuf=NULL;
  int xsize, ysize, row, col;
  unsigned char R[3], G[3], B[3];
  char path[256], tmp[256], buffer[256];
  int fd;

  xsize=2*border+(Lcd.cols-1)*cgap+Lcd.cols*Lcd.xres*pixel+(Lcd.cols*Lcd.xres-1)*pgap;
  ysize=2*border+(Lcd.rows-1)*rgap+Lcd.rows*Lcd.yres*pixel+(Lcd.rows*Lcd.yres-1)*pgap;
  
  if (bitbuf==NULL) {
    if ((bitbuf=malloc(xsize*ysize*sizeof(*bitbuf)))==NULL) {
      error ("Raster: malloc(%d) failed: %s", xsize*ysize*sizeof(*bitbuf), strerror(errno));
      return -1;
    }
  }
  
  if (rowbuf==NULL) {
    if ((rowbuf=malloc(3*xsize*sizeof(*rowbuf)))==NULL) {
      error ("Raster: malloc(%d) failed: %s", 3*xsize*sizeof(*rowbuf), strerror(errno));
      return -1;
    }
  }
  
  memset (bitbuf, 0, xsize*ysize*sizeof(*bitbuf));
  
  for (row=0; row<Lcd.rows*Lcd.yres; row++) {
    int y=border+(row/Lcd.yres)*rgap+row*(pixel+pgap);
    for (col=0; col<Lcd.cols*Lcd.xres; col++) {
      int x=border+(col/Lcd.xres)*cgap+col*(pixel+pgap);
      int a, b;
      for (a=0; a<pixel; a++)
	for (b=0; b<pixel; b++)
	  bitbuf[y*xsize+x+a*xsize+b]=LCDpixmap[row*Lcd.cols*Lcd.xres+col]+1;
    }
  }
  
  snprintf (path, sizeof(path), output, seq++);
  snprintf (tmp, sizeof(tmp), "%s.tmp", path);
  
  if ((fd=open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644))<0) {
    error ("Raster: open(%s) failed: %s", tmp, strerror(errno));
    return -1;
  }
  
  snprintf (buffer, sizeof(buffer), "P6\n%d %d\n255\n", xsize, ysize);
  if (write (fd, buffer, strlen(buffer))<0) {
    error ("Raster: write(%s) failed: %s", tmp, strerror(errno));
    return -1;
  }
  
  R[0]=0xff&background>>16;
  G[0]=0xff&background>>8;
  B[0]=0xff&background;
  
  R[1]=0xff&halfground>>16;
  G[1]=0xff&halfground>>8;
  B[1]=0xff&halfground;
  
  R[2]=0xff&foreground>>16;
  G[2]=0xff&foreground>>8;
  B[2]=0xff&foreground;

  for (row=0; row<ysize; row++) {
    int c=0;
    for (col=0; col<xsize; col++) {
      int i=bitbuf[row*xsize+col];
      rowbuf[c++]=R[i];
      rowbuf[c++]=G[i];
      rowbuf[c++]=B[i];
    }
    if (write (fd, rowbuf, c)<0) {
      error ("Raster: write(%s) failed: %s", tmp, strerror(errno));
      return -1;
    }
  }
  
  if (close (fd)<0) {
    error ("Raster: close(%s) failed: %s", tmp, strerror(errno));
    return -1;
  }
  if (rename (tmp, path)<0) {
    error ("Raster: rename(%s) failed: %s", tmp, strerror(errno));
    return -1;
  }
  
  return 0;
}
#endif

#ifdef WITH_PNG
int PNG_flush (void)
{
  static int seq=0;
  int xsize, ysize, row, col;
  char path[256], tmp[256];
  FILE *fp;
  gdImagePtr im;
  int bg, hg, fg;
  
  xsize=2*border+(Lcd.cols-1)*cgap+Lcd.cols*Lcd.xres*pixel+(Lcd.cols*Lcd.xres-1)*pgap;
  ysize=2*border+(Lcd.rows-1)*rgap+Lcd.rows*Lcd.yres*pixel+(Lcd.rows*Lcd.yres-1)*pgap;

  im = gdImageCreate(xsize, ysize);
  /* first color = background */
  bg = gdImageColorAllocate(im, 
			    0xff&background>>16,
			    0xff&background>>8,
                            0xff&background);
  hg = gdImageColorAllocate(im, 
			    0xff&halfground>>16,
			    0xff&halfground>>8,
                            0xff&halfground);
  
  fg = gdImageColorAllocate(im, 
			    0xff&foreground>>16,
			    0xff&foreground>>8,
                            0xff&foreground);
  
  
  for (row=0; row<Lcd.rows*Lcd.yres; row++) {
    int y=border+(row/Lcd.yres)*rgap+row*(pixel+pgap);
    for (col=0; col<Lcd.cols*Lcd.xres; col++) {
      int x=border+(col/Lcd.xres)*cgap+col*(pixel+pgap);
      gdImageFilledRectangle(im, x, y, x + pixel - 1 , y + pixel - 1,
			     LCDpixmap[row*Lcd.cols*Lcd.xres+col]? fg : hg);
    }
  }
  
  snprintf (path, sizeof(path), output, seq++);
  snprintf (tmp, sizeof(tmp), "%s.tmp", path);
  
  if ((fp=fopen(tmp, "w")) == NULL) {
    error("Png: fopen(%s) failed: %s\n", tmp, strerror(errno));
    return -1;
  }
  gdImagePng(im, fp);
  gdImageDestroy(im);
  
  
  if (fclose (fp) != 0) {
    error("Png: fclose(%s) failed: %s\n", tmp, strerror(errno));
    return -1;
  }
  if (rename (tmp, path)<0) {
    error("Png: rename(%s) failed: %s\n", tmp, strerror(errno));
    return -1;
  }
  
  return 0;
}
#endif


int Raster_clear (int full)
{
  if (pix_clear()!=0) 
    return -1;

  return 0;
}

int Raster_init (LCD *Self)
{
  char *s;
  int rows=-1, cols=-1;
  int xres=1, yres=-1;
  
  if (output==NULL || *output=='\0') {
    error ("Raster: no output file specified (use -o switch)");
    return -1;
  }
    
  if (sscanf(s=cfg_get("size","20x4"), "%dx%d", &cols, &rows)!=2 || rows<1 || cols<1) {
    error ("Raster: bad size '%s'", s);
    return -1;
  }

  if (sscanf(s=cfg_get("font","5x8"), "%dx%d", &xres, &yres)!=2 || xres<5 || yres<7) {
    error ("Raster: bad font '%s'", s);
    return -1;
  }

  if (sscanf(s=cfg_get("pixel","4+1"), "%d+%d", &pixel, &pgap)!=2 || pixel<1 || pgap<0) {
    error ("Raster: bad pixel '%s'", s);
    return -1;
  }

  if (sscanf(s=cfg_get("gap","3x3"), "%dx%d", &cgap, &rgap)!=2 || cgap<-1 || rgap<-1) {
    error ("Raster: bad gap '%s'", s);
    return -1;
  }
  if (rgap<0) rgap=pixel+pgap;
  if (cgap<0) cgap=pixel+pgap;

  if (cfg_number("border", 0, 0, 1000000, &border)<0) return -1;

  if (sscanf(s=cfg_get("foreground","#102000"), "#%x", &foreground)!=1) {
    error ("Raster: bad foreground color '%s'", s);
    return -1;
  }
  if (sscanf(s=cfg_get("halfground","#70c000"), "#%x", &halfground)!=1) {
    error ("Raster: bad halfground color '%s'", s);
    return -1;
  }
  if (sscanf(s=cfg_get("background","#80d000"), "#%x", &background)!=1) {
    error ("Raster: bad background color '%s'", s);
    return -1;
  }

  if (pix_init (rows, cols, xres, yres)!=0) {
    error ("Raster: pix_init(%d, %d, %d, %d) failed", rows, cols, xres, yres);
    return -1;
  }

  Self->rows=rows;
  Self->cols=cols;
  Self->xres=xres;
  Self->yres=yres;
  Lcd=*Self;

  pix_clear();
  return 0;
}

int Raster_put (int row, int col, char *text)
{
  return pix_put (row, col, text);
}

int Raster_bar (int type, int row, int col, int max, int len1, int len2)
{
  return pix_bar (type, row, col, max, len1, len2);
}


LCD Raster[] = {
#ifdef WITH_PPM  
  { name: "PPM",
    rows:  0,
    cols:  0,
    xres:  0,
    yres:  0,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 | BAR_T,
    gpos:  0,
    init:  Raster_init,
    clear: Raster_clear,
    put:   Raster_put,
    bar:   Raster_bar,
    gpo:   NULL,
    flush: PPM_flush },
#endif
#ifdef WITH_PNG
  { name: "PNG",
    rows:  0,
    cols:  0,
    xres:  0,
    yres:  0,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 | BAR_T,
    gpos:  0,
    init:  Raster_init,
    clear: Raster_clear,
    put:   Raster_put,
    bar:   Raster_bar,
    gpo:   NULL,
    flush: PNG_flush },
#endif
  { NULL }
};
