/* $Id: lock.c,v 1.1 2000/04/07 05:42:20 reinelt Exp $
 *
 * UUCP style locking
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
 * $Log: lock.c,v $
 * Revision 1.1  2000/04/07 05:42:20  reinelt
 *
 * UUCP style lockfiles for the serial port
 *
 */

/* 
 * exported functions:
 * 
 * pid_t lock_port (char *port)
 *   returns 0 if port could be successfully locked
 *   otherwise returns PID of the process holding the lock
 *
 * pid_t unlock_port (char *port)
 *   returns 0 if port could be successfully unlocked
 *   otherwise returns PID of the process holding the lock
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "lock.h"


pid_t lock_port (char *port)
{
  char lockfile[256];
  char tempfile[256];
  char buffer[16];
  char *p;
  int fd, len, pid;

  if ((p=strrchr (port, '/'))==NULL) 
    p=port;
  else
    p++;
  
  snprintf(lockfile, sizeof(lockfile), LOCK, p);
  snprintf(tempfile, sizeof(tempfile), LOCK, "TMP.XXXXXX");

  if (mktemp(tempfile)==NULL) {
    fprintf(stderr, "mktemp(%s) failed.\n", tempfile);
    return -1;
  }
  
  if ((fd=creat(tempfile, 0664))==-1) {
    fprintf(stderr, "creat(%s) failed: %s\n", tempfile, strerror(errno));
    return -1;
  }
  
  snprintf (buffer, sizeof(buffer), "%10d\n", (int)getpid());
  if (write(fd, buffer, strlen(buffer))!=strlen(buffer)) {
    fprintf(stderr, "write(%s) failed: %s\n", tempfile, strerror(errno));
    close(fd);
    unlink(tempfile);
    return -1;
  }
  close (fd);
  

  while (link(tempfile, lockfile)==-1) {

    if (errno!=EEXIST) {
      fprintf(stderr, "link(%s, %s) failed: %s\n", tempfile, lockfile, strerror(errno));
      unlink(tempfile);
      return -1;
    }

    if ((fd=open(lockfile, O_RDONLY))==-1) {
      if (errno==ENOENT) continue; // lockfile disappared
      fprintf (stderr, "open(%s) failed: %s\n", lockfile, strerror(errno));
      unlink (tempfile);
      return -1;
    }

    len=read(fd, buffer, sizeof(buffer)-1);
    if (len<0) {
      fprintf (stderr, "read(%s) failed: %s\n", lockfile, strerror(errno));
      unlink (tempfile);
      return -1;
    }
    
    buffer[len]='\0';
    if (sscanf(buffer, "%d", &pid)!=1 || pid==0) {
      fprintf (stderr, "scan(%s) failed.\n", lockfile);
      unlink (tempfile);
      return -1;
    }

    if (pid==getpid()) {
      fprintf (stderr, "%s already locked by us. uh-oh...\n", lockfile);
      unlink(tempfile);
      return 0;
    }
    
    if ((kill(pid, 0)==-1) && errno==ESRCH) {
      fprintf (stderr, "removing stale lockfile %s\n", lockfile);
      if (unlink(lockfile)==-1 && errno!=ENOENT) {
	fprintf (stderr, "unlink(%s) failed: %s\n", lockfile, strerror(errno));
	unlink(tempfile);
	return pid;
      }
      continue;
    }
    unlink (tempfile);
    return pid;
  }
  
  unlink (tempfile);
  return 0;
}


pid_t unlock_port (char *port)
{
  char lockfile[256];
  char *p;
  
  if ((p=strrchr (port, '/'))==NULL) 
    p=port;
  else
    p++;
  
  snprintf(lockfile, sizeof(lockfile), LOCK, p);
  unlink (lockfile);
  return 0;
}
