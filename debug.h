/* $Id: debug.h,v 1.4 2001/09/12 05:37:22 reinelt Exp $
 *
 * debug messages
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
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
 *
 * $Log: debug.h,v $
 * Revision 1.4  2001/09/12 05:37:22  reinelt
 *
 * fixed a bug in seti.c (file was never closed, lcd4linux run out of fd's
 *
 * improved socket debugging
 *
 * Revision 1.3  2001/03/14 13:19:29  ltoetsch
 * Added pop3/imap4 mail support
 *
 * Revision 1.2  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.1  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

void message (int level, const char *format, ...);

#define debug(args...) message (2, __FILE__ ": " args)
#define info(args...) message (1, args)
#define error(args...) message (0, args)

#endif
