/* $Id: display.h,v 1.9 2000/03/25 05:50:43 reinelt Exp $
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
 * $Log: display.h,v $
 * Revision 1.9  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.8  2000/03/22 15:36:21  reinelt
 *
 * added '-l' switch (list drivers)
 * generic pixmap driver added
 * X11 Framework done
 *
 * Revision 1.7  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.6  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.5  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 * Revision 1.4  2000/03/10 10:49:53  reinelt
 *
 * MatrixOrbital driver finished
 *
 * Revision 1.3  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 * Revision 1.2  2000/01/16 16:58:50  reinelt
 * *** empty log message ***
 *
 */

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#define BAR_L   (1<<0)
#define BAR_R   (1<<1)
#define BAR_U   (1<<2)
#define BAR_D   (1<<3)
#define BAR_H2  (1<<4)
#define BAR_V2  (1<<5)
#define BAR_LOG (1<<6)

#define BAR_H (BAR_L | BAR_R)
#define BAR_V (BAR_U | BAR_D)
#define BAR_HV (BAR_H | BAR_V)

typedef struct DISPLAY {
  char *name;
  int rows;
  int cols;
  int xres;
  int yres;
  int bars;
  int (*init) (struct DISPLAY *Self);
  int (*clear) (void);
  int (*put) (int x, int y, char *text);
  int (*bar) (int type, int x, int y, int max, int len1, int len2);
  int (*flush) (void);
} DISPLAY;

typedef struct {
  char *name;
  DISPLAY *Display;
} FAMILY;

int lcd_list (void);
int lcd_init (char *display);
int lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars);
int lcd_clear (void);
int lcd_put (int row, int col, char *text);
int lcd_bar (int type, int row, int col, int max, int len1, int len2);
int lcd_flush (void);

#endif
