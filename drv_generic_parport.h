/* $Id: drv_generic_parport.h,v 1.7 2004/09/18 15:58:57 reinelt Exp $
 *
 * generic driver helper for parallel port displays
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
 * $Log: drv_generic_parport.h,v $
 * Revision 1.7  2004/09/18 15:58:57  reinelt
 * even more HD44780 cleanups, hardwiring for LCM-162
 *
 * Revision 1.6  2004/09/18 08:22:59  reinelt
 * drv_generic_parport_status() to read status lines
 *
 * Revision 1.5  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.4  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.3  2004/06/20 10:09:55  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.2  2004/01/20 15:32:49  reinelt
 * first version of Next Generation HD44780 (untested! but it compiles...)
 * some cleanup in the other drivers
 *
 * Revision 1.1  2004/01/20 14:35:38  reinelt
 * drv_generic_parport added, code from parport.c
 *
 */

/* 
 *
 * exported fuctions:
 *
 * int drv_generic_parport_open (void)
 *   reads 'Port' entry from config and opens
 *   the parallel port
 *   returns 0 if ok, -1 on failure
 *
 * int drv_generic_parport_close (void)
 *   closes parallel port
 *   returns 0 if ok, -1 on failure
 *
 * unsigned char drv_generic_parport_wire_ctrl (char *name, char *deflt)
 *   reads wiring for one control signal from config
 *   returns DRV_GENERIC_PARPORT_CONTROL_* or 255 on error
 *
 * unsigned char drv_generic_parport_hardwire_ctrl (char *name)
 *   returns hardwiring for one control signal
 *   same as above, but does not read from config,
 *   but cheks the config and emits a warning that the config
 *   entry will be ignored
 *   returns DRV_GENERIC_PARPORT_CONTROL_* or 255 on error
 *
 * unsigned char drv_generic_parport_wire_data (char *name, char *deflt)
 *   reads wiring for one data signal from config
 *   returns 1<<bitpos or 255 on error
 *
 * void drv_generic_parport_direction (int direction)
 *   0 - write to parport
 *   1 - read from parport
 *
 * unsigned char drv_generic_parport_status (void)
 *   reads control lines
 *
 * void drv_generic_parport_control (unsigned char mask, unsigned char value)
 *   frobs control line and takes care of inverted signals
 *
 * void drv_generic_parport_toggle (unsigned char bit, int level, int delay)
 *   toggles the line <bit> to <level> for <delay> nanoseconds
 *
 * void drv_generic_parport_data (unsigned char value)
 *   put data bits on DB1..DB8
 *
 * unsigned char drv_generic_parport_read (void)
 *   reads a byte from the parallel port
 *
 * void drv_generic_parport_debug(void)
 *   prints status of control lines
 *
 */

#ifndef _DRV_GENERIC_PARPORT_H_
#define _DRV_GENERIC_PARPORT_H_

int           drv_generic_parport_open       (const char *section, const char *driver);
int           drv_generic_parport_close      (void);
unsigned char drv_generic_parport_wire_ctrl  (const char *name, const char *deflt);
unsigned char drv_generic_parport_wire_data  (const char *name, const char *deflt);
void          drv_generic_parport_direction  (const int direction);
unsigned char drv_generic_parport_status     (void);
void          drv_generic_parport_control    (const unsigned char mask, const unsigned char value);
void          drv_generic_parport_toggle     (const unsigned char bit, const int level, const int delay);
void          drv_generic_parport_data       (const unsigned char data);
unsigned char drv_generic_parport_read       (void);
void          drv_generic_parport_debug      (void);

#endif
