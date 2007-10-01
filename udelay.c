/* $Id$
 * $URL$
 *
 * short delays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <michael@reinelt.co.at>
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


#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"



void udelay_init(void)
{
    info("udelay: using gettimeofday() delay loop");
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
