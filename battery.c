/* $Id: battery.c,v 1.3 2001/08/05 17:13:29 reinelt Exp $
 *
 * APM and ACPI specific functions
 *
 * Copyright 2001 by Leo Tötsch (lt@toetsch.at)
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
 * $Log: battery.c,v $
 * Revision 1.3  2001/08/05 17:13:29  reinelt
 *
 * cleaned up inlude of sys/time.h and time.h
 *
 * Revision 1.2  2001/03/02 11:04:08  reinelt
 *
 * cosmetic cleanups (comment headers)
 *
 */

/* exported functions:
 *
 * int Battery (double *perc, int *status, double *lasting);
 *   returns:
 *     perc ... percentage of battery (0..100)
 *     status .. 0 (online), 1 (charging), 2 (discharging)
 *     lasting ... seconds, how long battery will last
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "debug.h"


int Battery (int *perc, int *status, double *dur) 
{
  static int fd = -2;
  static int apm = 0;

  if (fd==-1) return -1;

  if (fd==-2) {
    fd = open("/proc/apm", O_RDONLY | O_NDELAY);
    if (fd==-1) {
      error ("open(/proc/apm) failed: %s", strerror(errno));
      fd = open("/proc/acpi", O_RDONLY | O_NDELAY); /* FIXME */
      if (fd==-1) {
        error ("open(/proc/acpi) failed: %s", strerror(errno));
        return -1;
      }
      debug ("open(/proc/acpi)=%d", fd);
    }
    else {
      debug ("open(/proc/apm)=%d", fd);
      apm = 1;
    }
  }
  if (lseek(fd, 0L, SEEK_SET)!=0) {
    error ("lseek(/proc/apm|acpi) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  if (apm) {
    int f3, f4;
    char eh[4];
    char buffer[128];
    /* /usr/src/linux/arch/i386/kernel/apm.c :
      	p += sprintf(p, "%s %d.%d 0x%02x 0x%02x 0x%02x 0x%02x %d%% %d %s\n",
		     driver_version,
		     (apm_bios_info.version >> 8) & 0xff,
		     apm_bios_info.version & 0xff,
		     apm_bios_info.flags,
		     ac_line_status,
		     battery_status,
		     battery_flag,
		     percentage,
		     time_units,
		     units);
	   3) AC line status
	      0x00: Off-line
	      0x01: On-line
	      0x02: On backup power (BIOS >= 1.1 only)
	      0xff: Unknown
	   4) Battery status
	      0x00: High
	      0x01: Low
	      0x02: Critical
	      0x03: Charging
	      0x04: Selected battery not present (BIOS >= 1.2 only)
	      0xff: Unknown
    */
    
    if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
      error ("read(/proc/apm) failed: %s", strerror(errno));
      fd=-1;
      return -1;
    }
    /*              Ver v.v fl  as bs bf perc tu u*/
    sscanf(buffer, "%*s %*s %*s %x %x %*s %d%% %lf %3s", &f3, &f4, perc, dur, eh);
    if (f3 == 0x1) 
      *status = 0; /* Online */
    else
      *status = 2; /* Offline = discharging */
    if (f4 == 0x3)
      *status = 1; /* charging */
    eh[1] = '\0'; /* m | s */
    if (*eh == 'm')
      *dur *= 60;
#ifdef DEBUG_BATT    
    info("perc=%d, f3=%x, f4=%x, stat=%d, dur=%lf", *perc,f3,f4, *status, *dur);
#endif    
  }
  else {
    /* FIXME: parse ACPI */
  }
  return 0;
}
