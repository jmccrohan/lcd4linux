/* $Id: drv_generic_graphic.h,v 1.8 2005/01/18 06:30:23 reinelt Exp $
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
 *
 * $Log: drv_generic_graphic.h,v $
 * Revision 1.8  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.7  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.6  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.5  2004/06/20 10:09:55  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.4  2004/06/08 21:46:38  reinelt
 *
 * splash screen for X11 driver (and generic graphic driver)
 *
 * Revision 1.3  2004/06/01 06:45:30  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.2  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.1  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 */


#ifndef _DRV_GENERIC_GRAPHIC_H_
#define _DRV_GENERIC_GRAPHIC_H_


#include <termios.h>
#include "widget.h"


extern int DROWS, DCOLS; /* display size */
extern int LROWS, LCOLS; /* layout size */
extern int XRES,  YRES;  /* pixel width/height of one char  */

/* framebuffer */
extern unsigned char *drv_generic_graphic_FB;

/* these functions must be implemented by the real driver */
void (*drv_generic_graphic_real_blit)(const int row, const int col, const int height, const int width);

/* generic functions and widget callbacks */
int drv_generic_graphic_init      (const char *section, const char *driver);
int drv_generic_graphic_clear     (void);
int drv_generic_graphic_greet     (const char *msg1, const char *msg2);
int drv_generic_graphic_draw      (WIDGET *W);
int drv_generic_graphic_icon_draw (WIDGET *W);
int drv_generic_graphic_bar_draw  (WIDGET *W);
int drv_generic_graphic_quit      (void);

#endif
