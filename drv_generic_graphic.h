/* $Id$
 * $URL$
 *
 * generic driver helper for graphic displays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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


#ifndef _DRV_GENERIC_GRAPHIC_H_
#define _DRV_GENERIC_GRAPHIC_H_

#include "drv_generic.h"
#include "widget.h"
#include "rgb.h"

extern RGBA FG_COL;		/* foreground color */
extern RGBA HG_COL;		/* halfground color */
extern RGBA BG_COL;		/* background color */
extern RGBA BL_COL;		/* backlight color */

/* these functions must be implemented by the real driver */
extern void (*drv_generic_graphic_real_blit) (const int row, const int col, const int height, const int width);

/* helper function to get pixel color or gray value */
extern RGBA drv_generic_graphic_rgb(const int row, const int col);
extern unsigned char drv_generic_graphic_gray(const int row, const int col);
extern unsigned char drv_generic_graphic_black(const int row, const int col);


/* generic functions and widget callbacks */
int drv_generic_graphic_init(const char *section, const char *driver);
int drv_generic_graphic_clear(void);
int drv_generic_graphic_greet(const char *msg1, const char *msg2);
int drv_generic_graphic_draw(WIDGET * W);
int drv_generic_graphic_icon_draw(WIDGET * W);
int drv_generic_graphic_bar_draw(WIDGET * W);
int drv_generic_graphic_quit(void);

#endif
