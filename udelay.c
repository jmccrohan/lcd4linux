/* $Id: udelay.c,v 1.3 2001/03/01 22:33:50 reinelt Exp $
 *
 * short delays
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
 * $Log: udelay.c,v $
 * Revision 1.3  2001/03/01 22:33:50  reinelt
 *
 * renamed Raster_flush() to PPM_flush()
 *
 * Revision 1.2  2000/07/31 10:43:44  reinelt
 *
 * some changes to support kernel-2.4 (different layout of various files in /proc)
 *
 * Revision 1.1  2000/04/15 16:56:52  reinelt
 *
 * moved delay loops to udelay.c
 * renamed -d (debugging) switch to -v (verbose)
 * new switch -d to calibrate delay loop
 * 'Delay' entry for HD44780 back again
 * delay loops will not calibrate automatically, because this will fail with hich CPU load
 *
 */

/* 
 *
 * exported fuctions:
 *
 * void udelay (unsigned long usec)
 *  delays program execution for usec microseconds
 *  uses global variable 'loops_per_usec', which has to be set before.
 *  This function does busy-waiting! so use only for delays smaller
 *  than 10 msec
 *
 * void udelay_calibrate (void)
 *   does a binary approximation for 'loops_per_usec'
 *   should be called several times on an otherwise idle machine
 *   the maximum value should be used
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "udelay.h"

unsigned long loops_per_usec;

void udelay (unsigned long usec)
{
  unsigned long loop=usec*loops_per_usec;
  
  __asm__ (".align 16\n"
	   "1:\tdecl %0\n"
	   "\tjne 1b"
	   : /* no result */ 
	   :"a" (loop));
}


/* adopted from /usr/src/linux/init/main.c */

void udelay_calibrate (void)
{
  clock_t tick;
  unsigned long bit;
  
  loops_per_usec=1;
  while (loops_per_usec<<=1) {
    tick=clock();
    while (clock()==tick);
    tick=clock();
    udelay(1000000/CLK_TCK);
    if (clock()>tick)
      break;
  }
  
  loops_per_usec>>=1;
  bit=loops_per_usec;
  while (bit>>=1) {
    loops_per_usec|=bit;
    tick=clock();
    while (clock()==tick);
    tick=clock();
    udelay(1000000/CLK_TCK);
    if (clock()>tick)
      loops_per_usec&=~bit;
  }
}
