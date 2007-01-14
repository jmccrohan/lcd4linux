/* $Id$
 * $URL$
 *
 * plugin for /proc/net/dev parsing
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
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
 * int plugin_init_netdev (void)
 *  adds functions to access /proc/net/dev
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
#include "qprintf.h"
#include "hash.h"


static HASH NetDev;
static FILE *Stream = NULL;
static char *DELIMITER = " :|\t\n";

static int parse_netdev(void)
{
    int age;
    int row, col;
    static int first_time = 1;

    /* reread every 10 msec only */
    age = hash_age(&NetDev, NULL);
    if (age > 0 && age <= 10)
	return 0;

    if (Stream == NULL)
	Stream = fopen("/proc/net/dev", "r");
    if (Stream == NULL) {
	error("fopen(/proc/net/dev) failed: %s", strerror(errno));
	return -1;
    }

    rewind(Stream);
    row = 0;

    while (!feof(Stream)) {
	char buffer[256];
	char dev[16];
	char *beg, *end;
	unsigned int len;

	if (fgets(buffer, sizeof(buffer), Stream) == NULL)
	    break;

	switch (++row) {

	case 1:
	    /* skip row 1 */
	    continue;

	case 2:
	    /* row 2 used for headers */
	    if (first_time) {
		char *RxTx = strrchr(buffer, '|');
		first_time = 0;
		col = 0;
		beg = buffer;
		while (beg) {
		    char key[32];

		    while (strchr(DELIMITER, *beg))
			beg++;
		    if ((end = strpbrk(beg, DELIMITER)) != NULL)
			*end = '\0';
		    qprintf(key, sizeof(key), "%s_%s", beg < RxTx ? "Rx" : "Tx", beg);
		    hash_set_column(&NetDev, col++, key);
		    beg = end ? end + 1 : NULL;
		}
	    }
	    continue;

	default:
	    /* fetch interface name (1st column) as key */
	    beg = buffer;
	    while (*beg && *beg == ' ')
		beg++;
	    end = beg + 1;
	    while (*end && *end != ':')
		end++;
	    len = end - beg;
	    if (len >= sizeof(dev))
		len = sizeof(dev) - 1;
	    strncpy(dev, beg, len);
	    dev[len] = '\0';

	    hash_put_delta(&NetDev, dev, buffer);
	}
    }

    return 0;
}

static void my_netdev(RESULT * result, RESULT * arg1, RESULT * arg2, RESULT * arg3)
{
    char *dev, *key;
    int delay;
    double value;

    if (parse_netdev() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    dev = R2S(arg1);
    key = R2S(arg2);
    delay = R2N(arg3);

    value = hash_get_regex(&NetDev, dev, key, delay);

    SetResult(&result, R_NUMBER, &value);
}

static void my_netdev_fast(RESULT * result, RESULT * arg1, RESULT * arg2, RESULT * arg3)
{
    char *dev, *key;
    int delay;
    double value;

    if (parse_netdev() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    dev = R2S(arg1);
    key = R2S(arg2);
    delay = R2N(arg3);

    value = hash_get_delta(&NetDev, dev, key, delay);

    SetResult(&result, R_NUMBER, &value);
}


int plugin_init_netdev(void)
{
    hash_create(&NetDev);
    hash_set_delimiter(&NetDev, " :|\t\n");

    AddFunction("netdev", 3, my_netdev);
    AddFunction("netdev::fast", 3, my_netdev_fast);
    return 0;
}

void plugin_exit_netdev(void)
{
    if (Stream != NULL) {
	fclose(Stream);
	Stream = NULL;
    }
    hash_destroy(&NetDev);
}
