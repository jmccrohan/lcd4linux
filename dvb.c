/* $Id: dvb.c,v 1.7 2003/11/11 04:30:41 reinelt Exp $
 *
 * DVB specific functions
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: dvb.c,v $
 * Revision 1.7  2003/11/11 04:30:41  reinelt
 * very minor changes
 *
 * Revision 1.6  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.5  2003/08/08 23:08:38  reinelt
 * CVS Id and Log missing from dvb.{c,h}
 *
 */

/* 
 * exported functions:
 *
 * DVB (int *strength, int *snr)
 *   returns 0 if ok, -1 if error
 *   sets *strength to the signal strength
 *   sets *snr to the signal/noise ratio
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "config.h"
#include "debug.h"

#ifndef HAVE_LINUX_DVB_FRONTEND_H

#warning linux/dvb/frontend.h not found.
#warning DVB client will be disabled.

int DVB (double *strength, double *snr)
{
  static int flag=0;

  if (flag==0) {
    error("lcd4linux has been compiled without DVB support");
    error("DVB client will be disabled");
    flag=1;
  }

  *strength=0.0;
  *snr=0.0;

  return 0;
}

#else

#include <linux/dvb/frontend.h>
#include "dvb.h"

int DVB (double *strength, double *snr)
{
  static time_t now=0;
  static int fd=-2;
  static double strength0=0.0;
  static double snr0=0.0;
  
  unsigned short raw_strength;
  unsigned short raw_snr;

  *strength=strength0;
  *snr=snr0;

  if (fd==-1) return -1;
  
  if (time(NULL)==now) return 0;
  time(&now);

  if (fd==-2) {
    fd = open("/dev/dvb/adapter0/frontend0", O_RDONLY);
    if (fd==-1) {
      error ("open(/dev/dvb/adapter0/frontend0) failed: %s", strerror(errno));
      return -1;
    }
    debug ("open (/dev/dvb/adapter0/frontend0)=%d", fd);
  }

  if (ioctl(fd, FE_READ_SIGNAL_STRENGTH, &raw_strength)!=0) {
    error("ioctl(FE_READ_SIGNAL_STRENGTH) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  
  if (ioctl(fd, FE_READ_SNR, &raw_snr)!=0) {
    error("ioctl(FE_READ_SNR) failed: %s", strerror(errno));
    fd=-1;
    return -1;
  }
  
  // Normalize to 1.0
  strength0=raw_strength/65535.0;
  snr0=raw_snr/65535.0;
  
  *strength=strength0;
  *snr=snr0;

  return 0;
}

#endif
