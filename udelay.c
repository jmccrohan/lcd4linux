/* $Id: udelay.c,v 1.6 2001/08/08 05:40:24 reinelt Exp $
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
 * Revision 1.6  2001/08/08 05:40:24  reinelt
 *
 * renamed CLK_TCK to CLOCKS_PER_SEC
 *
 * Revision 1.5  2001/03/12 13:44:58  reinelt
 *
 * new udelay() using Time Stamp Counters
 *
 * Revision 1.4  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
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
 * void udelay_calibrate (void) (if USE_OLD_UDELAY is defined)
 *   does a binary approximation for 'loops_per_usec'
 *   should be called several times on an otherwise idle machine
 *   the maximum value should be used
 *
 * void udelay_init (void)
 *   selects delay method (gettimeofday() ord rdtsc() according
 *   to processor features
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>


#ifdef USE_OLD_UDELAY

#include <time.h>

#else

#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#ifdef HAVE_ASM_MSR_H
#include <asm/msr.h>
#endif

#endif


#include "debug.h"
#include "udelay.h"

#ifdef USE_OLD_UDELAY

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
    udelay(1000000/CLOCKS_PER_SEC);
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
    udelay(1000000/CLOCKS_PER_SEC);
    if (clock()>tick)
      loops_per_usec&=~bit;
  }
}

#else

static unsigned int ticks_per_usec=0;

static void getCPUinfo (int *hasTSC, double *MHz)
{
  int fd;
  char buffer[4096], *p;
  
  *hasTSC=0;
  *MHz=-1;
  
  fd=open("/proc/cpuinfo", O_RDONLY);
  if (fd==-1) {
    error ("open(/proc/cpuinfo) failed: %s", strerror(errno));
    return;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(/proc/cpuinfo) failed: %s", strerror(errno));
    close (fd);
    return;
  }
  close (fd);

  p=strstr(buffer, "flags");
  if (p==NULL) {
    debug ("/proc/cpuinfo has no 'flags' line");
  } else {
    p=strstr(p, "tsc");
    if (p==NULL) {
      debug ("CPU does not support Time Stamp Counter");
    } else {
      debug ("CPU supports Time Stamp Counter");
      *hasTSC=1;
    }
  }
  
  p=strstr(buffer, "cpu MHz");
  if (p==NULL) {
    debug ("/proc/cpuinfo has no 'cpu MHz' line");
  } else {
    if (sscanf(p+7, " : %lf", MHz)!=1) {
      error ("parse(/proc/cpuinfo) failed: unknown 'cpu MHz' format");
      *MHz=-1;
    } else {
      debug ("CPU runs at %f MHz", *MHz);
    }
  }

}


void udelay_init (void)
{

#ifdef HAVE_ASM_MSR_H
  
  int tsc;
  double mhz;
  
  getCPUinfo (&tsc, &mhz);
  
  if (tsc && mhz>0.0) {
    ticks_per_usec=ceil(mhz);
    debug ("using TSC delay loop, %u ticks per microsecond", ticks_per_usec);
  } else {
    ticks_per_usec=0;
    debug ("using gettimeofday() delay loop");
  }

#else
  
  debug ("lcd4linux has been compiled without asm/msr.h");
  debug ("using gettimeofday() delay loop");
  
#endif
  
}

void udelay (unsigned long usec)
{
  if (ticks_per_usec) {

    unsigned int t1, t2;
    
    usec*=ticks_per_usec;

    rdtscl(t1);
    do {
      rdtscl(t2);
    } while ((t2-t1)<usec);
    
  } else {

    struct timeval now, end;
    
    gettimeofday (&end, NULL);
    end.tv_usec+=usec;
    while (end.tv_usec>1000000) {
      end.tv_usec-=1000000;
      end.tv_sec++;
    }
    
    do {
      gettimeofday(&now, NULL);
    } while (now.tv_sec==end.tv_sec?now.tv_usec<end.tv_usec:now.tv_sec<end.tv_sec);
  }
}

#endif
