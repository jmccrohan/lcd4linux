/* $Id: wifi.h,v 1.1 2003/11/14 05:59:37 reinelt Exp $
 *
 * WIFI specific functions
 *
 * Copyright 2003 Xavier Vello <xavier66@free.fr>
 * 
 * based on lcd4linux/isdn.c which is
 * Copyright 1999, 2000 by Michael Reinelt <reinelt@eunet.at>
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
 * $Log: wifi.h,v $
 * Revision 1.1  2003/11/14 05:59:37  reinelt
 * added wifi.c wifi.h which have been forgotten at the last checkin
 *
 */

#ifndef _WIFI_H_
#define _WIFI_H_

int Wifi (int *signal, int *link, int *noise);

#endif
