/* $Id: display.c,v 1.9 2000/03/22 15:36:21 reinelt Exp $
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
 * lcd_init (char *display)
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

extern DISPLAY Skeleton[];
extern DISPLAY MatrixOrbital[];
extern DISPLAY XWindow[];

FAMILY Driver[] = {
  { "Skeleton",        Skeleton },
  { "Matrix Orbital",  MatrixOrbital },
  { "X Window System", XWindow },
  { "" }
};


static DISPLAY *Display = NULL;

int lcd_list (void)
{
  int i, j;

  printf ("available display drivers:");
  
  for (i=0; Driver[i].name[0]; i++) {
    printf ("\n   %-16s:", Driver[i].name);
    for (j=0; Driver[i].Display[j].name[0]; j++) {
      printf (" %s", Driver[i].Display[j].name);
    }
  }
  printf ("\n");
  return 0;
}

int lcd_init (char *display)
{
  int i, j;
  for (i=0; Driver[i].name[0]; i++) {
    for (j=0; Driver[i].Display[j].name[0]; j++) {
      if (strcmp (Driver[i].Display[j].name, display)==0) {
	Display=&Driver[i].Display[j];
	return Display->init(Display);
      }
    }
  }
  fprintf (stderr, "lcd_init(%s) failed: no such display\n", display);
  return -1;
}

int lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars)
{
  if (Display==NULL)
    return -1;
  
  *rows=Display->rows;
  *cols=Display->cols;
  *xres=Display->xres;
  *yres=Display->yres;
  *bars=Display->bars;

  return 0;
}

int lcd_clear (void)
{
  return Display->clear();
}

int lcd_put (int row, int col, char *text)
{
  if (row<1 || row>Display->rows) return -1;
  if (col<1 || col>Display->cols) return -1;
  return Display->put(row-1, col-1, text);
}

int lcd_bar (int type, int row, int col, int max, int len1, int len2)
{
  if (row<1 || row>Display->rows) return -1;
  if (col<1 || col>Display->cols) return -1;
  if (!(type & (BAR_H2 | BAR_V2))) len2=len1;
  if (type & BAR_LOG) {
    len1=(double)max*log(len1+1)/log(max); 
    len2=(double)max*log(len2+1)/log(max); 
  }
  return Display->bar (type & BAR_HV, row-1, col-1, max, len1, len2);
}

int lcd_flush (void)
{
  return Display->flush();
}
