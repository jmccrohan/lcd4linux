/* $Id: pixmap.h,v 1.3 2000/03/26 19:03:52 reinelt Exp $
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
 * $Log: pixmap.h,v $
 * Revision 1.3  2000/03/26 19:03:52  reinelt
 *
 * more Pixmap renaming
 * quoting of '#' in config file
 *
 * Revision 1.2  2000/03/24 11:36:56  reinelt
 *
 * new syntax for raster configuration
 * changed XRES and YRES to be configurable
 * PPM driver works nice
 *
 * Revision 1.1  2000/03/22 15:36:21  reinelt
 *
 * added '-l' switch (list drivers)
 * generic pixmap driver added
 * X11 Framework done
 *
 */

#ifndef _PIXMAP_H_
#define _PIXMAP_H_

extern unsigned char *LCDpixmap;

int pix_clear (void);
int pix_init (int rows, int cols, int xres, int yres);
int pix_put (int row, int col, char *text);
int pix_bar (int type, int row, int col, int max, int len1, int len2);

#endif
