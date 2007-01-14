/* $Id$
 * $URL$
 *
 * generic driver helper for keypads
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
 * $Log: drv_generic_keypad.c,v $
 * Revision 1.3  2006/02/22 15:59:39  cmay
 * removed KEYPADSIZE cruft per harbaum's suggestion
 *
 * Revision 1.2  2006/02/21 15:55:59  cmay
 * removed new update function for keypad, consolidated it with draw
 *
 * Revision 1.1  2006/02/21 05:50:34  reinelt
 * keypad support from Cris Maj
 *
 *
 */

#include <stdio.h>

#include "debug.h"
#include "widget.h"
#include "widget_keypad.h"

#include "drv_generic_keypad.h"

static char *Section = NULL;
static char *Driver = NULL;

int (*drv_generic_keypad_real_press) () = NULL;

int drv_generic_keypad_init(const char *section, const char *driver)
{
    WIDGET_CLASS wc;

    Section = (char *) section;
    Driver = (char *) driver;

    /* register keypad widget */
    wc = Widget_Keypad;
    widget_register(&wc);

    return 0;
}

int drv_generic_keypad_press(const int num)
{
    WIDGET *w;
    int val = 0;

    if (drv_generic_keypad_real_press)
	val = drv_generic_keypad_real_press(num);

    w = widget_find(WIDGET_TYPE_KEYPAD, &val);

    if (w && w->class->draw)
	w->class->draw(w);

    return val;
}

int drv_generic_keypad_quit(void)
{
    return 0;
}
