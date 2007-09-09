/* $Id$
 * $URL$
 *
 * plugin for load average
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
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
 * exported functions:
 *
 * int plugin_init_loadavg (void)
 *  adds functions for load average
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include "debug.h"
#include "plugin.h"

#ifndef HAVE_GETLOADAVG
static int fd = -2;

int getloadavg(double loadavg[], int nelem)
{
    char buf[65], *p;
    ssize_t nread;
    int i;

    if (fd == -2)
	fd = open("/proc/loadavg", O_RDONLY);
    if (fd < 0)
	return -1;

    lseek(fd, 0, SEEK_SET);
    nread = read(fd, buf, sizeof buf - 1);

    if (nread < 0)
	return -1;
    buf[nread - 1] = '\0';

    if (nelem > 3)
	nelem = 3;
    p = buf;
    for (i = 0; i < nelem; ++i) {
	char *endp;
	loadavg[i] = strtod(p, &endp);
	if (endp == NULL || endp == p)
	    /* This should not happen.  The format of /proc/loadavg
	       must have changed.  Don't return with what we have,
	       signal an error.  */
	    return -1;
	p = endp;
    }

    return i;
}

#endif


static void my_loadavg(RESULT * result, RESULT * arg1)
{
    static int nelem = -1;
    int index, age;
    static double loadavg[3];
    static struct timeval last_value;
    struct timeval now;

    gettimeofday(&now, NULL);

    age = (now.tv_sec - last_value.tv_sec) * 1000 + (now.tv_usec - last_value.tv_usec) / 1000;
    /* reread every 10 msec only */
    if (nelem == -1 || age == 0 || age > 10) {

	nelem = getloadavg(loadavg, 3);
	if (nelem < 0) {
	    error("getloadavg() failed!");
	    SetResult(&result, R_STRING, "");
	    return;
	}
	last_value = now;
    }

    index = R2N(arg1);
    if (index < 1 || index > nelem) {
	error("loadavg(%d): index out of range!", index);
	SetResult(&result, R_STRING, "");
	return;
    }


    SetResult(&result, R_NUMBER, &(loadavg[index - 1]));
    return;

}

int plugin_init_loadavg(void)
{
    AddFunction("loadavg", 1, my_loadavg);
    return 0;
}

void plugin_exit_loadavg(void)
{
#ifndef HAVE_GETLOADAVG
    if (fd > 0)
	close(fd);
    fd = -2;
#endif
}
