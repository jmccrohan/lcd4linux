/* $Id$
 * $URL$
 *
 * keypad widget handling
 *
 * Copyright (C) 2006 Chris Maj <cmaj@freedomcorpse.com>
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
 * $Log: widget_keypad.h,v $
 * Revision 1.3  2006/07/19 01:35:31  cmay
 * Renamed keypad direction names to avoid conflict with Curses library defs.
 * Added keypad support to Curses display driver.
 *
 * Revision 1.2  2006/02/25 13:36:33  geronet
 * updated indent.sh, applied coding style
 *
 * Revision 1.1  2006/02/21 05:50:34  reinelt
 * keypad support from Cris Maj
 *
 *
 */


#ifndef _WIDGET_KEYPAD_H_
#define _WIDGET_KEYPAD_H_

typedef enum {
    WIDGET_KEY_UP = 1,
    WIDGET_KEY_DOWN = 2,
    WIDGET_KEY_LEFT = 4,
    WIDGET_KEY_RIGHT = 8,
    WIDGET_KEY_CONFIRM = 16,
    WIDGET_KEY_CANCEL = 32,
    WIDGET_KEY_PRESSED = 64,
    WIDGET_KEY_RELEASED = 128
} KEYPADKEY;

typedef struct WIDGET_KEYPAD {
    char *expression;		/* expression that delivers the value */
    void *tree;			/* pre-compiled expression that delivers the value */
    int val;			/* current value of the expression */
    KEYPADKEY key;		/* which key */
} WIDGET_KEYPAD;


extern WIDGET_CLASS Widget_Keypad;

#endif
