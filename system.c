/* $Id: system.c,v 1.3 2000/03/07 11:01:34 reinelt Exp $
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
 * Revision 1.3  2000/03/07 11:01:34  reinelt
 *
 * system.c cleanup
 *
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
 * int Load (double *load1, double *load2, double *load3)
 *   sets load average during thwe last 1, 5 and 15 minutes
 *   retuns 0 if ok, -1 on error
 *
 * int Busy (double *user, double *nice, double *system, double *idle)
 *   sets percentage of CPU time spent in user processes, nice'd processes
 *   system calls and idle state
 *   returns 0 if ok, -1 on error
 *
 * int Disk (int *r, int *w);
 *   sets number of read and write accesses to all disks 
 *   returns 0 if ok, -1 on error
 *
 * int Net (int *rx, int *tx);
 *   sets number of packets received and transmitted
 *   returns 0 if ok, -1 on error
 *
 * int Sensor (int index, double *val, double *min, double *max)
 *   sets the current value of the index'th sensor and
 *   the minimum and maximum values from the config file
 *   returns 0 if ok, -1 on error
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
#include "config.h"
#include "filter.h"

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


int Load (double *load1, double *load2, double *load3)
{
  static int fd=-2;
  char buffer[16];
  static double val1=0;
  static double val2=0;
  static double val3=0;
  static time_t now=0;

  *load1=val1;
  *load2=val2;
  *load3=val3;

  if (fd==-1) return -1;
  
  if (time(NULL)==now) return 0;
  time(&now);

  if (fd==-2) {
    fd=open("/proc/loadavg", O_RDONLY);
    if (fd==-1) {
      perror ("open(/proc/loadavg) failed");
      return -1;
    }
  }
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror("lseek(/proc/loadavg) failed");
    fd=-1;
    return -1;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror("read(/proc/loadavg) failed");
    fd=-1;
    return -1;
  }
  if (sscanf(buffer, "%lf %lf %lf", &val1, &val2, &val3)<3) {
    fprintf(stderr, "scanf(/proc/loadavg) failed\n");
    fd=-1;
    return -1;
  }

  *load1=val1;
  *load2=val2;
  *load3=val3;

  return 0;
}


int Busy (double *user, double *nice, double *system, double *idle)
{
  static int fd=-2;
  char buffer[64];
  unsigned long v1, v2, v3, v4;
  double d1, d2, d3, d4, d5;

  *user=0.0;
  *nice=0.0;
  *system=0.0;
  *idle=0.0;

  if (fd==-1) return -1;

  if (fd==-2) {
    fd=open("/proc/stat", O_RDONLY);
    if (fd==-1) {
      perror ("open(proc/stat) failed");
      return -1;
    }
  }
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror ("lseek(/proc/stat) failed");
    fd=-1;
    return -1;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror ("read(/proc/stat) failed");
    fd=-1;
    return -1;
  }
  if (sscanf(buffer, "%*s %lu %lu %lu %lu\n", &v1, &v2, &v3, &v4)<4) {
    fprintf (stderr, "scanf(/proc/stat) failed\n");
    fd=-1;
    return -1;
  }

  d1=smooth("cpu_user", 500, v1);
  d2=smooth("cpu_nice", 500, v2);
  d3=smooth("cpu_sys",  500, v3);
  d4=smooth("cpu_idle", 500, v4);
  d5=d1+d2+d3+d4;
 
  if (d5!=0.0) {
    *user=(d1+d2)/d5;
    *nice=d2/d5;
    *system=d3/d5;
    *idle=d4/d5;
  }
  return 0;
}


int Disk (int *r, int *w)
{
  char buffer[4096], *p;
  static int fd=-2;
  unsigned long r1, r2, r3, r4;
  unsigned long w1, w2, w3, w4;
  
  *r=0;
  *w=0;

  if (fd==-1) return -1;
  
  if (fd==-2) {
    fd = open("/proc/stat", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      perror ("open(/proc/stat) failed");
      return -1;
    }
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror ("lseek(/proc/stat) failed");
    fd=-1;
    return -1;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror ("read(/proc/stat) failed");
    fd=-1;
    return -1;
  }
  p=strstr(buffer, "disk_rblk");
  if (p==NULL) {
    fprintf (stderr, "parse(/proc/stat) failed: no disk_rblk line\n");
    fd=-1;
    return -1;
  }
  if (sscanf(p+9, "%lu %lu %lu %lu\n", &r1, &r2, &r3, &r4)<4) {
    fprintf (stderr, "scanf(/proc/stat) failed\n");
    fd=-1;
    return -1;
  }
  p=strstr(buffer, "disk_wblk");
  if (p==NULL) {
    fprintf (stderr, "parse(/proc/stat) failed: no disk_wblk line\n");
    fd=-1;
    return -1;
  }
  if (sscanf(p+9, "%lu %lu %lu %lu\n", &w1, &w2, &w3, &w4)<4) {
    fprintf (stderr, "scanf(/proc/stat) failed\n");
    fd=-1;
    return -1;
  }
  
  *r=smooth ("disk_r", 500, r1+r2+r3+r4);
  *w=smooth ("disk_w", 500, w1+w2+w3+w4);

  return 0;
}


int Net (int *rx, int *tx)
{
  char buffer[4096], *p, *s;
  static int fd=-2;
  unsigned long pkg_rx, pkg_tx;
  
  *rx=0;
  *tx=0;

  if (fd==-1) return -1;
  
  if (fd==-2) {
    fd = open("/proc/net/dev", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      perror ("open(/proc/net/dev) failed");
      return -1;
    }
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    perror ("lseek(/proc/net/dev) failed");
    fd=-1;
    return -1;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    perror ("read(/proc/net/dev) failed");
    fd=-1;
    return -1;
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

  return 0;
}


int Sensor (int index, double *val, double *min, double *max)
{
  char buffer[32];
  double value;
  static int fd[SENSORS]={[0 ... SENSORS]=-2,};
  static char *sensor[SENSORS]={NULL,};
  static double val_buf[SENSORS]={0.0,};
  static double min_buf[SENSORS]={0.0,};
  static double max_buf[SENSORS]={0.0,};
  static time_t now[SENSORS]={0,};

  if (index<0 || index>=SENSORS) return -1;

  *val=val_buf[index];
  *min=min_buf[index];
  *max=max_buf[index];

  if (fd[index]==-1) return -1;

  if (time(NULL)==now[index]) return 0;
  time(&now[index]);

  if (fd[index]==-2) {
    snprintf(buffer, 32, "Sensor%d", index);
    sensor[index]=cfg_get(buffer);
    if (sensor[index]==NULL || *sensor[index]=='\0') {
      fprintf (stderr, "%s: no entry for '%s'\n", cfg_file(), buffer);
      fd[index]=-1;
      return -1;
    }

    snprintf(buffer, 32, "Sensor%d_min", index);
    min_buf[index]=atof(cfg_get(buffer));
    snprintf(buffer, 32, "Sensor%d_max", index);
    max_buf[index]=atof(cfg_get(buffer));
    if (max_buf[index]==0.0) max_buf[index]=100.0;
    
    fd[index]=open(sensor[index], O_RDONLY);
    if (fd[index]==-1) {
      fprintf (stderr, "open (%s) failed: %s\n", sensor[index], strerror(errno));
      return -1;
    }
  }
  if (lseek(fd[index], 0L, SEEK_SET)!=0) {
    fprintf (stderr, "lseek(%s) failed: %s\n", sensor[index], strerror(errno));
    fd[index]=-1;
    return -1;
  }
  if (read (fd[index], &buffer, sizeof(buffer)-1)==-1) {
    fprintf (stderr, "read(%s) failed: %s\n", sensor[index], strerror(errno));
    fd[index]=-1;
    return -1;
  }
  if (sscanf(buffer, "%*f %*f %lf", &value)<1) {
    fprintf (stderr, "scanf(%s) failed\n", sensor[index]);
    fd[index]=-1;
    return -1;
  }
  val_buf[index]=value;
  *val=value;
  return 0;
}


#ifdef STANDALONE

int tick, tack, tau;

void main (void)
{
  char *cfg_file="./lcd4linux.conf.sample";
  double load1, load2, load3;
  double user, nice, system, idle;
  int r, w;
  int rx, tx;
  double val, min, max;

  if (cfg_read (cfg_file)==-1)
    exit (1);

  tick=atoi(cfg_get("tick"));
  tack=atoi(cfg_get("tack"));
  tau=atoi(cfg_get("tau"));

  printf ("System    : %s\n", System());
  printf ("Release   : %s\n", Release ());
  printf ("Processor : %s\n", Processor ());
  printf ("Memory    : %d MB\n", Memory ());

  while (1) {
    
    Load (&load1, &load2, &load3);
    printf ("Load      : %f %f %f\n", load1, load2, load3);
    
    Busy (&user, &nice, &system, &idle);
    printf ("Busy      : %f %f %f %f\n", user, nice, system, idle);

    Disk (&r, &w);
    printf ("Disk      : %d %d\n", r, w);

    Net (&rx, &tx);
    printf ("Net       : %d %d\n", rx, tx);
    
    Sensor (1, &val, &min, &max);
    printf ("Sensor 1  : %f %f %f\n", val, min, max);
    
    Sensor (2, &val, &min, &max);
    printf ("Sensor 2  : %f %f %f\n", val, min, max);
    
    usleep(tack*1000);
  }

}

#endif
