/* $Id: system.c,v 1.23 2001/03/13 08:34:15 reinelt Exp $
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
 * Revision 1.23  2001/03/13 08:34:15  reinelt
 *
 * corrected a off-by-one bug with sensors
 *
 * Revision 1.22  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.21  2001/03/09 12:14:24  reinelt
 *
 * minor cleanups
 *
 * Revision 1.20  2001/03/02 20:18:12  reinelt
 *
 * allow compile on systems without net/if_ppp.h
 *
 * Revision 1.19  2001/02/16 08:23:09  reinelt
 *
 * new token 'ic' (ISDN connected) by Carsten Nau <info@cnau.de>
 *
 * Revision 1.18  2000/11/17 10:36:23  reinelt
 *
 * fixed parsing of /proc/net/dev for 2.0 kernels
 *
 * Revision 1.17  2000/10/08 09:16:40  reinelt
 *
 *
 * Linux-2.4.0-test9 changed the layout of /proc/stat (especially the disk_io line)
 * rearranged parsing of some /proc files and (hopefully) made it more robust in concerns of format changes
 *
 * Revision 1.16  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.15  2000/08/09 11:03:07  reinelt
 *
 * fixed a bug in system.c where the format of /proc/net/dev was not correctly
 * detected and parsed with different kernels
 *
 * Revision 1.14  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.13  2000/07/31 10:43:44  reinelt
 *
 * some changes to support kernel-2.4 (different layout of various files in /proc)
 *
 * Revision 1.12  2000/05/21 06:20:35  reinelt
 *
 * added ppp throughput
 * token is '%t[iomt]' at the moment, but this will change in the near future
 *
 * Revision 1.11  2000/04/15 11:56:35  reinelt
 *
 * more debug messages
 *
 * Revision 1.10  2000/04/13 06:09:52  reinelt
 *
 * added BogoMips() to system.c (not used by now, maybe sometimes we can
 * calibrate our delay loop with this value)
 *
 * added delay loop to HD44780 driver. It seems to be quite fast now. Hopefully
 * no compiler will optimize away the delay loop!
 *
 * Revision 1.9  2000/03/28 07:22:15  reinelt
 *
 * version 0.95 released
 * X11 driver up and running
 * minor bugs fixed
 *
 * Revision 1.8  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.7  2000/03/18 08:07:04  reinelt
 *
 * vertical bars implemented
 * bar compaction improved
 * memory information implemented
 *
 * Revision 1.6  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.5  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 * Revision 1.4  2000/03/10 10:49:53  reinelt
 *
 * MatrixOrbital driver finished
 *
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
 * double BogoMips (void);
 *   returns BogoMips from /proc/cpuinfo
 *
 * int   Memory (void);
 *   returns main memory (Megabytes)
 *
 * int Ram (int *total, int *free, int *shared, int *buffer, int *cached)
 *   sets various usage of ram
 *   retuns 0 if ok, -1 on error
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
 *   sets number of blocks read and written from/to all disks 
 *   returns 0 if ok, -1 on error
 *
 * int Net (int *rx, int *tx, int *bytes);
 *   sets number of packets or bytes received and transmitted
 *   *bytes ist set to 0 if rx/tx are packets
 *   *bytes ist set to 1 if rx/tx are bytes
 *   returns 0 if ok, -1 on error
 *
 * int PPP (int unit, int *rx, int *tx);
 *   sets number of packets received and transmitted
 *   returns 0 if ok, -1 on error
 *
 * int Sensor (int index, double *val, double *min, double *max)
 *   sets the current value of the index'th sensor and
 *   the minimum and maximum values from the config file
 *   returns 0 if ok, -1 on error
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#ifdef HAVE_NET_IF_PPP_H
#include <net/if_ppp.h>
#else
#warning if_ppp.h not found. PPP support deactivated.
#endif

#include "debug.h"
#include "cfg.h"
#include "system.h"
#include "filter.h"


static int parse_meminfo (char *tag, char *buffer)
{
  char *p;
  unsigned long val;
  
  p=strstr(buffer, tag);
  if (p==NULL) {
    error ("parse(/proc/meminfo) failed: no '%s' line", tag);
    return -1;
  }
  if (sscanf(p+strlen(tag), "%lu", &val)!=1) {
    error ("parse(/proc/meminfo) failed: unknown '%s' format", tag);
    return -1;
  }
  return val;
}


char *System(void)
{
  static char buffer[32]="";
  struct utsname ubuf;

  if (*buffer=='\0') {
    if (uname(&ubuf)==-1) {
      error ("uname() failed: %s", strerror(errno));
      strcpy (buffer, "unknown");
    } else {
      debug ("uname(sysname)=%s", ubuf.sysname);
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
      error ("uname() failed: %s", strerror(errno));
      strcpy (buffer, "unknown");
    } else {
      debug ("uname(release)=%s", ubuf.release);
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
      error ("uname() failed: %s", strerror(errno));
      strcpy (buffer, "unknown");
    } else {
      debug ("uname(machine)=%s", ubuf.machine);
      strncpy (buffer, ubuf.machine, sizeof(buffer));
    }
  }
  return buffer;
}


double BogoMips (void)
{
  static double val=-2;
  char buffer[4096];

  if (val==-1) return -1;

  if (val==-2) {
    char *p;
    int fd=open("/proc/cpuinfo", O_RDONLY);
    if (fd==-1) {
      error ("open(/proc/cpuinfo) failed: %s", strerror(errno));
      val=-1;
      return -1;
    }
    debug ("open(proc/cpuinfo)=%d", fd);
    if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
      error ("read(/proc/cpuinfo) failed: %s", strerror(errno));
      close (fd);
      val=-1;
      return -1;
    }
    close (fd);
    p=strstr(buffer, "bogomips");
    if (p==NULL) {
      error ("parse(/proc/cpuinfo) failed: no 'bogomips' line");
      val=-1;
      return -1;
    }
    if (sscanf(p+8, " : %lf", &val)!=1) {
      error ("parse(/proc/cpuinfo) failed: unknown 'bogomips' format");
      val=-1;
      return -1;
    }
    debug ("BogoMips=%f", val);
  }
  return val;
}


int Memory(void)
{
  static int value=-1;
  struct stat buf;

  if (value==-1) {
    if (stat("/proc/kcore", &buf)==-1) {
      error ("stat(/proc/kcore) failed: %s", strerror(errno));
      value=0;
    } else {
      debug ("sizeof(/proc/kcore)=%ld bytes", buf.st_size);
      value=buf.st_size>>20;
    }
  }
  return value;
}

int Ram (int *total, int *free, int *shared, int *buffered, int *cached)
{
  static time_t now=0;
  static int fd=-2;
  static int v1=0;
  static int v2=0;
  static int v3=0;
  static int v4=0;
  static int v5=0;
  char buffer[4096];
  
  *total=v1;
  *free=v2;
  *shared=v3;
  *buffered=v4;
  *cached=v5;

  if (fd==-1) return -1;
  
  if (time(NULL)==now) return 0;
  time(&now);
  
  if (fd==-2) {
    fd = open("/proc/meminfo", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      error ("open(/proc/meminfo) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open(/proc/meminfo)=%d", fd);
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error ("lseek(/proc/meminfo) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(/proc/meminfo) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  
  if ((v1=parse_meminfo ("MemTotal:", buffer))<0) {
    fd=-1;
    return -1;
  }
  if ((v2=parse_meminfo ("MemFree:", buffer))<0) {
    fd=-1;
    return -1;
  }
  if ((v3=parse_meminfo ("MemShared:", buffer))<0) {
    fd=-1;
    return -1;
  }
  if ((v4=parse_meminfo ("Buffers:", buffer))<0) {
    fd=-1;
    return -1;
  }
  if ((v5=parse_meminfo ("Cached:", buffer))<0) {
    fd=-1;
    return -1;
  }

  *total=v1;
  *free=v2;
  *shared=v3;
  *buffered=v4;
  *cached=v5;

  return 0;

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
      error ("open(/proc/loadavg) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open(/proc/loadavg)=%d", fd);
  }

  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error("lseek(/proc/loadavg) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error("read(/proc/loadavg) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  if (sscanf(buffer, "%lf %lf %lf", &val1, &val2, &val3)!=3) {
    error ("parse(/proc/loadavg) failed: unknown format");
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
  char buffer[64], *p;
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
      error ("open(proc/stat) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open (/proc/stat)=%d", fd);
  }

  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error ("lseek(/proc/stat) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(/proc/stat) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  p=strstr(buffer, "cpu ");
  if (p==NULL) {
    error ("parse(/proc/stat) failed: no 'cpu' line");
    fd=-1;
    return -1;
  }

  if (sscanf(p+4, " %lu %lu %lu %lu", &v1, &v2, &v3, &v4)!=4) {
    error ("parse(/proc/stat) failed: unknown 'cpu' format");
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
  
  *r=0;
  *w=0;

  if (fd==-1) return -1;
  
  if (fd==-2) {
    fd = open("/proc/stat", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      error ("open(/proc/stat) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open (/proc/stat)=%d", fd);
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error ("lseek(/proc/stat) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(/proc/stat) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  p=strstr(buffer, "disk_io:");
  if (p!=NULL) {
    unsigned long rsum=0, wsum=0;
    p+=8; while (*p==' ') p++;
    while (*p && *p!='\n') {
      int i, n;
      unsigned long dummy;
      unsigned long rblk, wblk;
      i=sscanf(p, "(%lu,%lu):(%lu,%lu,%lu,%lu,%lu)%n", 
	       &dummy, &dummy, &dummy, &dummy, &rblk, &dummy, &wblk, &n);
      if (n==0 || i!= 7) {
	i=sscanf(p, "(%lu,%lu):(%lu,%lu,%lu,%lu)%n",
		 &dummy, &dummy, &dummy, &rblk, &dummy, &wblk, &n)+1;
      }
      if (n>0 && i==7) {
	rsum+=rblk;
	wsum+=wblk;
	p+=n; while (*p==' ') p++;
      } else {
	error ("parse(/proc/stat) failed: unknown 'disk_io' format");
	fd=-1;
	return -1;
      }
    }
    // assume that we got the number of sectors, and a sector size of 512 bytes.
    // to get te number in kilobytes/sec, we calculate as follows:
    // kb=blocks*512/1024, which is blocks/2
    *r=smooth ("disk_r", 500, rsum/2);
    *w=smooth ("disk_w", 500, wsum/2);

  } else {

    unsigned long r1, r2, r3, r4;
    unsigned long w1, w2, w3, w4;
    p=strstr(buffer, "disk_rblk");
    if (p==NULL) {
      error ("parse(/proc/stat) failed: neither 'disk_io' nor 'disk_rblk' found");
      fd=-1;
      return -1;
    }
    if (sscanf(p+9, "%lu %lu %lu %lu", &r1, &r2, &r3, &r4)!=4) {
      error ("parse(/proc/stat) failed: unknown 'disk_rblk' format");
      fd=-1;
      return -1;
    }
    p=strstr(buffer, "disk_wblk");
    if (p==NULL) {
      error ("parse(/proc/stat) failed: no 'disk_wblk' line");
      fd=-1;
      return -1;
    }
    if (sscanf(p+9, "%lu %lu %lu %lu", &w1, &w2, &w3, &w4)!=4) {
      error ("parse(/proc/stat) failed: unknown 'disk_wblk' format");
      fd=-1;
      return -1;
    }
    *r=smooth ("disk_r", 500, r1+r2+r3+r4);
    *w=smooth ("disk_w", 500, w1+w2+w3+w4);
  }
  
  return 0;
}


int Net (int *rx, int *tx, int *bytes)
{
  char buffer[4096], *p, *s;
  static int fd=-2;
  unsigned long pkg_rx, pkg_tx;
  
  *rx=0;
  *tx=0;
  *bytes=0;

  if (fd==-1) return -1;
  
  if (fd==-2) {
    fd = open("/proc/net/dev", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      error ("open(/proc/net/dev) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open (/proc/net/dev)=%d", fd);
  }
  
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error ("lseek(/proc/net/dev) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(/proc/net/dev) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }

  pkg_rx=0;
  pkg_tx=0;
  p=buffer;
  while ((s=strsep(&p, "\n"))) {
    int n, u;
    unsigned long v[16];
    n=sscanf (s, " eth%d: %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
	      &u, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7],
	      &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]);

    if (n==17) {
      pkg_rx+=v[0];
      pkg_tx+=v[8];
      *bytes=1;
    } else if (n==12) {
      pkg_rx+=v[0];
      pkg_tx+=v[5];
      *bytes=0;
    } else if (n>0) {
      error ("parse(/proc/net/dev) failed: unknown format");
      fd=-1;
      return -1;
    }
  }

  *rx=smooth("net_rx", 500, pkg_rx);
  *tx=smooth("net_tx", 500, pkg_tx);

  return 0;
}


int PPP (int unit, int *rx, int *tx)
{
  static int fd=-2;
  char buffer[16];
  
#ifdef HAVE_NET_IF_PPP_H
  struct ifpppstatsreq req;
#endif

  *rx=0;
  *tx=0;

#ifdef HAVE_NET_IF_PPP_H

  if (fd==-1) return -1;
  
  if (fd==-2) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) {
      error ("socket() failed: %s", strerror(errno));
      return -1;
    }
    debug ("socket()=%d", fd);
  }

  memset (&req, 0, sizeof (req));
  req.stats_ptr = (caddr_t) &req.stats;
  snprintf (req.ifr__name, sizeof(req.ifr__name), "ppp%d", unit);
  
  if (ioctl(fd, SIOCGPPPSTATS, &req) < 0)
    return 0;
  
  snprintf (buffer, sizeof(buffer), "ppp%d_rx", unit);
  *rx=smooth(buffer, 500, req.stats.p.ppp_ibytes);
  snprintf (buffer, sizeof(buffer), "ppp%d_tx", unit);
  *tx=smooth(buffer, 500, req.stats.p.ppp_obytes);

#endif

  return 0;
}


int Sensor (int index, double *val, double *min, double *max)
{
  char buffer[32];
  double dummy, value;
  static int fd[SENSORS+1]={[0 ... SENSORS]=-2,};
  static char *sensor[SENSORS+1]={NULL,};
  static double val_buf[SENSORS+1]={0.0,};
  static double min_buf[SENSORS+1]={0.0,};
  static double max_buf[SENSORS+1]={0.0,};
  static time_t now[SENSORS+1]={0,};

  if (index<0 || index>SENSORS) return -1;

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
      error ("%s: no entry for '%s'", cfg_file(), buffer);
      fd[index]=-1;
      return -1;
    }

    snprintf(buffer, 32, "Sensor%d_min", index);
    min_buf[index]=atof(cfg_get(buffer)?:"0");
    *min=min_buf[index];

    snprintf(buffer, 32, "Sensor%d_max", index);
    max_buf[index]=atof(cfg_get(buffer)?:"100");
    *max=max_buf[index];
    
    fd[index]=open(sensor[index], O_RDONLY);
    if (fd[index]==-1) {
      error ("open(%s) failed: %s", sensor[index], strerror(errno));
      return -1;
    }
    debug ("open (%s)=%d", sensor[index], fd[index]);
  }

  if (lseek(fd[index], 0L, SEEK_SET)!=0) {
    error ("lseek(%s) failed: %s", sensor[index], strerror(errno));
    fd[index]=-1;
    return -1;
  }

  if (read (fd[index], &buffer, sizeof(buffer)-1)==-1) {
    error ("read(%s) failed: %s", sensor[index], strerror(errno));
    fd[index]=-1;
    return -1;
  }

  if (sscanf(buffer, "%lf %lf %lf", &dummy, &dummy, &value)!=3) {
    error ("parse(%s) failed: unknown format", sensor[index]);
    fd[index]=-1;
    return -1;
  }

  val_buf[index]=value;
  *val=value;
  return 0;
}
