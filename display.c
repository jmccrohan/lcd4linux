/* $Id: display.c,v 1.16 2000/04/28 05:19:55 reinelt Exp $
 *
 * framework for device drivers
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
 * $Log: display.c,v $
 * Revision 1.16  2000/04/28 05:19:55  reinelt
 *
 * first release of the Beckmann+Egle driver
 *
 * Revision 1.15  2000/04/12 08:05:45  reinelt
 *
 * first version of the HD44780 driver
 *
 * Revision 1.14  2000/04/03 17:31:52  reinelt
 *
 * suppress welcome message if display is smaller than 20x2
 * change lcd4linux.ppm to 32 pixel high so KDE won't stretch the icon
 *
 * Revision 1.13  2000/03/30 16:46:57  reinelt
 *
 * configure now handles '--with-x' and '--without-x' correct
 *
 * Revision 1.12  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.11  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.10  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.9  2000/03/22 15:36:21  reinelt
 *
 * added '-l' switch (list drivers)
 * generic pixmap driver added
 * X11 Framework done
 *
 * Revision 1.8  2000/03/19 08:41:28  reinelt
 *
 * documentation available! README, README.MatrixOrbital, README.Drivers
 * added Skeleton.c as a starting point for new drivers
 *
 * Revision 1.7  2000/03/18 08:07:04  reinelt
 *
 * vertical bars implemented
 * bar compaction improved
 * memory information implemented
 *
 * Revision 1.6  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.5  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.4  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 * Revision 1.3  2000/03/10 10:49:53  reinelt
 *
 * MatrixOrbital driver finished
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 */

/* 
 * exported functions:
 *
 * lcd_list (void)
 *   lists all available drivers to stdout
 *
 * lcd_init (char *driver)
 *    initializes the named driver
 *
 * lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars)
 *    queries the attributes of the selected driver
 *
 * lcd_clear ()
 *    clears the display
 *
 * int lcd_put (int row, int col, char *text)
 *    writes text at row, col
 *
 * int lcd_bar (int type, int row, int col, int max, int len1, int len2)
 *    draws a specified bar at row, col with len
 *
 * int lcd_flush (void)
 *    flushes the framebuffer to the display
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "cfg.h"
#include "display.h"

extern LCD Skeleton[];
extern LCD MatrixOrbital[];
extern LCD BeckmannEgle[];
extern LCD HD44780[];
extern LCD Raster[];
extern LCD XWindow[];

FAMILY Driver[] = {
  { "Skeleton",        Skeleton },
  { "Matrix Orbital",  MatrixOrbital },
  { "Beckmann+Egle",   BeckmannEgle },
  { "HD 44780 based",  HD44780 },
  { "Raster",          Raster },
#ifndef X_DISPLAY_MISSING
  { "X Window System", XWindow },
#endif
  { NULL }
};


static LCD *Lcd = NULL;

int lcd_list (void)
{
  int i, j;

  printf ("available display drivers:");
  
  for (i=0; Driver[i].name; i++) {
    printf ("\n   %-16s:", Driver[i].name);
    for (j=0; Driver[i].Model[j].name; j++) {
      printf (" %s", Driver[i].Model[j].name);
    }
  }
  printf ("\n");
  return 0;
}

int lcd_init (char *driver)
{
  int i, j;
  for (i=0; Driver[i].name; i++) {
    for (j=0; Driver[i].Model[j].name; j++) {
      if (strcmp (Driver[i].Model[j].name, driver)==0) {
	Lcd=&Driver[i].Model[j];
	return Lcd->init(Lcd);
      }
    }
  }
  fprintf (stderr, "lcd_init(%s) failed: no such display\n", driver);
  return -1;
}

int lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars)
{
  if (Lcd==NULL)
    return -1;
  
  if (rows) *rows=Lcd->rows;
  if(cols) *cols=Lcd->cols;
  if (xres) *xres=Lcd->xres;
  if (yres) *yres=Lcd->yres;
  if (bars) *bars=Lcd->bars;

  return 0;
}

int lcd_clear (void)
{
  return Lcd->clear();
}

int lcd_put (int row, int col, char *text)
{
  if (row<1 || row>Lcd->rows) return -1;
  if (col<1 || col>Lcd->cols) return -1;
  return Lcd->put(row-1, col-1, text);
}

int lcd_bar (int type, int row, int col, int max, int len1, int len2)
{
  if (row<1 || row>Lcd->rows) return -1;
  if (col<1 || col>Lcd->cols) return -1;
  if (!(type & (BAR_H2 | BAR_V2))) len2=len1;
  if (type & BAR_LOG) {
    len1=(double)max*log(len1+1)/log(max); 
    len2=(double)max*log(len2+1)/log(max); 
  }
  return Lcd->bar (type & BAR_HV, row-1, col-1, max, len1, len2);
}

int lcd_flush (void)
{
  return Lcd->flush();
}
