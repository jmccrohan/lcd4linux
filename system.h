/* $Id: system.h,v 1.9 2003/10/05 17:58:50 reinelt Exp $
 *
 * system status retreivement
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
 * $Log: system.h,v $
 * Revision 1.9  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.8  2000/07/31 10:43:44  reinelt
 *
 * some changes to support kernel-2.4 (different layout of various files in /proc)
 *
 * Revision 1.7  2000/05/21 06:20:35  reinelt
 *
 * added ppp throughput
 * token is '%t[iomt]' at the moment, but this will change in the near future
 *
 * Revision 1.6  2000/04/13 06:09:52  reinelt
 *
 * added BogoMips() to system.c (not used by now, maybe sometimes we can
 * calibrate our delay loop with this value)
 *
 * added delay loop to HD44780 driver. It seems to be quite fast now. Hopefully
 * no compiler will optimize away the delay loop!
 *
 * Revision 1.5  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.4  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 * Revision 1.3  2000/03/07 11:01:34  reinelt
 *
 * system.c cleanup
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#define SENSORS 9

char  *System (void);
char  *Release (void);
char  *Processor (void);
double BogoMips (void);
int    Memory (void);
int    Ram (int *total, int *free, int *shared, int *buffered, int *cached);
int    Load (double *load1, double *load2, double *load3);
int    Busy (double *user, double *nice, double *system, double *idle);
int    Disk (int *r, int *w);
int    Net (int *rx, int *tx, int *bytes);
int    PPP (int unit, int *rx, int *tx);
int    Sensor (int index, double *val, double *min, double *max);

#endif
