/* $Id: plugin_dvb.c,v 1.1 2004/02/10 06:54:39 reinelt Exp $
 *
 * plugin for DVB status
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin_dvb.c,v $
 * Revision 1.1  2004/02/10 06:54:39  reinelt
 * DVB plugin ported
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_dvb (void)
 *  adds dvb() function
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

#include <linux/dvb/frontend.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"

static char *frontend="/dev/dvb/adapter0/frontend0";

static HASH DVB = { 0, };

static int get_dvb_stats (void)
{
  int age;
  int fd;
  unsigned short snr, sig;
  unsigned long  ber, ucb;
  char val[16];
  
  // reread every 1000 msec only
  age=hash_age(&DVB, NULL, NULL);
  if (age>0 && age<=1000) return 0;
  
  // open frontend
  fd = open(frontend, O_RDONLY);
  if (fd==-1) {
    error ("open(%s) failed: %s", frontend, strerror(errno));
    return -1;
  }
  
  if (ioctl(fd, FE_READ_SIGNAL_STRENGTH, &sig)!=0) {
    error("ioctl(FE_READ_SIGNAL_STRENGTH) failed: %s", strerror(errno));
    sig=0;
  }
  
  if (ioctl(fd, FE_READ_SNR, &snr)!=0) {
    error("ioctl(FE_READ_SNR) failed: %s", strerror(errno));
    snr=0;
  }
  
  if (ioctl(fd, FE_READ_BER, &ber)!=0) {
    error("ioctl(FE_READ_BER) failed: %s", strerror(errno));
    ber=0;
  }

  if (ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &ucb)!=0) {
    error("ioctl(FE_READ_UNCORRECTED_BLOCKS) failed: %s", strerror(errno));
    ucb=0;
  }

  close (fd);

  snprintf (val, sizeof(val), "%f", sig/65535.0);
  hash_set (&DVB, "signal_strength", val);

  snprintf (val, sizeof(val), "%f", snr/65535.0);
  hash_set (&DVB, "snr", val);
  
  snprintf (val, sizeof(val), "%lu", ber);
  hash_set (&DVB, "ber", val);

  snprintf (val, sizeof(val), "%lu", ucb);
  hash_set (&DVB, "uncorrected_blocks", val);

  return 0;
}


static void my_dvb (RESULT *result, RESULT *arg1)
{
  char *val;
  
  if (get_dvb_stats()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }

  val=hash_get(&DVB, R2S(arg1));
  if (val==NULL) val="";

  SetResult(&result, R_STRING, val); 
}

int plugin_init_dvb (void)
{
  AddFunction ("dvb", 3, my_dvb);
  return 0;
}

