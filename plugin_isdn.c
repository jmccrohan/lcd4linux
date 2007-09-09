/* $Id$
 * $URL$
 *
 * plugin for ISDN subsystem
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Based on the old isdn client (isdn.c) which is
 * Copyright (C) 1999, 2000 Michael Reinelt <michael@reinelt.co.at>
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
 * int plugin_init_isdn (void)
 *  adds functions to access ISDN information
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#ifdef HAVE_LINUX_ISDN_H
#include <linux/isdn.h>
#else
#warning isdn.h not found. CPS support deactivated.
#endif

#include "debug.h"
#include "plugin.h"
#include "qprintf.h"
#include "hash.h"


typedef struct {
    unsigned long in;
    unsigned long out;
} CPS;


static HASH ISDN_INFO;
static HASH ISDN_CPS;


static void hash_put_info(const char *name, const int channel, const char *val)
{
    char key[16];

    qprintf(key, sizeof(key), "%s[%d]", name, channel);
    hash_put(&ISDN_INFO, key, val);
}

static int parse_isdninfo(void)
{
    int age;
    FILE *stream;
    long flags;

    /* reread every 10 msec only */
    age = hash_age(&ISDN_INFO, NULL);
    if (age > 0 && age <= 10)
	return 0;

    /* open file */
    stream = fopen("/dev/isdninfo", "r");
    if (stream == NULL) {
	error("open(/dev/isdninfo) failed: %s", strerror(errno));
	return -1;
    }

    /* get flags */
    flags = fcntl(fileno(stream), F_GETFL);
    if (flags < 0) {
	error("fcntl(/dev/isdninfo, F_GETFL) failed: %s", strerror(errno));
	return -1;
    }

    /* set O_NONBLOCK */
    if (fcntl(fileno(stream), F_SETFL, flags | O_NONBLOCK) < 0) {
	error("fcntl(/dev/isdninfo, F_SETFL, O_NONBLOCK) failed: %s", strerror(errno));
	return -1;
    }

    while (!feof(stream)) {
	char buffer[4096];
	char *beg, *end;
	if (fgets(buffer, sizeof(buffer), stream) == NULL)
	    break;
	beg = strchr(buffer, ':');
	if (beg != NULL) {
	    char delim[] = " \t\n";
	    int i = 0;
	    *beg++ = '\0';
	    while (*beg && strchr(delim, *beg))
		beg++;
	    while (beg && *beg) {
		if ((end = strpbrk(beg, delim)))
		    *end = '\0';
		hash_put_info(buffer, i, beg);
		beg = end ? end + 1 : NULL;
		while (*beg && strchr(delim, *beg))
		    beg++;
		i++;
	    }
	} else {
	    error("Huh? no colon found in <%s>", buffer);
	}
    }

    fclose(stream);

    return 0;
}


static void my_isdn_info(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char key[16], *val;

    if (parse_isdninfo() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    qprintf(key, sizeof(key), "%s[%d]", R2S(arg1), (int) R2N(arg2));
    val = hash_get(&ISDN_INFO, key, NULL);
    if (val == NULL)
	val = "";
    SetResult(&result, R_STRING, val);
}


#ifdef HAVE_LINUX_ISDN_H

static void hash_put_cps(const int channel, const CPS * cps)
{
    char key[16], val[16];

    qprintf(key, sizeof(key), channel < 0 ? "i" : "i%d", channel);
    qprintf(val, sizeof(val), "%u", cps->in);
    hash_put_delta(&ISDN_CPS, key, val);

    qprintf(key, sizeof(key), channel < 0 ? "o" : "o%d", channel);
    qprintf(val, sizeof(val), "%u", cps->out);
    hash_put_delta(&ISDN_CPS, key, val);
}


static int get_cps(void)
{
    int age, i;
    static int fd = -2;
    CPS cps[ISDN_MAX_CHANNELS];
    CPS sum;

    /* reread every 10 msec only */
    age = hash_age(&ISDN_CPS, NULL);
    if (age > 0 && age <= 10)
	return 0;

    if (fd == -1)
	return -1;

    if (fd == -2) {
	fd = open("/dev/isdninfo", O_RDONLY | O_NDELAY);
	if (fd == -1) {
	    error("open(/dev/isdninfo) failed: %s", strerror(errno));
	    return -1;
	}
    }

    if (ioctl(fd, IIOCGETCPS, &cps)) {
	error("ioctl(IIOCGETCPS) failed: %s", strerror(errno));
	fd = -1;
	return -1;
    }

    sum.in = 0;
    sum.out = 0;
    for (i = 0; i < ISDN_MAX_CHANNELS; i++) {
	sum.in += cps[i].in;
	sum.out += cps[i].out;
	hash_put_cps(i, &cps[i]);
    }
    hash_put_cps(-1, &sum);

    return 0;
}


static void my_isdn_cps(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    double value;

    if (get_cps() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    value = hash_get_delta(&ISDN_CPS, R2S(arg1), NULL, R2N(arg2));
    SetResult(&result, R_NUMBER, &value);

}

#endif


int plugin_init_isdn(void)
{
    hash_create(&ISDN_INFO);
    hash_create(&ISDN_CPS);

    AddFunction("isdn::info", 2, my_isdn_info);

#ifdef HAVE_LINUX_ISDN_H
    AddFunction("isdn::cps", 2, my_isdn_cps);
#endif

    return 0;
}


void plugin_exit_isdn(void)
{
    hash_destroy(&ISDN_INFO);
    hash_destroy(&ISDN_CPS);
}
