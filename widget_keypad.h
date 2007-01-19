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
 */


#ifndef _WIDGET_KEYPAD_H_
#define _WIDGET_KEYPAD_H_


#include "property.h"


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
    PROPERTY expression;	/* expression that delivers the value */
    KEYPADKEY key;		/* which key */
} WIDGET_KEYPAD;


extern WIDGET_CLASS Widget_Keypad;

#endif
