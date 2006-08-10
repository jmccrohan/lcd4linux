/* $Id: udelay.c,v 1.22 2006/08/10 19:06:52 reinelt Exp $
 *
 * short delays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: udelay.c,v $
 * Revision 1.22  2006/08/10 19:06:52  reinelt
 * new 'fuzz' parameter for timings
 *
 * Revision 1.21  2005/12/12 09:08:08  reinelt
 * finally removed old udelay code path; read timing values from config
 *
 * Revision 1.20  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.19  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.18  2004/09/18 09:48:29  reinelt
 * HD44780 cleanup and prepararation for I2C backend
 * LCM-162 submodel framework
 *
 * Revision 1.17  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.16  2004/04/12 05:14:42  reinelt
 * another BIG FAT WARNING on the use of raw ports instead of ppdev
 *
 * Revision 1.15  2004/04/12 04:56:00  reinelt
 * emitted a BIG FAT WARNING if msr.h could not be found (and therefore
 * the gettimeofday() delay loop would be used)
 *
 * Revision 1.14  2003/10/12 04:46:19  reinelt
 *
 *
 * first try to integrate the Evaluator into a display driver (MatrixOrbital here)
 * small warning in processor.c fixed (thanks to Zachary Giles)
 * workaround for udelay() on alpha (no msr.h avaliable) (thanks to Zachary Giles)
 *
 * Revision 1.13  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.12  2003/07/18 04:43:14  reinelt
 * udelay: unnecessary sanity check removed
 *
 * Revision 1.11  2003/04/04 06:02:04  reinelt
 * new parallel port abstraction scheme
 *
 * Revision 1.10  2003/02/27 07:43:11  reinelt
 *
 * asm/msr.h: included hard-coded definition of rdtscl() if msr.h cannot be found.
 *
 * autoconf/automake/autoanything: switched back to 1.4. Hope it works again.
 *
 * Revision 1.9  2002/08/21 06:09:53  reinelt
 * some T6963 fixes, ndelay wrap
 *
 * Revision 1.8  2002/08/17 14:14:21  reinelt
 *
 * USBLCD fixes
 *
 * Revision 1.7  2002/04/29 11:00:28  reinelt
 *
 * added Toshiba T6963 driver
 * added ndelay() with nanosecond resolution
 *
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
 * void udelay_init (void)
 *   selects delay method (gettimeofday() ord rdtsc() according
 *   to processor features
 *
 * unsigned long timing (const char *driver, const char *section, const char *name, const int defval, const char *unit);
 *   returns a timing value from config or the default value
 *
 * void udelay (unsigned long usec)
 *   delays program execution for usec microseconds
 *   uses global variable 'loops_per_usec', which has to be set before.
 *   This function does busy-waiting! so use only for delays smaller
 *   than 10 msec
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>


#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#ifdef HAVE_ASM_MSR_H
#include <asm/msr.h>
#endif


#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"


static unsigned int ticks_per_usec = 0;


static void getCPUinfo(int *hasTSC, double *MHz)
{
    int fd;
    char buffer[4096], *p;

    *hasTSC = 0;
    *MHz = -1;

    fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd == -1) {
	error("udelay: open(/proc/cpuinfo) failed: %s", strerror(errno));
	return;
    }
    if (read(fd, &buffer, sizeof(buffer) - 1) == -1) {
	error("udelay: read(/proc/cpuinfo) failed: %s", strerror(errno));
	close(fd);
	return;
    }
    close(fd);

    p = strstr(buffer, "flags");
    if (p == NULL) {
	info("udelay: /proc/cpuinfo has no 'flags' line");
    } else {
	p = strstr(p, "tsc");
	if (p == NULL) {
	    info("udelay: CPU does not support Time Stamp Counter");
	} else {
	    info("udelay: CPU supports Time Stamp Counter");
	    *hasTSC = 1;
	}
    }

    p = strstr(buffer, "cpu MHz");
    if (p == NULL) {
	info("udelay: /proc/cpuinfo has no 'cpu MHz' line");
    } else {
	if (sscanf(p + 7, " : %lf", MHz) != 1) {
	    error("udelay: parse(/proc/cpuinfo) failed: unknown 'cpu MHz' format");
	    *MHz = -1;
	} else {
	    info("udelay: CPU runs at %f MHz", *MHz);
	}
    }

}


void udelay_init(void)
{
#ifdef HAVE_ASM_MSR_H

    int tsc;
    double mhz;

    getCPUinfo(&tsc, &mhz);

    if (tsc && mhz > 0.0) {
	ticks_per_usec = ceil(mhz);
	info("udelay: using TSC delay loop, %u ticks per microsecond", ticks_per_usec);
    } else
#else
    error("udelay: The file 'include/asm/msr.h' was missing at compile time.");
    error("udelay: Even if your CPU supports TSC, it will not be used!");
    error("udelay: You *really* should install msr.h and recompile LCD4linux!");
#endif
    {
	ticks_per_usec = 0;
	info("udelay: using gettimeofday() delay loop");
    }
}


unsigned long timing(const char *driver, const char *section, const char *name, const int defval, const char *unit)
{
    char sec[256];
    int fuzz, val;

    qprintf(sec, sizeof(sec), "%s.Timing", section);

    /* fuzz all timings by given factor */
    cfg_number(sec, "fuzz", 100, 1, -1, &fuzz);

    cfg_number(sec, name, defval, 0, -1, &val);
    val = val * fuzz / 100;
    
    if (val != defval) {
	if (fuzz != 100) {
	    info("%s: timing: %6s = %5d %s (default %d %s, fuzz %d)", driver, name, val, unit, defval, unit, fuzz);
	} else {
	    info("%s: timing: %6s = %5d %s (default %d %s)", driver, name, val, unit, defval, unit);
	}
    } else {
	info("%s: timing: %6s = %5d %s (default)", driver, name, defval, unit);
    }
    return val;
}


void ndelay(const unsigned long nsec)
{

#ifdef HAVE_ASM_MSR_H

    if (ticks_per_usec) {

	unsigned int t1, t2;
	unsigned long tsc;

	tsc = (nsec * ticks_per_usec + 999) / 1000;

	rdtscl(t1);
	do {
	    rep_nop();
	    rdtscl(t2);
	} while ((t2 - t1) < tsc);

    } else
#endif

    {
	struct timeval now, end;

	gettimeofday(&end, NULL);
	end.tv_usec += (nsec + 999) / 1000;
	while (end.tv_usec > 1000000) {
	    end.tv_usec -= 1000000;
	    end.tv_sec++;
	}

	do {
	    rep_nop();
	    gettimeofday(&now, NULL);
	} while (now.tv_sec == end.tv_sec ? now.tv_usec < end.tv_usec : now.tv_sec < end.tv_sec);
    }
}
