/* $Id: pid.h,v 1.1 2003/08/08 08:05:23 reinelt Exp $
 *
 * PID file handling
 *
 * Copyright 2003 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: pid.h,v $
 * Revision 1.1  2003/08/08 08:05:23  reinelt
 * added PID file handling
 *
 */

#ifndef _PID_H_
#define _PID_H_

int pid_init (const char *pidfile);
int pid_exit (const char *pidfile);

#endif
