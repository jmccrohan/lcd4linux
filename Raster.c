/* $Id: Raster.c,v 1.6 2000/03/26 19:03:52 reinelt Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "cfg.h"
#include "display.h"
#include "pixmap.h"

#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 )

static LCD Lcd;

static int pixel=-1;
static int pgap=0;
static int rgap=0;
static int cgap=0;
static int border=0;

static int foreground=0;
static int halfground=0;
static int background=0;

extern char* output;


int Raster_flush (void)
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
      fprintf (stderr, "Raster: malloc(%d) failed: %s\n", xsize*ysize*sizeof(*bitbuf), strerror(errno));
      return -1;
    }
  }
  
  if (rowbuf==NULL) {
    if ((rowbuf=malloc(3*xsize*sizeof(*rowbuf)))==NULL) {
      fprintf (stderr, "Raster: malloc(%d) failed: %s\n", 3*xsize*sizeof(*rowbuf), strerror(errno));
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
    fprintf (stderr, "Raster: open(%s) failed: %s\n", tmp, strerror(errno));
    return -1;
  }
  
  snprintf (buffer, sizeof(buffer), "P6\n%d %d\n255\n", xsize, ysize);
  if (write (fd, buffer, strlen(buffer))<0) {
    fprintf (stderr, "Raster: write(%s) failed: %s\n", tmp, strerror(errno));
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
      fprintf (stderr, "Raster: write(%s) failed: %s\n", tmp, strerror(errno));
      return -1;
    }
  }
  
  if (close (fd)<0) {
    fprintf (stderr, "Raster: close(%s) failed: %s\n", tmp, strerror(errno));
    return -1;
  }
  if (rename (tmp, path)<0) {
    fprintf (stderr, "Raster: close(%s) failed: %s\n", tmp, strerror(errno));
    return -1;
  }
  
  return 0;
}

int Raster_clear (void)
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
    fprintf (stderr, "Raster: no output file specified (use -o switch)\n");
    return -1;
  }
    
  if (sscanf(s=cfg_get("size")?:"20x4", "%dx%d", &cols, &rows)!=2 || rows<1 || cols<1) {
    fprintf (stderr, "Raster: bad size '%s'\n", s);
    return -1;
  }

  if (sscanf(s=cfg_get("font")?:"5x8", "%dx%d", &xres, &yres)!=2 || xres<5 || yres<7) {
    fprintf (stderr, "Raster: bad font '%s'\n", s);
    return -1;
  }

  if (sscanf(s=cfg_get("pixel")?:"4+1", "%d+%d", &pixel, &pgap)!=2 || pixel<1 || pgap<0) {
    fprintf (stderr, "Raster: bad pixel '%s'\n", s);
    return -1;
  }

  if (sscanf(s=cfg_get("gap")?:"3x3", "%dx%d", &rgap, &cgap)!=2 || rgap<0 || cgap<0) {
    fprintf (stderr, "Raster: bad gap '%s'\n", s);
    return -1;
  }

  border=atoi(cfg_get("border")?:"0");

  foreground=strtol(cfg_get("foreground")?:"000000", NULL, 16);
  halfground=strtol(cfg_get("halfground")?:"ffffff", NULL, 16);
  background=strtol(cfg_get("background")?:"ffffff", NULL, 16);

  if (pix_init (rows, cols, xres, yres)!=0) {
    fprintf (stderr, "Raster: pix_init(%d, %d, %d, %d) failed\n", rows, cols, xres, yres);
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
  { "PPM", 0, 0, 0, 0, BARS, Raster_init, Raster_clear, Raster_put, Raster_bar, Raster_flush },
  { NULL }
};
