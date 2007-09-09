/* $Id$
 * $URL$
 *
 * generic driver helper for serial and usbserial displays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <michael@reinelt.co.at>
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


#ifndef _DRV_GENERIC_SERIALH_
#define _DRV_GENERIC_SERIAL_H_

int drv_generic_serial_open(const char *section, const char *driver, const unsigned int flags);
int drv_generic_serial_poll(char *string, const int len);
int drv_generic_serial_read(char *string, const int len);
void drv_generic_serial_write(const char *string, const int len);
int drv_generic_serial_close(void);

#endif
