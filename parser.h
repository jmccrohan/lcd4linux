/* $Id: parser.h,v 1.1 2000/03/13 15:58:24 reinelt Exp $
 *
 * row definition parser
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
 * $Log: parser.h,v $
 * Revision 1.1  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 */

#ifndef _PARSER_H_
#define _PARSER_H_

typedef enum {
  T_PERCENT=128, T_DOLLAR,
  T_OS, T_RELEASE, T_CPU, T_RAM,
  T_LOAD_1, T_LOAD_2, T_LOAD_3, T_OVERLOAD, 
  T_CPU_USER, T_CPU_NICE, T_CPU_SYSTEM, T_CPU_BUSY, T_CPU_IDLE,
  T_DISK_READ, T_DISK_WRITE, T_DISK_TOTAL, T_DISK_MAX,
  T_NET_RX, T_NET_TX, T_NET_TOTAL, T_NET_MAX,
  T_ISDN_IN, T_ISDN_OUT, T_ISDN_TOTAL, T_ISDN_MAX,
  T_SENSOR_1, T_SENSOR_2, T_SENSOR_3, T_SENSOR_4, T_SENSOR_5, 
  T_SENSOR_6, T_SENSOR_7, T_SENSOR_8, T_SENSOR_9,
} TOKEN;

char *parse (char *string, int supported_bars);

#endif
