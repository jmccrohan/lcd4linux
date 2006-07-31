/* $Id: drv_generic.h,v 1.4 2006/07/31 03:48:09 reinelt Exp $
 *
 * generic driver helper
 *
 * Copyright (C) 2006 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_generic.h,v $
 * Revision 1.4  2006/07/31 03:48:09  reinelt
 * preparations for scrolling
 *
 */


#ifndef _DRV_GENERIC_H_
#define _DRV_GENERIC_H_

/* these values are chars (text displays) or pixels (graphic displays) */

extern int LROWS, LCOLS;	/* layout size */
extern int DROWS, DCOLS;	/* display size */

extern int XRES, YRES;		/* pixel width/height of one char */

/* these function must be implemented by the generic driver */
extern void (*drv_generic_blit) (const int row, const int col, const int height, const int width);

int drv_generic_init(void);

#endif
