/* $Id: drv_generic_serial.c,v 1.1 2004/01/20 14:26:09 reinelt Exp $
 *
 * generic driver helper for serial and usbserial displays
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_generic_serial.c,v $
 * Revision 1.1  2004/01/20 14:26:09  reinelt
 * moved drv_generic to drv_generic_serial
 *
 * Revision 1.2  2004/01/20 05:36:59  reinelt
 * moved text-display-specific stuff to drv_generic_text
 * moved all the bar stuff from drv_generic_bar to generic_text
 *
 * Revision 1.1  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 */

/* 
 *
 * exported fuctions:
 *
 * int  drv_generic_serial_open    (char *driver, char *port, speed_t speed);
 *   opens the serial port
 *
 * int  drv_generic_serial_read    (char *string, int len);
 *   reads from the serial or USB port
 *
 * void drv_generic_serial_write   (char *string, int len);
 *   writes to the serial or USB port
 *
 * int  drv_generic_serial_close   (void);
 *   closes the serial port
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "cfg.h"
#include "drv_generic_serial.h"


static char   *Driver;
static char   *Port;
static speed_t Speed;
static int     Device=-1;


#define LOCK "/var/lock/LCK..%s"


// ****************************************
// *** generic serial/USB communication ***
// ****************************************

pid_t drv_generic_serial_lock_port (char *Port)
{
  char lockfile[256];
  char tempfile[256];
  char buffer[16];
  char *port, *p;
  int fd, len, pid;

  if (strncmp(Port, "/dev/", 5)==0) {
    port=strdup(Port+5);
  } else {
    port=strdup(Port);
  }
  
  while ((p=strchr(port, '/'))!=NULL) {
    *p='_';
  }
  
  snprintf(lockfile, sizeof(lockfile), LOCK, port);
  snprintf(tempfile, sizeof(tempfile), LOCK, "TMP.XXXXXX");

  free (port);
  
  if ((fd=mkstemp(tempfile))==-1) {
    error ("mkstemp(%s) failed: %s", tempfile, strerror(errno));
    return -1;
  }
  
  if (fchmod(fd,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)==-1) {
    error ("fchmod(%s) failed: %s", tempfile, strerror(errno));
    close(fd);
    unlink(tempfile);
    return -1;
  }
  
  snprintf (buffer, sizeof(buffer), "%10d\n", (int)getpid());
  if (write(fd, buffer, strlen(buffer))!=strlen(buffer)) {
    error ("write(%s) failed: %s", tempfile, strerror(errno));
    close(fd);
    unlink(tempfile);
    return -1;
  }
  close (fd);
  
  
  while (link(tempfile, lockfile)==-1) {

    if (errno!=EEXIST) {
      error ("link(%s, %s) failed: %s", tempfile, lockfile, strerror(errno));
      unlink(tempfile);
      return -1;
    }

    if ((fd=open(lockfile, O_RDONLY))==-1) {
      if (errno==ENOENT) continue; // lockfile disappared
      error ("open(%s) failed: %s", lockfile, strerror(errno));
      unlink (tempfile);
      return -1;
    }

    len=read(fd, buffer, sizeof(buffer)-1);
    if (len<0) {
      error ("read(%s) failed: %s", lockfile, strerror(errno));
      unlink (tempfile);
      return -1;
    }
    
    buffer[len]='\0';
    if (sscanf(buffer, "%d", &pid)!=1 || pid==0) {
      error ("scan(%s) failed.", lockfile);
      unlink (tempfile);
      return -1;
    }

    if (pid==getpid()) {
      error ("%s already locked by us. uh-oh...", lockfile);
      unlink(tempfile);
      return 0;
    }
    
    if ((kill(pid, 0)==-1) && errno==ESRCH) {
      error ("removing stale lockfile %s", lockfile);
      if (unlink(lockfile)==-1 && errno!=ENOENT) {
	error ("unlink(%s) failed: %s", lockfile, strerror(errno));
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


static pid_t drv_generic_serial_unlock_port (char *Port)
{
  char lockfile[256];
  char *port, *p;
  
  if (strncmp(Port, "/dev/", 5)==0) {
    port=strdup(Port+5);
  } else {
    port=strdup(Port);
  }
  
  while ((p=strchr(port, '/'))!=NULL) {
    *p='_';
  }
  
  snprintf(lockfile, sizeof(lockfile), LOCK, port);
  unlink (lockfile);
  free (port);

  return 0;
}


int drv_generic_serial_open (char *section, char *driver)
{
  int i, fd;
  pid_t pid;
  struct termios portset;
  
  Driver = driver;

  Port=cfg_get(section, "Port", NULL);
  if (Port==NULL || *Port=='\0') {
    error ("%s: no '%s.Port' entry from %s", Driver, section, cfg_source());
    return -1;
  }
  
  if (cfg_number(section, "Speed", 19200, 1200, 38400, &i)<0) return -1;
  switch (i) {
  case 1200:  Speed = B1200;  break;
  case 2400:  Speed = B2400;  break;
  case 4800:  Speed = B4800;  break;
  case 9600:  Speed = B9600;  break;
  case 19200: Speed = B19200; break;
  case 38400: Speed = B38400; break;
  default:
    error ("%s: unsupported speed '%d' from %s", Driver, i, cfg_source());
    return -1;
  }    
  
  info ("%s: using port '%s' at %d baud", Driver, Port, i);
  
  if ((pid=drv_generic_serial_lock_port(Port))!=0) {
    if (pid==-1)
      error ("%s: port %s could not be locked", Driver, Port);
    else
      error ("%s: port %s is locked by process %d", Driver, Port, pid);
    return -1;
  }

  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("%s: open(%s) failed: %s", Driver, Port, strerror(errno));
    drv_generic_serial_unlock_port(Port);
    return -1;
  }

  if (tcgetattr(fd, &portset)==-1) {
    error ("%s: tcgetattr(%s) failed: %s", Driver, Port, strerror(errno));
    drv_generic_serial_unlock_port(Port);
    return -1;
  }

  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("%s: tcsetattr(%s) failed: %s", Driver, Port, strerror(errno));
    drv_generic_serial_unlock_port(Port);
    return -1;
  }
  
  Device=fd;
  return Device;
}


int drv_generic_serial_read (char *string, int len)
{
  int run, ret;
  
  if (Device==-1) return -1;
  for (run=0; run<10; run++) {
    ret=read (Device, string, len);
    if (ret>=0 || errno!=EAGAIN) break;
    debug ("read(): EAGAIN");
    usleep(1000);
  }
  
  if (ret<0) {
    error("%s: read(%s, %d) failed: %s", Driver, Port, len, strerror(errno));
  } else if (ret!=len) {
    error ("%s: partial read: len=%d ret=%d", Driver, len, ret);
  }
  
  return ret;
}


void drv_generic_serial_write (char *string, int len)
{
  int run, ret;
  
  if (Device==-1) return;
  for (run=0; run<10; run++) {
    ret=write (Device, string, len);
    if (ret>=0 || errno!=EAGAIN) break;
    debug ("write(): EAGAIN");
    usleep(1000);
  }
  
  if (ret<0) {
    error ("MatrixOrbital: write(%s) failed: %s", Port, strerror(errno));
  } else if (ret!=len) {
    error ("MatrixOrbital: partial write: len=%d ret=%d", len, ret);
  }
  
  return;
}


int drv_generic_serial_close (void)
{
  debug ("%s: closing port %s", Driver, Port);
  close (Device);
  drv_generic_serial_unlock_port(Port);
  return 0;
}

