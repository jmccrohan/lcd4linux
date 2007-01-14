/* $Id$
 * $URL$
 *
 * generic driver helper for text-based displays
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


#ifndef _DRV_GENERIC_TEXT_H_
#define _DRV_GENERIC_TEXT_H_


#include "drv_generic.h"
#include "widget.h"

extern int CHARS, CHAR0;	/* number of user-defineable characters, ASCII of first char */
extern int ICONS;		/* number of user-defineable characters reserved for icons */
extern int GOTO_COST;		/* number of bytes a goto command requires */
extern int INVALIDATE;		/* re-send a modified userdefined char? */

/* these functions must be implemented by the real driver */
extern void (*drv_generic_text_real_write) (const int row, const int col, const char *data, const int len);
extern void (*drv_generic_text_real_defchar) (const int ascii, const unsigned char *matrix);

/* generic functions and widget callbacks */
int drv_generic_text_init(const char *section, const char *driver);
int drv_generic_text_greet(const char *msg1, const char *msg2);
int drv_generic_text_draw(WIDGET * W);
int drv_generic_text_icon_init(void);
int drv_generic_text_icon_draw(WIDGET * W);
int drv_generic_text_bar_init(const int single_segments);
void drv_generic_text_bar_add_segment(const int val1, const int val2, const DIRECTION dir, const int ascii);
int drv_generic_text_bar_draw(WIDGET * W);
int drv_generic_text_quit(void);


#endif
