/* $Id: parport.c,v 1.1 2003/04/04 06:02:03 reinelt Exp $
 *
 * generic parallel port handling
 *
 * Copyright 2003 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: parport.c,v $
 * Revision 1.1  2003/04/04 06:02:03  reinelt
 * new parallel port abstraction scheme
 *
 */

/* 
 * exported functions:
 * 
 * int parport_open (void)
 *   reads 'Port' entry from config and opens
 *   the parallel port
 *   returns 0 if ok, -1 on failure
 *
 * int parport_close (void)
 *   closes parallel port
 *   returns 0 if ok, -1 on failure
 *
 * unsigned char parport_wire (char *name, char *deflt)
 *   reads wiring for one control signal from config
 *   returns PARPORT_CONTROL_* or 255 on error
 *
 * void parport_control (unsigned char mask, unsigned char value)
 *   frobs control line and takes care of inverted signals
 *
 * void parport_toggle (unsigned char bit, int level, int delay)
 *   toggles the line <bit> to <level> for <delay> nanoseconds
 *
 * void parport_data (unsigned char value)
 *   put data bits on DB1..DB8
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>


#ifdef HAVE_SYS_IO_H
#include <sys/io.h>
#define WITH_OUTB
#else
#ifdef HAVE_ASM_IO_H
#include <asm/io.h>
#define WITH_OUTB
#endif
#endif

#if defined (HAVE_LINUX_PARPORT_H) && defined (HAVE_LINUX_PPDEV_H)
#define WITH_PPDEV
#include <linux/parport.h>
#include <linux/ppdev.h>
#else
#define PARPORT_CONTROL_STROBE    0x1
#define PARPORT_CONTROL_AUTOFD    0x2
#define PARPORT_CONTROL_INIT      0x4
#define PARPORT_CONTROL_SELECT    0x8
#endif

#if !defined(WITH_OUTB) && !defined(WITH_PPDEV)
#error neither outb() nor ppdev() possible
#error cannot compile parallel port driver
#endif


#include "debug.h"
#include "cfg.h"
#include "udelay.h"
#include "parport.h"


static unsigned short Port=0;
static char *PPdev=NULL;

// initial value taken from linux/parport_pc.c
static unsigned char ctr = 0xc;

#ifdef WITH_PPDEV
static int PPfd=-1;
#endif

int parport_open (void)
{
  char *s, *e;

#ifdef USE_OLD_UDELAY
  s=cfg_get ("Delay",NULL);
  if (s==NULL || *s=='\0') {
    error ("parport: no 'Delay' entry in %s", cfg_file());
    return -1;
  }
  if ((loops_per_usec=strtol(s, &e, 0))==0 || *e!='\0') {
    error ("parport: bad delay '%s' in %s", s, cfg_file());
    return -1;
  }    
#else
  udelay_init();
#endif
  
  s=cfg_get ("Port",NULL);
  if (s==NULL || *s=='\0') {
    error ("parport: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  PPdev=NULL;
  if ((Port=strtol(s, &e, 0))==0 || *e!='\0') {
#ifdef WITH_PPDEV
    Port=0;
    PPdev=s;
#else
    error ("parport: bad Port '%s' in %s", s, cfg_file());
    return -1;
#endif
  }
  
  
#ifdef WITH_PPDEV
  
  if (PPdev) {
    debug ("using ppdev %s", PPdev);
    PPfd=open(PPdev, O_RDWR);
    if (PPfd==-1) {
      error ("parport: open(%s) failed: %s", PPdev, strerror(errno));
      return -1;
    }
    
#if 0
    // Fixme
    if (ioctl(PPfd, PPEXCL)) {
      debug ("ioctl(%s, PPEXCL) failed: %s", PPdev, strerror(errno));
    } else {
      debug ("ioctl(%s, PPEXCL) succeded.");
    }
#endif
    
    if (ioctl(PPfd, PPCLAIM)) {
      error ("parport: ioctl(%s, PPCLAIM) failed: %d %s", PPdev, errno, strerror(errno));
      return -1;
    }
  } else
    
#endif
    
    {
      debug ("using raw port 0x%x", Port);
      if (ioperm(Port, 3, 1)!=0) {
	error ("parport: ioperm(0x%x) failed: %s", Port, strerror(errno));
	return -1;
      }
    }
  return 0;
}


int parport_close (void)
{
#ifdef WITH_PPDEV
  if (PPdev) {
    debug ("closing ppdev %s", PPdev);
    if (ioctl(PPfd, PPRELEASE)) {
      error ("parport: ioctl(%s, PPRELEASE) failed: %s", PPdev, strerror(errno));
    }
    if (close(PPfd)==-1) {
      error ("parport: close(%s) failed: %s", PPdev, strerror(errno));
      return -1;
    }
  } else 
#endif    
    {
      debug ("closing raw port 0x%x", Port);
      if (ioperm(Port, 3, 0)!=0) {
	error ("parport: ioperm(0x%x) failed: %s", Port, strerror(errno));
	return -1;
      }
    }
  return 0;
}


unsigned char parport_wire (char *name, unsigned char *deflt)
{
  unsigned char w;
  char wire[256];
  char *s;
  int n;
  
  snprintf (wire, sizeof(wire), "Wire.%s", name);
  s=cfg_get (wire,deflt);
  if (strcasecmp(s,"STROBE")==0) {
    w=PARPORT_CONTROL_STROBE;
  } else if(strcasecmp(s,"AUTOFD")==0) {
    w=PARPORT_CONTROL_AUTOFD;
  } else if(strcasecmp(s,"INIT")==0) {
    w=PARPORT_CONTROL_INIT;
  } else if(strcasecmp(s,"SELECT")==0) {
    w=PARPORT_CONTROL_SELECT;
  } else {
    error ("parport: unknown signal <%s> for wire <%s>", s, name);
    error ("         should be STROBE, AUTOFD, INIT or SELECT");
    return 255;
  }
  
  n=0;
  if (w&PARPORT_CONTROL_STROBE) {
    n++;
    info ("wiring: [DISPLAY:%s]==[PARPORT:STROBE]", name);
  }
  if (w&PARPORT_CONTROL_AUTOFD) {
    n++;
    info ("wiring: [DISPLAY:%s]==[PARPORT:AUTOFD]", name);
  }
  if (w&PARPORT_CONTROL_INIT) {
    n++;
    info ("wiring: [DISPLAY:%s]==[PARPORT:INIT]", name);
  }
  if (w&PARPORT_CONTROL_SELECT) {
    n++;
    info ("wiring: [DISPLAY:%s]==[PARPORT:SELECT]", name);
  }
  if (n<1) {
    error ("parport: no signal for wire <%s> found!", name);
    return 255;
  }
  if (n>1) {
    error ("parport: more than one signal for wire <%s> found!", name);
    return 255;
  }
  return w;
}


void parport_control (unsigned char mask, unsigned char value)
{

  // sanity check
  if (mask==0) {
    error ("parport: internal error: control without signal called!");
    return;
  }

  // Strobe, Select and AutoFeed are inverted!
  value ^= PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD;

#ifdef WITH_PPDEV
  if (PPdev) {
    struct ppdev_frob_struct frob;
    frob.mask=mask;
    frob.val=value;
    ioctl (PPfd, PPFCONTROL, &frob);
  } else
#endif
    {
      // code stolen from linux/parport_pc.h
      ctr = (ctr & ~mask) ^ value;
      outb (ctr, Port+2);
    }
}


void parport_toggle (unsigned char bit, int level, int delay)
{

  // sanity check
  if (bit==0) {
    error ("parport: internal error: toggle without signal called!");
    return;
  }

   // Strobe, Select and AutoFeed are inverted!
  if (bit & (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD)) {
    level=!level;
  }
 
#ifdef WITH_PPDEV
  if (PPdev) {
    struct ppdev_frob_struct frob;
    frob.mask=bit;
    
    // rise
    frob.val=level?bit:0;
    ioctl (PPfd, PPFCONTROL, &frob);
    
    // pulse width
    ndelay(delay);      
    
    // lower
    frob.val=level?0:bit;
    ioctl (PPfd, PPFCONTROL, &frob);
  } else
#endif
    {
      // rise
      ctr = (ctr & ~bit) ^ (level?bit:0);
      outb (ctr, Port+2);

      // pulse width
      ndelay(delay);      
      
      // lower
      ctr = (ctr & ~bit) ^ (level?0:bit);
      outb (ctr, Port+2);
    }
}


void parport_data (unsigned char data)
{
#ifdef WITH_PPDEV
  if (PPdev) {
    ioctl(PPfd, PPWDATA, &data);
  } else
#endif
    {
      outb (data, Port);
    }
}

