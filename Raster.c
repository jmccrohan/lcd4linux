/* $Id: Raster.c,v 1.1 2000/03/23 07:24:48 reinelt Exp $
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
 * Revision 1.1  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DISPLAY Raster[]
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "cfg.h"
#include "display.h"
#include "pixmap.h"

#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 )

static DISPLAY Display;

static int pixelsize=-1;
static int pixelgap=0;
static int rowgap=0;
static int colgap=0;
static int border=0;

static int foreground=0;
static int halfground=0;
static int background=0;

#define R(color) (0xff&((color)>>16))
#define G(color) (0xff&((color)>>8))
#define B(color) (0xff&((color)))

int Raster_flush (void)
{
  int xsize, ysize;
  int x, y, pos;
  
  xsize=2*border+Display.cols*Display.xres*(pixelsize+pixelgap);
  ysize=2*border+Display.rows*Display.yres*(pixelsize+pixelgap);
  
  printf ("P3\n");
  printf ("%d %d\n", xsize, ysize);
  printf ("255\n");

  pos=0;
  
  for (y=0; y<ysize; y++) {
    for (x=0; x<xsize; x++) {
      if (x<border || x>=xsize-border || 
	  y<border || y>=ysize-border ||
	  (y-border)%(pixelsize+pixelgap)>=pixelsize ||
	  (x-border)%(pixelsize+pixelgap)>=pixelsize) {
	pos+=printf ("%d %d %d ", R(background), G(background), B(background));
      } else {
	if (Pixmap[((y-border)/(pixelsize+pixelgap))*Display.cols*Display.xres+(x-border)/(pixelsize+pixelgap)])
	  pos+=printf ("%d %d %d ", R(foreground), G(foreground), B(foreground));
	else
	  pos+=printf ("%d %d %d ", R(halfground), G(halfground), B(halfground));
      }
      if (pos>80) {
	pos=0;
	printf ("\n");
      }
    }
  }
  return 0;
}

int Raster_clear (void)
{
  if (pix_clear()!=0) 
    return -1;

  return 0;
}

int Raster_init (DISPLAY *Self)
{
  int rows=-1;
  int cols=-1;

  rows=atoi(cfg_get("rows")?:"4");
  cols=atoi(cfg_get("columns")?:"20");

  pixelsize=atoi(cfg_get("pixelsize")?:"1");
  pixelgap=atoi(cfg_get("pixelgap")?:"0");
  rowgap=atoi(cfg_get("rowgap")?:"0");
  colgap=atoi(cfg_get("colgap")?:"0");
  border=atoi(cfg_get("border")?:"0");

  foreground=strtol(cfg_get("foreground")?:"000000", NULL, 16);
  halfground=strtol(cfg_get("halfground")?:"ffffff", NULL, 16);
  background=strtol(cfg_get("background")?:"ffffff", NULL, 16);

  if (rows<1 || cols<1) {
    fprintf (stderr, "Raster: incorrect number of rows or columns\n");
    return -1;
  }
  
  if (pixelsize<1) {
    fprintf (stderr, "Raster: incorrect pixel size\n");
    return -1;
  }

  if (pix_init (rows, cols)!=0) {
    fprintf (stderr, "Raster: pix_init(%d, %d) failed\n", rows, cols);
    return -1;
  }

  Self->rows=rows;
  Self->cols=cols;
  Display=*Self;

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


DISPLAY Raster[] = {
  { "PPM", 0, 0, XRES, YRES, BARS, Raster_init, Raster_clear, Raster_put, Raster_bar, Raster_flush },
  { "" }
};
