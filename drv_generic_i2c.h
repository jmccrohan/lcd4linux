/* $Id: drv_generic_i2c.h,v 1.1 2005/03/28 19:39:23 reinelt Exp $
 *
 * generic driver helper for i2c displays
 *
 * Copyright (C) 2005 Luis F. Correia <luis.f.correia@seg-social.pt>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_generic_i2c.h,v $
 * Revision 1.1  2005/03/28 19:39:23  reinelt
 * HD44780/I2C patch from Luis merged (still does not work for me)
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * int drv_generic_i2c_open (void)
 *   reads 'Port' entry from config and opens
 *   the i2c port
 *   returns 0 if ok, -1 on failure
 *
 * int drv_generic_i2c_close (void)
 *   closes i2c port
 *   returns 0 if ok, -1 on failure
 *
 * unsigned char drv_generic_i2c_wire (char *name, char *deflt)
 *   reads wiring for one data signal from config
 *   returns 1<<bitpos or 255 on error
 *
 * void drv_generic_i2c_data (unsigned char value)
 *   put data bits on DB1..DB8
 */

#ifndef _DRV_GENERIC_I2C_H_
#define _DRV_GENERIC_I2C_H_

int           drv_generic_i2c_open  (const char *section, const char *driver);
int           drv_generic_i2c_close (void);
unsigned char drv_generic_i2c_wire  (const char *name, const char *deflt);
void          drv_generic_i2c_data  (const unsigned char data);

#endif
