/* $Id: drv.h,v 1.2 2004/01/10 20:22:33 reinelt Exp $
 *
 * new framework for display drivers
 *
 * Copyright 1999-2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: drv.h,v $
 * Revision 1.2  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.1  2004/01/09 17:03:07  reinelt
 * initiated transfer to new driver architecture
 * new file 'drv.c' will someday replace 'display.c'
 * new file 'drv_MatrixOrbital.c' will replace 'MatrixOrbital.c'
 * due to this 'soft' transfer lcd4linux should stay usable during the switch
 * (at least I hope so)
 *
 */

#ifndef _DRV_H_
#define _DRV_H_

typedef struct DRIVER {
  char *name;
  int (*list)  (void);
  int (*init)  (char *section);
  int (*quit)  (void);
} DRIVER;


// output file for Raster driver
// has to be defined here because it's referenced
// even if the raster driver is not included!
extern char *output;

int drv_list  (void);
int drv_init  (char *section, char *driver);
int drv_quit  (void);

/*
  int drv_query (int *rows, int *cols, int *xres, int *yres, int *bars, int *icons, int *gpos);
  int drv_clear (int full);
  int drv_put   (int row, int col, char *text);
  int drv_bar   (int type, int row, int col, int max, int len1, int le2);
  int drv_icon  (int num, int seq, int row, int col);
  int drv_gpo   (int num, int val);
  int drv_flush (void);
*/

#endif
