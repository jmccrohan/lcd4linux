/* $Id: pixmap.c,v 1.11 2003/09/10 14:01:53 reinelt Exp $
 *
 * generic pixmap driver
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
 * $Log: pixmap.c,v $
 * Revision 1.11  2003/09/10 14:01:53  reinelt
 * icons nearly finished\!
 *
 * Revision 1.10  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.9  2001/05/27 07:19:28  reinelt
 *
 * fixed a warning in pixmap.c
 * temporarily fixed a bug in isdn.c (ISDN_MAX_CHANNELS is no longer defined?)
 * fixed a bug in configure.in (--with-drivers=xyz did not work)
 *
 * Revision 1.8  2001/04/27 05:04:57  reinelt
 *
 * replaced OPEN_MAX with sysconf()
 * replaced mktemp() with mkstemp()
 * unlock serial port if open() fails
 *
 * Revision 1.7  2001/03/17 11:44:10  ltoetsch
 * allow more than 1 BAR_T
 *
 * Revision 1.6  2001/03/16 16:40:17  ltoetsch
 * implemented time bar
 *
 * Revision 1.5  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.4  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.3  2000/03/24 11:36:56  reinelt
 *
 * new syntax for raster configuration
 * changed XRES and YRES to be configurable
 * PPM driver works nice
 *
 * Revision 1.2  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.1  2000/03/22 15:36:21  reinelt
 *
 * added '-l' switch (list drivers)
 * generic pixmap driver added
 * X11 Framework done
 *
 */

/* 
 * exported functions:
 *
 * int pix_clear(void);
 *   clears the pixmap
 *
 * int pix_init (int rows, int cols, int XRES, int YRES);
 *   allocates & clears pixmap
 *
 * int pix_put (int row, int col, char *text);
 *   draws text into the pixmap
 *
 * int pix_bar (int type, int row, int col, int max, int len1, int len2);
 *   draws a bar into the pixmap
 *
 * void pix_icon (int ascii, char *buffer)
 *   used as the "define char" function for icons
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "display.h"
#include "bar.h"
#include "icon.h"
#include "pixmap.h"
#include "fontmap.h"
#include "debug.h"

static int ROWS=0;
static int COLS=0;
static int XRES=0;
static int YRES=0;

unsigned char *LCDpixmap=NULL;


int pix_clear(void)
{
  int i;

  for (i=0; i<ROWS*COLS; i++) {
    LCDpixmap[i]=0;
  }

  return 0;
}


int pix_init (int rows, int cols, int xres, int yres)
{
  if (rows<1 || cols<1 || xres<1 || yres<1) 
    return -1;
  
  if (LCDpixmap) 
    free (LCDpixmap);

  XRES=xres;
  YRES=yres;
  ROWS=rows*yres;
  COLS=cols*xres;

  if ((LCDpixmap=malloc (ROWS*COLS*sizeof(unsigned char)))==NULL)
    return -1;
    
  return pix_clear();
}


int pix_put (int row, int col, char *text)
{
  int c, x, y, mask;

  row*=YRES;
  col*=XRES;
  
  while (*text && col<COLS) {
    c=*(unsigned char*)text;
    if (c!='\t') {
      for (y=0; y<YRES; y++) {
	mask=1<<XRES;
	for (x=0; x<XRES; x++) {
	  mask>>=1;
	  LCDpixmap[(row+y)*COLS+col+x]=Fontmap[c][y]&mask?1:0;
	}
      }
    }
    col+=XRES;
    text++;
  }
  return 0;
}


#define N_BAR_T 10

int pix_bar (int type, int row, int col, int max, int len1, int len2)
{
  int x, y, len, rev;
  static struct {
    int row, col, len;
    int *buf;
    int init;
    time_t old;
  } vals[N_BAR_T];
  time_t now;
  int i=0;
  
  row*=YRES;
  col*=XRES;

  if (type & BAR_H) {
    if (max>COLS-col)
      max=COLS-col;
  } else {
    if (max>ROWS-row)
      max=ROWS-row;
  }
  
  if (len1<1) len1=1;
  else if (len1>max) len1=max;

  if (type == BAR_T) {
    for (i=0; i < N_BAR_T; i++) {
      if (vals[i].init == 0 && vals[i].buf == 0) {
        vals[i].buf = calloc(len2, sizeof(int));
        if (vals[i].buf == NULL) {
	  error("Couldn't allocte valbuf");
  	  vals[i].init = -1;
          return -1;
        }
        vals[i].init = 1;
        vals[i].len = len2;	
        vals[i].col = col;	
        vals[i].row = row;	
        time(&vals[i].old);
    	break;
      }
      else if (vals[i].init==1 && col==vals[i].col && row==vals[i].row)
	break;
    }
    if (i == N_BAR_T) {
      error("Too many $t, %d supported", N_BAR_T);
      return -1;
    }
  }
  else {
    if (len2<1) len2=1;
    else if (len2>max) len2=max;
  }
  
  rev=0;
  
  switch (type) {
  case BAR_L:
    len1=max-len1;
    len2=max-len2;
    rev=1;
    
  case BAR_R:
    for (y=0; y<YRES; y++) {
      len=y<YRES/2?len1:len2;
      for (x=0; x<max; x++) {
	LCDpixmap[(row+y)*COLS+col+x]=x<len?!rev:rev;
      }
    }
    break;

  case BAR_U:
    len1=max-len1;
    len2=max-len2;
    rev=1;
    
  case BAR_D:
    for (y=0; y<max; y++) {
      for (x=0; x<XRES; x++) {
	len=x<XRES/2?len1:len2;
  	LCDpixmap[(row+y)*COLS+col+x]=y<len?!rev:rev;
      }
    }
    break;

  case BAR_T:
    len1=max-len1;
    len2=vals[i].len;
    rev=1;
    time(&now);
    if (now == vals[i].old) {
      vals[i].buf[len2-1] += len1;
      vals[i].buf[len2-1] /= 2;
    }
    else {  
      for (; vals[i].old < now; vals[i].old++)
        for (x=1; x<len2; x++)
          vals[i].buf[x-1]=vals[i].buf[x];
      vals[i].buf[len2-1] = len1;
    }
    for (x=0; x<len2; x++) {
      len = vals[i].buf[x];
      for (y=0; y<max; y++) {
	// TODO: allow for drawing lines?
  	LCDpixmap[(row+y)*COLS+col+x]=y<len?!rev:rev;
      }
    }
    break;

  }
  return 0;
}


void pix_icon (int ascii, char *buffer)
{
  // we have to peek the whole screen for this particular icon,
  // and render it again

  int row, col;
  int x, y, mask;
  int c;
  
  for (row=0; row<ROWS/YRES; row++) {
    for (col=0; col<COLS/XRES; col++) {
      c=icon_peek(row, col);
      if (c!=ascii) continue;
      for (y=0; y<YRES; y++) {
	mask=1<<XRES;
	for (x=0; x<XRES; x++) {
	  mask>>=1;
	  LCDpixmap[(row*YRES+y)*COLS+col*XRES+x]=buffer[y]&mask?1:0;
	}
      }
    }
  }
  
}
