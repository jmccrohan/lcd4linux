/* $Id$
 * $URL$
 *
 * plugin for seti@home status reporting
 *
 * Copyright (C) 2004 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old seti client which is 
 * Copyright (C) 2001 Axel Ehnert <axel@ehnert.net>
 *
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
 * int plugin_init_seti (void)
 *  adds functions to access /seti/state.sah
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
#include "cfg.h"

#define SECTION   "Plugin:Seti"
#define DIRKEY    "Directory"
#define STATEFILE "state.sah"

static HASH SETI;
static int fatal = 0;

static int parse_seti(void)
{
    static char fn[256] = "";
    FILE *stream;
    int age;

    /* if a fatal error occurred, do nothing */
    if (fatal != 0)
	return -1;

    /* reread every 100 msec only */
    age = hash_age(&SETI, NULL);
    if (age > 0 && age <= 100)
	return 0;

    if (fn[0] == '\0') {
	char *dir = cfg_get(SECTION, DIRKEY, NULL);
	if (dir == NULL || *dir == '\0') {
	    error("no '%s.%s' entry from %s\n", SECTION, DIRKEY, cfg_source());
	    fatal = 1;
	    return -1;
	}
	if (strlen(dir) > sizeof(fn) - sizeof(STATEFILE) - 2) {
	    error("entry '%s.%s' too long from %s!\n", SECTION, DIRKEY, cfg_source());
	    fatal = 1;
	    free(dir);
	    return -1;
	}
	strcpy(fn, dir);
	if (fn[strlen(fn) - 1] != '/')
	    strcat(fn, "/");
	strcat(fn, STATEFILE);
	free(dir);
    }

    stream = fopen(fn, "r");
    if (stream == NULL) {
	error("fopen(%s) failed: %s", fn, strerror(errno));
	return -1;
    }

    while (!feof(stream)) {
	char buffer[256];
	char *c, *key, *val;
	fgets(buffer, sizeof(buffer), stream);
	c = strchr(buffer, '=');
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
	hash_put(&SETI, key, val);
    }

    fclose(stream);

    return 0;
}


static void my_seti(RESULT * result, RESULT * arg1)
{
    char *key, *val;

    if (parse_seti() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    key = R2S(arg1);
    val = hash_get(&SETI, key, NULL);
    if (val == NULL)
	val = "";

    SetResult(&result, R_STRING, val);
}


int plugin_init_seti(void)
{
    hash_create(&SETI);
    AddFunction("seti", 1, my_seti);
    return 0;
}


void plugin_exit_seti(void)
{
    hash_destroy(&SETI);
}
