/* $Id: seti.c,v 1.11 2004/01/06 22:33:14 reinelt Exp $
 *
 * seti@home specific functions
 *
 * Copyright 2001 Axel Ehnert <axel@ehnert.net>
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
 * $Log: seti.c,v $
 * Revision 1.11  2004/01/06 22:33:14  reinelt
 * Copyright statements cleaned up
 *
 * Revision 1.10  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.9  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.8  2003/06/13 05:11:11  reinelt
 * error message cosmetics
 *
 * Revision 1.7  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.6  2001/09/12 05:37:22  reinelt
 *
 * fixed a bug in seti.c (file was never closed, lcd4linux run out of fd's
 *
 * improved socket debugging
 *
 * Revision 1.5  2001/08/05 17:13:29  reinelt
 *
 * cleaned up inlude of sys/time.h and time.h
 *
 * Revision 1.4  2001/03/08 09:02:04  reinelt
 *
 * seti client cleanup
 *
 * Revision 1.3  2001/02/19 00:15:46  reinelt
 *
 * integrated mail and seti client
 * major rewrite of parser and tokenizer to support double-byte tokens
 *
 * Revision 1.2  2001/02/18 21:16:06  reinelt
 * *** empty log message ***
 *
 * Revision 1.1  2001/02/18 21:15:15  reinelt
 *
 * added setiathome client
 *
 */

/* 
 * exported functions:
 *
 * Seti (int *perc, int *cput)
 *   returns 0 if ok, -1 if error
 *   sets *perc to the percentage completed by seti@home client
 *   sets *perc to the cpu time used
 *
 */


#define STATEFILE "state.sah"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "cfg.h"
#include "debug.h"
#include "seti.h"


int Seti (double *perc, double *cput)
{
  static char fn[256];
  static time_t now=0;
  static int fd=-2;
  static double v1=0;
  static double v2=0;
  char buffer[8192], *p;
  
  *perc=v1;
  *cput=v2;

  if (fd==-1) return -1;
  
  if (time(NULL)==now) return 0;
  time(&now);
  
  if (fd==-2) {
    char *dir=cfg_get("SetiDir",NULL);
    if (dir==NULL || *dir=='\0') {
      error ("no 'SetiDir' entry in %s!\n", cfg_source());
      fd=-1;
      return -1;
    }
    if (strlen(dir)>sizeof(fn)-sizeof(STATEFILE)-2) {
      error ("entry 'SetiDir' too long in %s!\n", cfg_source());
      fd=-1;
      return -1;
    }
    strcpy(fn, dir);
    strcat(fn, "/");
    strcat(fn, STATEFILE);
  }

  fd = open(fn, O_RDONLY);
  if (fd==-1) {
    error ("open(%s) failed: %s", fn, strerror(errno));
    return -1;
  }

  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(%s) failed: %s", fn, strerror(errno));
    close (fd);
    fd=-1;
    return -1;
  }

  close (fd);
  
  p=strstr(buffer, "prog=");
  if (p==NULL) {
    error ("parse(%s) failed: no 'prog=' line", fn);
    fd=-1;
    return -1;
  }
  if (sscanf(p+5, "%lf", &v1)!=1) {
    error ("parse(%s) failed: unknown 'prog=' format", fn);
    fd=-1;
    return -1;
  }

  p=strstr(buffer, "cpu=");
  if (p==NULL) {
    error ("parse(%s) failed: no 'cpu=' line", fn);
    fd=-1;
    return -1;
  }
  if (sscanf(p+4, "%lf", &v2)!=1) {
    error ("parse(%s) failed: unknown 'cpu=' format", fn);
    fd=-1;
    return -1;
  }

  *perc=v1;
  *cput=v2;

  return 0;

}
