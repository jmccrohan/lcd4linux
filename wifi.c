/* $Id: wifi.c,v 1.4 2004/01/06 22:33:14 reinelt Exp $
 *
 * WIFI specific functions
 *
 * Copyright 2003 Xavier Vello <xavier66@free.fr>
 * 
 * based on lcd4linux/isdn.c which is
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
 * $Log: wifi.c,v $
 * Revision 1.4  2004/01/06 22:33:14  reinelt
 * Copyright statements cleaned up
 *
 * Revision 1.3  2003/12/01 07:08:51  reinelt
 *
 * Patches from Xavier:
 *  - WiFi: make interface configurable
 *  - "quiet" as an option from the config file
 *  - ignore missing "MemShared" on Linux 2.6
 *
 * Revision 1.2  2003/11/28 18:34:55  nicowallmeier
 * Minor bugfixes
 *
 * Revision 1.1  2003/11/14 05:59:37  reinelt
 * added wifi.c wifi.h which have been forgotten at the last checkin
 *
 */

/* 
 * exported functions:
 *
 * Wifi (int *signal, int *link, int *noise)
 *   returns 0 if ok, -1 if error
 *   sets *signal to signal level (which determines the rate)
 *   sets *link to link quality
 *   sets *noise to noise level (reverse of link quality)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "debug.h"
#include "wifi.h"
#include "filter.h"
#include "cfg.h"

int Wifi (int *signal, int *link, int *noise)
{
  int ws, wl, wn;
  static int fd=-2;
  char buffer[4096];  
  char *p;

  char *interface=cfg_get("Wifi.Interface","wlan0");

  *signal=0;
  *link=0;
  *noise=0;

  if (fd==-1) return -1;	
	
  if (fd==-2) {
    fd = open("/proc/net/wireless", O_RDONLY);   // the real procfs file
    //fd = open("/wireless", O_RDONLY);         // a fake file for testing
    if (fd==-1) {
      error ("open(/proc/net/wireless) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open(/proc/net/wireless)=%d", fd);
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error ("lseek(/proc/net/wireless) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }  

  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error("read(/proc/net/wireless) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  
  p=strstr(buffer, interface);
  if (p!=NULL) {
  // TODO : size of interface ?? 
    if (sscanf(p+13, "%d", &wl)!=1) { 
      error ("parse(/proc/net/wireless) failed: unknown format");
      fd=-1;
      return -1;
    }  	  
    if (sscanf(p+19, "%d", &ws)!=1) { 
      error ("parse(/proc/net/wireless) failed: unknown format");
      fd=-1;
      return -1;
    } 
    if (sscanf(p+25, "%d", &wn)!=1) { 
      error ("parse(/proc/net/wireless) failed: unknown format");
      fd=-1;
      return -1;
    }  
  } else {
    error("read(/proc/net/wireless) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }	  
  *signal=ws;
  *link=wl;
  *noise=wn;
  return 0; 
}
