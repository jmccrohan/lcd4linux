/* $Id: drv_generic_serial.c,v 1.8 2004/03/03 04:44:16 reinelt Exp $
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
 * Revision 1.8  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.7  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.6  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.5  2004/02/04 19:10:51  reinelt
 * Crystalfontz driver nearly finished
 *
 * Revision 1.4  2004/02/01 08:05:12  reinelt
 * Crystalfontz 633 extensions (CRC checking and stuff)
 * Models table for HD44780
 * Noritake VFD BVrightness patch from Bill Paxton
 *
 * Revision 1.3  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.2  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
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
 * int drv_generic_serial_poll (unsigned char *string, int len)
 *   reads from the serial or USB port
 *   without retry
 *
 * int  drv_generic_serial_read    (unsigned char *string, int len);
 *   reads from the serial or USB port
 *   with retry
 *
 * void drv_generic_serial_write   (unsigned char *string, int len);
 *   writes to the serial or USB port
 *
 * int  drv_generic_serial_close   (void);
 *   closes the serial port
 *
 */

#include "config.h"

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
#include "qprintf.h"
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
  
  qprintf(lockfile, sizeof(lockfile), LOCK, port);
  qprintf(tempfile, sizeof(tempfile), LOCK, "TMP.XXXXXX");

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
  
  qprintf(lockfile, sizeof(lockfile), LOCK, port);
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
  
  if (cfg_number(section, "Speed", 19200, 1200, 115200, &i)<0) return -1;
  switch (i) {
  case 1200:   Speed = B1200;   break;
  case 2400:   Speed = B2400;   break;
  case 4800:   Speed = B4800;   break;
  case 9600:   Speed = B9600;   break;
  case 19200:  Speed = B19200;  break;
  case 38400:  Speed = B38400;  break;
  case 57600:  Speed = B57600;  break;
  case 115200: Speed = B115200; break;
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


int drv_generic_serial_poll (unsigned char *string, int len)
{
  int ret;
  if (Device==-1) return -1;
  ret=read (Device, string, len);
  if (ret<0 && errno!=EAGAIN) {
    error("%s: read(%s) failed: %s", Driver, Port, strerror(errno));
  }
  return ret;
}


int drv_generic_serial_read (unsigned char *string, int len)
{
  int run, ret;
  
  for (run=0; run<10; run++) {
    ret=drv_generic_serial_poll(string, len);
    if (ret>=0 || errno!=EAGAIN) break;
    info ("%s: read(%s): EAGAIN", Driver, Port);
    usleep(1000);
  }
  
  if (ret>0 && ret!=len) {
    error ("%s: partial read(%s): len=%d ret=%d", Driver, Port, len, ret);
  }
  
  return ret;
}


void drv_generic_serial_write (unsigned char *string, int len)
{
  int run, ret;
  
  if (Device==-1) return;
  for (run=0; run<10; run++) {
    ret=write (Device, string, len);
    if (ret>=0 || errno!=EAGAIN) break;
    info ("%s: write(%s): EAGAIN", Driver, Port);
    usleep(1000);
  }
  
  if (ret<0) {
    error ("%s: write(%s) failed: %s", Driver, Port, strerror(errno));
  } else if (ret!=len) {
    error ("%s: partial write(%s): len=%d ret=%d", Driver, Port, len, ret);
  }
  
  return;
}


int drv_generic_serial_close (void)
{
  info ("%s: closing port %s", Driver, Port);
  close (Device);
  drv_generic_serial_unlock_port(Port);
  free(Port);
  return 0;
}

