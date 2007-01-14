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
 *
 * $Log: plugin_uname.c,v $
 * Revision 1.6  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.5  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.4  2005/01/09 10:53:24  reinelt
 * small type in plugin_uname fixed
 * new homepage lcd4linux.bulix.org
 *
 * Revision 1.3  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.2  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.1  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
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
