/* $Id: drv_generic_text.h,v 1.1 2004/01/20 05:36:59 reinelt Exp $
 *
 * generic driver helper for text-based displays
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
 * $Log: drv_generic_text.h,v $
 * Revision 1.1  2004/01/20 05:36:59  reinelt
 * moved text-display-specific stuff to drv_generic_text
 * moved all the bar stuff from drv_generic_bar to generic_text
 *
 */

/* 
 *
 * exported fuctions:
 *
 * Fixme: document me!
 *
 */

#ifndef _DRV_GENERIC_TEXT_H_
#define _DRV_GENERIC_TEXT_H_


#include <termios.h>
#include "widget.h"


extern int DROWS, DCOLS; // display size
extern int LROWS, LCOLS; // layout size
extern int XRES,  YRES;  // pixels of one char cell
extern int CHARS;        // number of user-defineable characters


extern char *LayoutFB;
extern char *DisplayFB;


int  drv_generic_text_init      (char *Name);
void drv_generic_text_resizeFB  (int rows, int cols);
int  drv_generic_text_draw_text (WIDGET *W, int goto_len, 
				 void (*drv_goto)(int row, int col), 
				 void (*drv_write)(char *buffer, int len));

int  drv_generic_text_bar_init  (void);
void drv_generic_text_bar_add_segment (int val1, int val2, DIRECTION dir, int ascii);
int  drv_generic_text_draw_bar  (WIDGET *W, int goto_len, 
				 void (*drv_defchar)(int ascii, char *buffer),
				 void (*drv_goto)(int row, int col), 
				 void (*drv_write)(char *buffer, int len));

int  drv_generic_text_quit      (void);



#endif
