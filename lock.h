/* $Id: lock.h,v 1.2 2003/10/05 17:58:50 reinelt Exp $
 *
 * UUCP style port locking
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: lock.h,v $
 * Revision 1.2  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.1  2000/04/07 05:42:20  reinelt
 *
 * UUCP style lockfiles for the serial port
 *
 */

#ifndef _LOCK_H_
#define _LOCK_H_

#define LOCK "/var/lock/LCK..%s"

pid_t lock_port (char *port);
pid_t unlock_port (char *port);

#endif
