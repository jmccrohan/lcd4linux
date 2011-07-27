/* $Id$
 * $URL$
 *
 * plugin for /proc/cpuinfo parsing
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
 * int plugin_init_cpuinfo (void)
 *  adds functions to access /proc/cpuinfo
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"

#ifdef __MAC_OS_X_VERSION_10_3
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

static HASH CPUinfo;
static FILE *stream = NULL;

static int parse_cpuinfo(char __attribute__ ((unused)) * oid)
{
    int age;

    /* reread every second only */
    age = hash_age(&CPUinfo, NULL);
    if (age > 0 && age <= 1000)
	return 0;

#ifndef __MAC_OS_X_VERSION_10_3

    /* Linux Kernel, /proc-filesystem */

    if (stream == NULL)
	stream = fopen("/proc/cpuinfo", "r");
    if (stream == NULL) {
	error("fopen(/proc/cpuinfo) failed: %s", strerror(errno));
	return -1;
    }
    rewind(stream);
    while (!feof(stream)) {
	char buffer[256];
	char *c, *key, *val;
	fgets(buffer, sizeof(buffer), stream);
	c = strchr(buffer, ':');
	if (c == NULL)
	    continue;
	key = buffer;
	val = c + 1;
	/* strip leading blanks from key */
	while (isspace(*key))
	    *key++ = '\0';
	/* strip trailing blanks from key */
	do
	    *c = '\0';
	while (isspace(*--c));
	/* strip leading blanks from value */
	while (isspace(*val))
	    *val++ = '\0';
	/* strip trailing blanks from value */
	for (c = val; *c != '\0'; c++);
	while (isspace(*--c))
	    *c = '\0';

	/* add entry to hash table */
	hash_put(&CPUinfo, key, val);

    }

#else

    /* MACH Kernel, MacOS X */

    char val_ret[256];
    int *val;
    size_t val_len;

    if (sysctlbyname(oid, NULL, &val_len, NULL, 0) != 0) {
	error("Error %d by sysctl(%s): %s", errno, oid, strerror(errno));
	return -1;
    }
    if (val_len > sizeof(val_ret)) {
	error("Error: Result of sysctl(%s) too big (%zd > %zd)!", oid, val_len, sizeof(val_ret));
	return -1;
    }
    sysctlbyname(oid, &val_ret, &val_len, NULL, 0);
    if (val_len == sizeof(int)) {
	/* we got an integer instead of a string */
	val = (int *) val_ret;
	snprintf(val_ret, sizeof(val_ret), "%d", *val);
    }
    hash_put(&CPUinfo, oid, val_ret);
#endif

    return 0;
}


static void my_cpuinfo(RESULT * result, RESULT * arg1)
{
    char *key, *val;

    key = R2S(arg1);
    if (parse_cpuinfo(key) < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    val = hash_get(&CPUinfo, key, NULL);
    if (val == NULL)
	val = "";

    SetResult(&result, R_STRING, val);
}


int plugin_init_cpuinfo(void)
{
    hash_create(&CPUinfo);
    AddFunction("cpuinfo", 1, my_cpuinfo);
    return 0;
}

void plugin_exit_cpuinfo(void)
{
    if (stream != NULL) {
	fclose(stream);
	stream = NULL;
    }
    hash_destroy(&CPUinfo);
}
