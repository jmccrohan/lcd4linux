/* $Id: system.h,v 1.2 2000/03/06 06:04:06 reinelt Exp $
 *
 * system status retreivement
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
 * $Log: system.h,v $
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

char *System (void);
char *Release (void);
char *Processor (void);
int   Memory (void);
double Load (void);
double Busy (void);
int Disk (int *r, int *w);
int Net (int *r, int *w);
double Temperature (void);

#endif
