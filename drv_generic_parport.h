/* $Id: drv_generic_parport.h,v 1.1 2004/01/20 14:35:38 reinelt Exp $
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
 * unsigned char drv_generic_parport_wire_data (char *name, char *deflt)
 *   reads wiring for one data signal from config
 *   returns 1<<bitpos or 255 on error
 *
 * void drv_generic_parport_direction (int direction)
 *   0 - write to parport
 *   1 - read from parport
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

int           drv_generic_parport_open       (void);
int           drv_generic_parport_close      (void);
unsigned char drv_generic_parport_wire_ctrl  (char *name, unsigned char *deflt);
unsigned char drv_generic_parport_wire_data  (char *name, unsigned char *deflt);
void          drv_generic_parport_direction  (int direction);
void          drv_generic_parport_control    (unsigned char mask, unsigned char value);
void          drv_generic_parport_toggle     (unsigned char bit, int level, int delay);
void          drv_generic_parport_data       (unsigned char data);
unsigned char drv_generic_parport_read       (void);
void          drv_generic_parport_debug      (void);

#endif
