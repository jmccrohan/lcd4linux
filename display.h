/* $Id: display.h,v 1.2 2000/01/16 16:58:50 reinelt Exp $
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
 * Revision 1.2  2000/01/16 16:58:50  reinelt
 * *** empty log message ***
 *
 *
 */

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#define BAR_L 1
#define BAR_R 2
#define BAR_U 4
#define BAR_D 8
#define BAR_S 256

typedef struct {
  char name[16];
  int rows;
  int cols;
  int xres;
  int yres;
  int bars;
  int (*init) (void);
  int (*clear) (void);
  int (*put) (int x, int y, char *text);
  int (*bar) (int type, int x, int y, int max, int len1, int len2);
  int (*flush) (void);
} DISPLAY;

typedef struct {
  char name[16];
  DISPLAY *Display;
} FAMILY;

int lcd_init (char *display);
int lcd_clear (void);
int lcd_put (int x, int y, char *text);
int lcd_bar (int type, int x, int y, int max, int len1, int len2);
int lcd_flush (void);

#endif
