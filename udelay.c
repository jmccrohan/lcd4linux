/* $Id$
 * $URL$
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
typedef u_int32_t u32;
typedef u_int64_t u64;
#define __KERNEL__
#include <asm/msr.h>
#undef __KERNEL__
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
