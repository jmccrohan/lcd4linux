/* $Id$
 * $URL$
 *
 * plugin for uname() syscall
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
 * int plugin_init_uname (void)
 *  adds uname() functions
 *
 */


#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/utsname.h>

#include "debug.h"
#include "plugin.h"



static void my_uname(RESULT * result, RESULT * arg1)
{
    struct utsname utsbuf;
    char *key, *value;

    key = R2S(arg1);

    if (uname(&utsbuf) != 0) {
	error("uname() failed: %s", strerror(errno));
	SetResult(&result, R_STRING, "");
	return;
    }

    if (strcasecmp(key, "sysname") == 0) {
	value = utsbuf.sysname;
    } else if (strcasecmp(key, "nodename") == 0) {
	value = utsbuf.nodename;
    } else if (strcasecmp(key, "release") == 0) {
	value = utsbuf.release;
    } else if (strcasecmp(key, "version") == 0) {
	value = utsbuf.version;
    } else if (strcasecmp(key, "machine") == 0) {
	value = utsbuf.machine;
#ifdef _GNU_SOURCE
    } else if (strcasecmp(key, "domainname") == 0) {
	value = utsbuf.domainname;
#endif
    } else {
	error("uname: unknown field '%s'", key);
	value = "";
    }

    SetResult(&result, R_STRING, value);
}


int plugin_init_uname(void)
{
    AddFunction("uname", 1, my_uname);
    return 0;
}

void plugin_exit_uname(void)
{
}
