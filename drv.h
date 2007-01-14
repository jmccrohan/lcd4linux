/* $Id$
 * $URL$
 *
 * new framework for display drivers
 *
 * Copyright (C) 1999-2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 */

#ifndef _DRV_H_
#define _DRV_H_

typedef struct DRIVER {
    char *name;
    int (*list) (void);
    int (*init) (const char *section, const int quiet);
    int (*quit) (const int quiet);
} DRIVER;


/* output file for Raster driver
 * has to be defined here because it's referenced
 * even if the raster driver is not included! 
 */
extern char *output;

int drv_list(void);
int drv_init(const char *section, const char *driver, const int quiet);
int drv_quit(const int quiet);

#endif
