/* $Id: drv_generic_keypad.h,v 1.3 2006/02/22 15:59:39 cmay Exp $
 *
 * generic driver helper for keypads
 *
 * Copyright (C) 2006 Chris Maj <cmaj@freedomcorpse.com>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: drv_generic_keypad.h,v $
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

#ifndef _DRV_GENERIC_KEYPAD_H_
#define _DRV_GENERIC_KEYPAD_H_

#include "widget.h"

/* these functinos must be implemented by the real driver */
extern int (*drv_generic_keypad_real_press) (const int num);

/* generic functions and widget callbacks */
int drv_generic_keypad_init(const char *section, const char *driver);
int drv_generic_keypad_press(const int num);
int drv_generic_keypad_quit(void);

#endif
