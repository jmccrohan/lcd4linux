/* $Id: drv_generic_parport.c,v 1.1 2004/01/20 14:35:38 reinelt Exp $
 *
 * generic driver helper for serial and parport access
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
 * $Log: drv_generic_parport.c,v $
 * Revision 1.1  2004/01/20 14:35:38  reinelt
 * drv_generic_parport added, code from parport.c
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
#include "drv_generic_parport.h"


static unsigned short Port=0;
static char *PPdev=NULL;

// initial value taken from linux/parport_pc.c
static unsigned char ctr = 0xc;

#ifdef WITH_PPDEV
static int PPfd=-1;
#endif


int drv_generic_parport_open (void)
{
  char *s, *e;
  
#ifdef USE_OLD_UDELAY
  if (cfg_number(NULL, "Delay", 0, 1, 1000000000, &loops_per_usec)<0) return -1;
#else
  udelay_init();
#endif
  
  s=cfg_get (NULL, "Port", NULL);
  if (s==NULL || *s=='\0') {
    error ("parport: no 'Port' entry in %s", cfg_source());
    return -1;
  }
  PPdev=NULL;
  if ((Port=strtol(s, &e, 0))==0 || *e!='\0') {
#ifdef WITH_PPDEV
    Port=0;
    PPdev=s;
#else
    error ("parport: bad Port '%s' in %s", s, cfg_source());
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
    // Fixme: this always fails here...
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
      if ((Port+3)<=0x3ff) {
	if (ioperm(Port, 3, 1)!=0) {
	  error ("parport: ioperm(0x%x) failed: %s", Port, strerror(errno));
	  return -1;
	}
      } else {
	if (iopl(3)!=0) {
	  error ("parport: iopl(1) failed: %s", strerror(errno));
	  return -1;
	}
      }
    }
  return 0;
}


int drv_generic_parport_close (void)
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
      if ((Port+3)<=0x3ff) {
	if (ioperm(Port, 3, 0)!=0) {
	  error ("parport: ioperm(0x%x) failed: %s", Port, strerror(errno));
	  return -1;
	} 
      } else {
	if (iopl(0)!=0) {
	  error ("parport: iopl(0) failed: %s", strerror(errno));
	  return -1;
	}
      }
    }
  return 0;
}


unsigned char drv_generic_parport_wire_ctrl (char *name, unsigned char *deflt)
{
  unsigned char w;
  char wire[256];
  char *s;
  
  snprintf (wire, sizeof(wire), "Wire.%s", name);
  s=cfg_get (NULL, wire, deflt);
  if (strcasecmp(s,"STROBE")==0) {
    w=PARPORT_CONTROL_STROBE;
  } else if(strcasecmp(s,"AUTOFD")==0) {
    w=PARPORT_CONTROL_AUTOFD;
  } else if(strcasecmp(s,"INIT")==0) {
    w=PARPORT_CONTROL_INIT;
  } else if(strcasecmp(s,"SELECT")==0) {
    w=PARPORT_CONTROL_SELECT;
  } else if(strcasecmp(s,"GND")==0) {
    w=0;
  } else {
    error ("parport: unknown signal <%s> for wire <%s>", s, name);
    error ("         should be STROBE, AUTOFD, INIT, SELECT or GND");
    return 0xff;
  }

  if (w&PARPORT_CONTROL_STROBE) {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:STROBE]", name);
  }
  if (w&PARPORT_CONTROL_AUTOFD) {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:AUTOFD]", name);
  }
  if (w&PARPORT_CONTROL_INIT) {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:INIT]", name);
  }
  if (w&PARPORT_CONTROL_SELECT) {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:SELECT]", name);
  }
  if (w==0) {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:GND]", name);
  }
  
  return w;
}


unsigned char drv_generic_parport_wire_data (char *name, unsigned char *deflt)
{
  unsigned char w;
  char wire[256];
  char *s;
  
  snprintf (wire, sizeof(wire), "Wire.%s", name);
  s=cfg_get (NULL, wire, deflt);
  if(strlen(s)==3 && strncasecmp(s,"DB",2)==0 && s[2]>='0' && s[2]<='7') {
    w=s[2]-'0';
  } else if(strcasecmp(s,"GND")==0) {
    w=0;
  } else {
    error ("parport: unknown signal <%s> for wire <%s>", s, name);
    error ("         should be DB0..7 or GND");
    return 0xff;
  }
  
  if (w==0) {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:GND]", name);
  } else {
    info ("wiring: [DISPLAY:%s]<==>[PARPORT:DB%d]", name, w);
  }
  
  w=1<<w;

  return w;
}


void drv_generic_parport_direction (int direction)
{
#ifdef WITH_PPDEV
  if (PPdev) {
    ioctl (PPfd, PPDATADIR, &direction);
  } else
#endif
    {
      // code stolen from linux/parport_pc.h
      ctr = (ctr & ~0x20) ^ (direction?0x20:0x00);
      outb (ctr, Port+2);
    }
}


void drv_generic_parport_control (unsigned char mask, unsigned char value)
{
  // any signal affected?
  // Note: this may happen in case a signal is hardwired to GND
  if (mask==0) return;

  // Strobe, Select and AutoFeed are inverted!
  value = mask & (value ^ (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD));

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


void drv_generic_parport_toggle (unsigned char bits, int level, int delay)
{
  unsigned char value1, value2;

  // any signal affected?
  // Note: this may happen in case a signal is hardwired to GND
  if (bits==0) return;

  // prepare value
  value1=level?bits:0;
  value2=level?0:bits;
  
  // Strobe, Select and AutoFeed are inverted!
  value1 = bits & (value1 ^ (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD));
  value2 = bits & (value2 ^ (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD));
  
 
#ifdef WITH_PPDEV
  if (PPdev) {
    struct ppdev_frob_struct frob;
    frob.mask=bits;
    
    // rise
    frob.val=value1;
    ioctl (PPfd, PPFCONTROL, &frob);
    
    // pulse width
    ndelay(delay);      
    
    // lower
    frob.val=value2;
    ioctl (PPfd, PPFCONTROL, &frob);
  } else
#endif
    {
      // rise
      ctr = (ctr & ~bits) ^ value1;
      outb (ctr, Port+2);

      // pulse width
      ndelay(delay);      
      
      // lower
      ctr = (ctr & ~bits) ^ value2;
      outb (ctr, Port+2);
    }
}


void drv_generic_parport_data (unsigned char data)
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

unsigned char drv_generic_parport_read (void)
{
  unsigned char data;
  
#ifdef WITH_PPDEV
  if (PPdev) {
    ioctl (PPfd, PPRDATA, &data);
  } else
#endif
    {
      data=inb (Port);
    }
  return data;
}


void drv_generic_parport_debug(void)
{
  unsigned char control;
  
#ifdef WITH_PPDEV
  if (PPdev) {
    ioctl (PPfd, PPRCONTROL, &control);
  } else 
#endif
    {
      control=ctr;
    }
  
  debug ("%cSTROBE %cAUTOFD %cINIT %cSELECT", 
	 control & PARPORT_CONTROL_STROBE ? '-':'+',
	 control & PARPORT_CONTROL_AUTOFD ? '-':'+',
	 control & PARPORT_CONTROL_INIT   ? '+':'-',
	 control & PARPORT_CONTROL_SELECT ? '-':'+');
  
}
