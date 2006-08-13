/* $Id: drv_generic_i2c.h,v 1.8 2006/08/13 06:46:51 reinelt Exp $
 *
 * generic driver helper for i2c displays
 *
 * Copyright (C) 2005 Luis Correia <lfcorreia@users.sf.net>
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
 * Revision 1.8  2006/08/13 06:46:51  reinelt
 * T6963 soft-timing & enhancements; indent
 *
 * Revision 1.7  2006/07/31 03:48:09  reinelt
 * preparations for scrolling
 *
 * Revision 1.6  2006/02/27 06:15:55  reinelt
 * indent...
 *
 * Revision 1.5  2006/02/25 13:36:33  geronet
 * updated indent.sh, applied coding style
 *
 * Revision 1.4  2005/05/31 21:32:00  lfcorreia
 * fix my email address
 *
 * Revision 1.3  2005/05/31 20:42:55  lfcorreia
 * new file: lcd4linux_i2c.h
 * avoid the problems detecting the proper I2C kernel include files
 *
 * rearrange all the other autoconf stuff to remove I2C detection
 *
 * new method by Paul Kamphuis to write to the I2C device
 *
 * Revision 1.2  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
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
 *
 * void drv_generic_i2c_command(unsigned char command, unsigned char *data,unsigned char length)
 *   send command and the data to the i2c device
 * 
 */ 
    
#ifndef _DRV_GENERIC_I2C_H_
#define _DRV_GENERIC_I2C_H_
int drv_generic_i2c_open(const char *section, const char *driver);
int drv_generic_i2c_close(void);
unsigned char drv_generic_i2c_wire(const char *name, const char *deflt);
void drv_generic_i2c_data(const unsigned char data);
void drv_generic_i2c_command(const unsigned char command, const unsigned char *data, const unsigned char length);

#endif				/*  */
