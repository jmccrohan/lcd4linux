/* $Id: pid.c,v 1.4 2004/03/03 03:47:04 reinelt Exp $
 *
 * PID file handling
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: pid.c,v $
 * Revision 1.4  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.3  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.2  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.1  2003/08/08 08:05:23  reinelt
 * added PID file handling
 *
 */

/* 
 * exported functions:
 * 
 * int pid_init (const char *pidfile)
 *   returns 0 if PID file could be successfully created
 *   returns PID of an already running process
 *   returns -1 in case of an error

 * int pid_exit (const char *pidfile)
 *   returns 0 if PID file could be successfully deleted
 *   otherwise returns error from unlink() call
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "pid.h"
#include "qprintf.h"

int pid_init (const char *pidfile)
{
  char tmpfile[256];
  char buffer[16];
  int fd, len, pid;
  
  qprintf(tmpfile, sizeof(tmpfile), "%s.%s", pidfile, "XXXXXX");

  if ((fd=mkstemp(tmpfile))==-1) {
    error ("mkstemp(%s) failed: %s", tmpfile, strerror(errno));
    return -1;
  }
  
  if (fchmod(fd,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)==-1) {
    error ("fchmod(%s) failed: %s", tmpfile, strerror(errno));
    close(fd);
    unlink(tmpfile);
    return -1;
  }
  
  qprintf(buffer, sizeof(buffer), "%d\n", (int)getpid());
  if (write(fd, buffer, strlen(buffer))!=strlen(buffer)) {
    error ("write(%s) failed: %s", tmpfile, strerror(errno));
    close(fd);
    unlink(tmpfile);
    return -1;
  }
  close (fd);
  
  
  while (link(tmpfile, pidfile)==-1) {

    if (errno!=EEXIST) {
      error ("link(%s, %s) failed: %s", tmpfile, pidfile, strerror(errno));
      unlink(tmpfile);
      return -1;
    }

    if ((fd=open(pidfile, O_RDONLY))==-1) {
      if (errno==ENOENT) continue; // pidfile disappared
      error ("open(%s) failed: %s", pidfile, strerror(errno));
      unlink (tmpfile);
      return -1;
    }

    len=read(fd, buffer, sizeof(buffer)-1);
    if (len<0) {
      error ("read(%s) failed: %s", pidfile, strerror(errno));
      unlink (tmpfile);
      return -1;
    }
    
    buffer[len]='\0';
    if (sscanf(buffer, "%d", &pid)!=1 || pid==0) {
      error ("scan(%s) failed.", pidfile);
      unlink (tmpfile);
      return -1;
    }

    if (pid==getpid()) {
      error ("%s already locked by us. uh-oh...", pidfile);
      unlink(tmpfile);
      return 0;
    }
    
    if ((kill(pid, 0)==-1) && errno==ESRCH) {
      error ("removing stale PID file %s", pidfile);
      if (unlink(pidfile)==-1 && errno!=ENOENT) {
	error ("unlink(%s) failed: %s", pidfile, strerror(errno));
	unlink(tmpfile);
	return pid;
      }
      continue;
    }
    unlink (tmpfile);
    return pid;
  }
  
  unlink (tmpfile);
  return 0;
}


int pid_exit (const char *pidfile)
{
  return unlink(pidfile);
}
