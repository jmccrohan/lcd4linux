/* $Id: plugin_apm.c,v 1.5 2005/01/18 06:30:23 reinelt Exp $
 *
 * plugin for APM (battery status)
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old 'battery.c' which is 
 * Copyright (C) 2001 Leopold Tötsch <lt@toetsch.at>
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
 * $Log: plugin_apm.c,v $
 * Revision 1.5  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.4  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.3  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.2  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.1  2004/03/14 07:11:42  reinelt
 * parameter count fixed for plugin_dvb()
 * plugin_APM (battery status) ported
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_apm (void)
 *  adds apm() function
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <asm/types.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"

static int fd = -2;
static HASH APM;

/* from /usr/src/linux/arch/i386/kernel/apm.c:
 *
 * Arguments, with symbols from linux/apm_bios.h.  Information is
 * from the Get Power Status (0x0a) call unless otherwise noted.
 *
 * 0) Linux driver version (this will change if format changes)
 * 1) APM BIOS Version.  Usually 1.0, 1.1 or 1.2.
 * 2) APM flags from APM Installation Check (0x00):
 *    bit 0: APM_16_BIT_SUPPORT
 *    bit 1: APM_32_BIT_SUPPORT
 *    bit 2: APM_IDLE_SLOWS_CLOCK
 *    bit 3: APM_BIOS_DISABLED
 *    bit 4: APM_BIOS_DISENGAGED
 * 3) AC line status
 *    0x00: Off-line
 *    0x01: On-line
 *    0x02: On backup power (BIOS >= 1.1 only)
 *    0xff: Unknown
 * 4) Battery status
 *    0x00: High
 *    0x01: Low
 *    0x02: Critical
 *    0x03: Charging
 *    0x04: Selected battery not present (BIOS >= 1.2 only)
 *    0xff: Unknown
 * 5) Battery flag
 *    bit 0: High
 *    bit 1: Low
 *    bit 2: Critical
 *    bit 3: Charging
 *    bit 7: No system battery
 *    0xff: Unknown
 * 6) Remaining battery life (percentage of charge):
 *    0-100: valid
 *    -1: Unknown
 * 7) Remaining battery life (time units):
 *    Number of remaining minutes or seconds
 *    -1: Unknown
 * 8) min = minutes; sec = seconds
 *
 * p+= sprintf(p, "%s %d.%d 0x%02x 0x%02x 0x%02x 0x%02x %d%% %d %s\n",
 *             driver_version,
 *             (apm_info.bios.version >> 8) & 0xff,
 *             apm_info.bios.version & 0xff,
 *             apm_info.bios.flags,
 *             ac_line_status,
 *             battery_status,
 *             battery_flag,
 *             percentage,
 *             time_units,
 *             units);
 */


static int parse_proc_apm (void)
{
  char *key[] = { "driver_version", 
		  "bios_version", 
		  "bios_flags", 
		  "line_status",
		  "battery_status",
		  "battery_flag",
		  "battery_percent",
		  "battery_remaining",
		  "time_units" }; 
  
  char buffer[128], *beg, *end; 
  int age, i;

  /* reread every 10 msec only */
  age = hash_age (&APM, NULL);
  if (age > 0 && age <= 10) return 0;
  
  if (fd == -2) {
    fd = open("/proc/apm", O_RDONLY | O_NDELAY);
    if (fd == -1) {
      error ("open(/proc/apm) failed: %s", strerror(errno));
      return -1;
    }
  }
  
  if (lseek(fd, 0L, SEEK_SET) != 0) {
    error ("lseek(/proc/apm) failed: %s", strerror(errno));
    fd = -1;
    return -1;
  }
  
  if (read (fd, &buffer, sizeof(buffer)-1) == -1) {
    error ("read(/proc/apm) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  
  beg = buffer;
  for (i = 0; i < 9 && beg != NULL; i++) {
    while (*beg == ' ') beg++; 
    if ((end = strpbrk(beg, " \n"))) *end='\0'; 
    hash_put (&APM, key[i], beg); 
    beg = end ? end+1 : NULL;
  }
  
  return 0;
}


static void my_apm (RESULT *result, RESULT *arg1)
{
  char *val;
  
  if (parse_proc_apm() < 0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }

  val = hash_get(&APM, R2S(arg1), NULL);
  if (val == NULL) val = "";

  SetResult(&result, R_STRING, val); 
}

int plugin_init_apm (void)
{
  hash_create (&APM);
  
  AddFunction ("apm", 1, my_apm);

  return 0;
}

void plugin_exit_apm (void) 
{
  if (fd > -1) {
    close (fd);
  }
  fd = -2;

  hash_destroy(&APM);
}
