/* $Id: display.h,v 1.15 2002/08/19 04:41:20 reinelt Exp $
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
 * Revision 1.15  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.14  2001/03/16 16:40:17  ltoetsch
 * implemented time bar
 *
 * Revision 1.13  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.12  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.11  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.10  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
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

typedef struct LCD {
  char *name;
  int rows;
  int cols;
  int xres;
  int yres;
  int bars;
  int gpos;
  int (*init) (struct LCD *Self);
  int (*clear) (void);
  int (*put) (int x, int y, char *text);
  int (*bar) (int type, int x, int y, int max, int len1, int len2);
  int (*gpo) (int num, int val);
  int (*flush) (void);
  int (*quit) (void);
} LCD;

typedef struct {
  char *name;
  LCD *Model;
} FAMILY;

int lcd_list (void);
int lcd_init (char *driver);
int lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars, int *gpos);
int lcd_clear (void);
int lcd_put (int row, int col, char *text);
int lcd_bar (int type, int row, int col, int max, int len1, int len2);
int lcd_gpo (int num, int val);
int lcd_flush (void);
int lcd_quit (void);

#endif
