/* $Id: drv_generic.h,v 1.1 2004/01/20 04:51:39 reinelt Exp $
 *
 * generic driver helper for text- and graphic-based displays
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_generic.h,v $
 * Revision 1.1  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 */

/* 
 *
 * exported fuctions:
 *
 * Fixme: document me!
 *
 */

#ifndef _DRV_GENERIC_H_
#define _DRV_GENERIC_H_


#include <termios.h>
#include "widget.h"


extern int DROWS, DCOLS; // display size
extern int LROWS, LCOLS; // layout size
extern int XRES,  YRES;  // pixels of one char cell
extern int CHARS;        // number of user-defineable characters


extern char *LayoutFB;
extern char *DisplayFB;


int  drv_generic_serial_open    (char *driver, char *port, speed_t speed);
int  drv_generic_serial_read    (char *string, int len);
void drv_generic_serial_write   (char *string, int len);
int  drv_generic_serial_close   (void);


int  drv_generic_text_init      (char *Name);
void drv_generic_text_resizeFB  (int rows, int cols);
int  drv_generic_text_draw_text (WIDGET *W, int goto_len, 
				 void (*drv_goto)(int row, int col), 
				 void (*drv_write)(char *buffer, int len));


int  drv_generic_quit           (void);


#endif
