/* $Id: debug.c,v 1.3 2001/03/12 12:39:36 reinelt Exp $
 *
 * debug() and error() functions
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: debug.c,v $
 * Revision 1.3  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.2  2001/03/09 13:08:11  ltoetsch
 * Added Text driver
 *
 * Revision 1.1  2000/11/28 20:20:38  reinelt
 *
 * added debug.c
 * things like that should not hapen. debug.c exists for a few months now, but was never added to CVS. Shit happens....
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

void message (int level, const char *format, ...)
{
  va_list ap;
  char buffer[256];
  static int log_open=0;
  extern int debugging;
  extern int foreground;

  if (level>debugging) return;

  va_start(ap, format);
  (void) vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  
  if (foreground) {
#ifdef WITH_TEXT
    extern int curs_err(char *);
    if (!curs_err(buffer))
#endif      
    fprintf (level?stdout:stderr, "%s\n", buffer);
    return;
  }

  if (!log_open) {
    openlog ("LCD4Linux", LOG_PID, LOG_USER);
    log_open=1;
  }
  
  switch (level) {
  case 0:
    syslog (LOG_ERR, "%s", buffer);
    break;
  case 1:
    syslog (LOG_INFO, "%s", buffer);
    break;
  default:
    syslog (LOG_DEBUG, "%s", buffer);
  }
}
