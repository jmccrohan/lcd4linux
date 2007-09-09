/* $Id$
 * $URL$
 *
 * plugin for /proc/meminfo parsing
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
 * int plugin_init_meminfo (void)
 *  adds functions to access /proc/meminfo
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


static HASH MemInfo;
static FILE *stream = NULL;

static int parse_meminfo(void)
{
    int age;

    /* reread every 10 msec only */
    age = hash_age(&MemInfo, NULL);
    if (age > 0 && age <= 10)
	return 0;

    if (stream == NULL)
	stream = fopen("/proc/meminfo", "r");
    if (stream == NULL) {
	error("fopen(/proc/meminfo) failed: %s", strerror(errno));
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
	/* skip lines that do not end with " kB" */
	if (*c == 'B' && *(c - 1) == 'k' && *(c - 2) == ' ') {
	    /* strip trailing " kB" from value */
	    *(c - 2) = '\0';
	    /* add entry to hash table */
	    hash_put(&MemInfo, key, val);
	}
    }
    return 0;
}

static void my_meminfo(RESULT * result, RESULT * arg1)
{
    char *key, *val;

    if (parse_meminfo() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    key = R2S(arg1);
    val = hash_get(&MemInfo, key, NULL);
    if (val == NULL)
	val = "";

    SetResult(&result, R_STRING, val);
}


int plugin_init_meminfo(void)
{
    hash_create(&MemInfo);
    AddFunction("meminfo", 1, my_meminfo);
    return 0;
}


void plugin_exit_meminfo(void)
{
    if (stream != NULL) {
	fclose(stream);
	stream = NULL;
    }
    hash_destroy(&MemInfo);
}
