/* $Id: system.c,v 1.2 2000/03/06 06:04:06 reinelt Exp $
 *
 * system status retreivement
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
 * $Log: system.c,v $
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 *
 */

/* 
 * exported functions:
 *
 * char *System (void);
 *   returns OS name ('Linux')
 *
 * char *Release (void);
 *   returns OS release ('2.0.38')
 * 
 * char *Processor (void);
 *   returns processor type ('i686')
 *
 * int   Memory (void);
 *   returns main memory (Megabytes)
 *
 * double Load (void);
 *   returns load average
 *
 * double Busy (void);
 *   returns CPU utilization
 *
 * int Disk (int *r, int *w);
 *   returns disk utilization
 *
 * int Net (int *r, int *w);
 *   returns network utilization
 *
 * double Temperature (void);
 *   returns temperature 
 *   a sensor must be specified with a 'temperature'-line  
 *   in the config file
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <asm/param.h>

#include "system.h"
#include "filter.h"
#include "lcd4linux.h"

char *System(void)
{
  static char buffer[32]="";
  struct utsname ubuf;

  if (*buffer=='\0') {
    if (uname(&ubuf)==-1) {
      perror ("uname() failed");
      strcpy (buffer, "unknown");
    } else {
      strncpy (buffer, ubuf.sysname, sizeof(buffer));
    }
  }
  return buffer;
}

char *Release(void)
{
  static char buffer[32]="";
  struct utsname ubuf;

  if (*buffer=='\0') {
    if (uname(&ubuf)==-1) {
      perror ("uname() failed");
      strcpy (buffer, "unknown");
    } else {
      strncpy (buffer, ubuf.release, sizeof(buffer));
    }
  }
  return buffer;
}


char *Processor(void)
{
  static char buffer[16]="";
  struct utsname ubuf;

  if (*buffer=='\0') {
    if (uname(&ubuf)==-1) {
      perror ("uname() failed");
      strcpy (buffer, "unknown");
    } else {
      strncpy (buffer, ubuf.machine, sizeof(buffer));
    }
  }
  return buffer;
}


int Memory(void)
{
  static int value=-1;
  struct stat buf;

  if (value==-1) {
    if (stat("/proc/kcore", &buf)==-1) {
      perror ("stat(/proc/kcore) failed");
      value=0;
    } else {
      value=buf.st_size>>20;
    }
  }
  return value;
}


double Load (void)
{
  static int fd=-2;
  char buffer[16];
  static double value=0;
  static time_t now=0;

  if (fd==-1) return 0;
  
  if (time(NULL)==now) return value;
  time(&now);

  if (fd==-2) {
    fd=open("/proc/loadavg", O_RDONLY);
    if (fd==-1) {
      perror ("open(/proc/loadavg) failed");
      return 0;
    }
  }
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror("lseek(/proc/loadavg) failed");
    fd=-1;
    return 0;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror("read(/proc/loadavg) failed");
    fd=-1;
    return 0;
  }
  if (sscanf(buffer, "%lf", &value)<1) {
    fprintf(stderr, "scanf(/proc/loadavg) failed\n");
    fd=-1;
    return 0;
  }
  return (value);
}


double Busy (void)
{
  static int fd=-2;
  char buffer[64];
  unsigned long v1, v2, v3, v4;
  double busy, idle;

  if (fd==-1) return 0;

  if (fd==-2) {
    fd=open("/proc/stat", O_RDONLY);
    if (fd==-1) {
      perror ("open(proc/stat) failed");
      return 0;
    }
  }
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror ("lseek(/proc/stat) failed");
    fd=-1;
    return 0;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror ("read(/proc/stat) failed");
    fd=-1;
    return 0;
  }
  if (sscanf(buffer, "%*s %lu %lu %lu %lu\n", &v1, &v2, &v3, &v4)<4) {
    fprintf (stderr, "scanf(/proc/stat) failed\n");
    fd=-1;
    return 0;
  }

  busy=smooth("cpu_busy", 500, v1+v2+v3);
  idle=smooth("cpu_idle", 500, v4);
  
  if (busy+idle==0.0)
    return 0.0;
  else
    return busy/(busy+idle);
}


int Disk (int *r, int *w)
{
  char buffer[4096], *p;
  static int fd=-2;
  unsigned long r1, r2, r3, r4;
  unsigned long w1, w2, w3, w4;
  
  *r=0;
  *w=0;

  if (fd==-1) return 0;
  
  if (fd==-2) {
    fd = open("/proc/stat", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      perror ("open(/proc/stat) failed");
      return 0;
    }
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror ("lseek(/proc/stat) failed");
    fd=-1;
    return 0;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror ("read(/proc/stat) failed");
    fd=-1;
    return 0;
  }
  p=strstr(buffer, "disk_rblk");
  if (p==NULL) {
    fprintf (stderr, "parse(/proc/stat) failed: no disk_rblk line\n");
    fd=-1;
    return 0;
  }
  if (sscanf(p+9, "%lu %lu %lu %lu\n", &r1, &r2, &r3, &r4)<4) {
    fprintf (stderr, "scanf(/proc/stat) failed\n");
    fd=-1;
    return 0;
  }
  p=strstr(buffer, "disk_wblk");
  if (p==NULL) {
    fprintf (stderr, "parse(/proc/stat) failed: no disk_wblk line\n");
    fd=-1;
    return 0;
  }
  if (sscanf(p+9, "%lu %lu %lu %lu\n", &w1, &w2, &w3, &w4)<4) {
    fprintf (stderr, "scanf(/proc/stat) failed\n");
    fd=-1;
    return 0;
  }
  
  *r=smooth ("disk_r", 500, r1+r2+r3+r4);
  *w=smooth ("disk_w", 500, w1+w2+w3+w4);

  return *r+*w;
}


int Net (int *rx, int *tx)
{
  char buffer[4096], *p, *s;
  static int fd=-2;
  unsigned long pkg_rx, pkg_tx;
  
  *rx=0;
  *tx=0;

  if (fd==-1) return 0;
  
  if (fd==-2) {
    fd = open("/proc/net/dev", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      perror ("open(/proc/net/dev) failed");
      return 0;
    }
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror ("lseek(/proc/net/dev) failed");
    fd=-1;
    return 0;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror ("read(/proc/net/dev) failed");
    fd=-1;
    return 0;
  }
  pkg_rx=0;
  pkg_tx=0;
  p=buffer;
  while ((s=strsep(&p, "\n"))) {
    unsigned long r, t;
    if (sscanf (s, " eth%*d:%*d: %ld %*d %*d %*d %*d %ld", &r, &t)==2 ||
	sscanf (s, " eth%*d: %ld %*d %*d %*d %*d %ld", &r, &t)==2) {
      pkg_rx+=r;
      pkg_tx+=t;
    }  
  }
  *rx=smooth("net_rx", 500, pkg_rx);
  *tx=smooth("net_tx", 500, pkg_tx);

  return *rx+*tx;
}


double Temperature (void)
{
  static int fd=-2;
  char buffer[32];
  static double value=0.0;
  static time_t now=0;

  if (fd==-1) return 0;
  
  if (time(NULL)==now) return value;
  time(&now);

  if (fd==-2) {
    fd=open(sensor, O_RDONLY);
    if (fd==-1) {
      fprintf (stderr, "open (%s) failed: %s\n", sensor, strerror(errno));
      return 0;
    }
  }
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    fprintf (stderr, "lseek(%s) failed: %s\n", sensor, strerror(errno));
    fd=-1;
    return 0;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    fprintf (stderr, "read(%s) failed: %s\n", sensor, strerror(errno));
    fd=-1;
    return 0;
  }
  if (sscanf(buffer, "%*f %*f %lf", &value)<1) {
    fprintf (stderr, "scanf(%s) failed\n", sensor);
    fd=-1;
    return 0;
  }
  return (value);
}
