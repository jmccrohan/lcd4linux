/* $Id: XWindow.c,v 1.2 2000/03/23 07:24:48 reinelt Exp $
 *
 * driver for X11
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
 * $Log: XWindow.c,v $
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
 *
 * exported fuctions:
 *
 * struct DISPLAY XWindow[]
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "cfg.h"
#include "display.h"
#include "pixmap.h"

#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 )

static DISPLAY Display;

int X_flush (void)
{
  int r, c;

  for (r=0; r<Display.rows*Display.yres; r++) {
    for (c=0; c<Display.cols*Display.xres; c++) {
      printf ("%c", Pixmap[r*Display.cols*Display.xres+c] ? '#':'.');
    }
    printf ("\n");
  }
  printf ("\n");
  return 0;
}

int X_clear (void)
{
  if (pix_clear()!=0) 
    return -1;

  if (X_flush()!=0) 
    return -1;

  return 0;
}

int X_init (DISPLAY *Self)
{
  int rows=-1;
  int cols=-1;

  rows=atoi(cfg_get("rows")?:"4");
  cols=atoi(cfg_get("columns")?:"20");

  if (rows<1 || cols<1) {
    fprintf (stderr, "X11: incorrect number of rows or columns\n");
    return -1;
  }

  if (pix_init (rows, cols)!=0) {
    fprintf (stderr, "X11: pix_init(%d, %d) failed\n", rows, cols);
    return -1;
  }

  Self->rows=rows;
  Self->cols=cols;
  Display=*Self;

  pix_clear();
  return 0;
}

int X_put (int row, int col, char *text)
{
  return pix_put (row, col, text);
}

int X_bar (int type, int row, int col, int max, int len1, int len2)
{
  return pix_bar (type, row, col, max, len1, len2);
}


DISPLAY XWindow[] = {
  { "X11", 0, 0, XRES, YRES, BARS, X_init, X_clear, X_put, X_bar, X_flush },
  { "" }
};
