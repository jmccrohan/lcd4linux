/* $Id$
 * $URL$
 *
 * debug() and error() functions
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
 * exported functions:
 *
 * message (level, format, ...)
 *   passes the arguments to vsprintf() and
 *   writes the resulting string either to stdout
 *   or syslog.
 *   this function should not be called directly,
 *   but the macros info(), debug() and error()
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "debug.h"

int running_foreground = 0;
int running_background = 0;

int verbose_level = 0;

void message(const int level, const char *format, ...)
{
    va_list ap;
    char buffer[256];
    static int log_open = 0;

    if (level > verbose_level)
	return;

    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    if (!running_background) {

#ifdef WITH_CURSES
	extern int curses_error(char *);
	if (!curses_error(buffer))
#endif
	    fprintf(level ? stdout : stderr, "%s\n", buffer);
    }

    if (running_foreground)
	return;

    if (!log_open) {
	openlog("LCD4Linux", LOG_PID, LOG_USER);
	log_open = 1;
    }

    switch (level) {
    case 0:
	syslog(LOG_ERR, "%s", buffer);
	break;
    case 1:
	syslog(LOG_INFO, "%s", buffer);
	break;
    default:
	syslog(LOG_DEBUG, "%s", buffer);
    }
}
