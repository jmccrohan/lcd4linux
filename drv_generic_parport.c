/* $Id: drv_generic_parport.c,v 1.9 2004/09/18 08:22:59 reinelt Exp $
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
 * Revision 1.9  2004/09/18 08:22:59  reinelt
 * drv_generic_parport_status() to read status lines
 *
 * Revision 1.8  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.7  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.6  2004/06/20 10:09:55  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.5  2004/04/12 05:14:42  reinelt
 * another BIG FAT WARNING on the use of raw ports instead of ppdev
 *
 * Revision 1.4  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.3  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.2  2004/01/20 15:32:49  reinelt
 * first version of Next Generation HD44780 (untested! but it compiles...)
 * some cleanup in the other drivers
 *
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
#include "qprintf.h"
#include "cfg.h"
#include "udelay.h"
#include "drv_generic_parport.h"


static char *Driver="";
static char *Section="";
static unsigned short Port=0;
static char *PPdev=NULL;

/* initial value taken from linux/parport_pc.c */
static unsigned char ctr = 0xc;

#ifdef WITH_PPDEV
static int PPfd=-1;
#endif


int drv_generic_parport_open (const char *section, const char *driver)
{
  char *s, *e;
  
  Section = (char*)section;
  Driver  = (char*)driver;
  
  udelay_init();
  
#ifndef WITH_PPDEV
  error ("The files include/linux/parport.h and/or include/linux/ppdev.h");
  error ("were missing at compile time. Even if your system supports");
  error ("ppdev, it will not be used.");
  error ("You *really* should install these files and recompile LCD4linux!");
#endif
  
  s=cfg_get (Section, "Port", NULL);
  if (s==NULL || *s=='\0') {
    error ("%s: no '%s.Port' entry from %s", Driver, Section, cfg_source());
    return -1;
  }

  PPdev=NULL;
  if ((Port=strtol(s, &e, 0))==0 || *e!='\0') {
#ifdef WITH_PPDEV
    Port=0;
    PPdev=s;
#else
    error ("%s: bad %s.Port '%s' from %s", Driver, Section, s, cfg_source());
    free(s);
    return -1;
#endif
  }
  
  
#ifdef WITH_PPDEV
  
  if (PPdev) {
    info ("using ppdev %s", PPdev);
    PPfd=open(PPdev, O_RDWR);
    if (PPfd==-1) {
      error ("%s: open(%s) failed: %s", Driver, PPdev, strerror(errno));
      return -1;
    }
    
#if 0
    /* Fixme: this always fails here... */
    if (ioctl(PPfd, PPEXCL)) {
      debug ("ioctl(%s, PPEXCL) failed: %s", PPdev, strerror(errno));
    } else {
      debug ("ioctl(%s, PPEXCL) succeded.");
    }
#endif
    
    if (ioctl(PPfd, PPCLAIM)) {
      error ("%s: ioctl(%s, PPCLAIM) failed: %d %s", Driver, PPdev, errno, strerror(errno));
      return -1;
    }
  } else
    
#endif
    
    {
      error ("using raw port 0x%x (deprecated!)", Port);
      error ("You *really* should change your setup and use ppdev!");
      if ((Port+3)<=0x3ff) {
	if (ioperm(Port, 3, 1)!=0) {
	  error ("%s: ioperm(0x%x) failed: %s", Driver, Port, strerror(errno));
	  return -1;
	}
      } else {
	if (iopl(3)!=0) {
	  error ("%s: iopl(1) failed: %s", Driver, strerror(errno));
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
      error ("%s: ioctl(%s, PPRELEASE) failed: %s", Driver, PPdev, strerror(errno));
    }
    if (close(PPfd)==-1) {
      error ("%s: close(%s) failed: %s", Driver, PPdev, strerror(errno));
      return -1;
    }
    free(PPdev);
  } else 
#endif    
    {
      debug ("closing raw port 0x%x", Port);
      if ((Port+3)<=0x3ff) {
	if (ioperm(Port, 3, 0)!=0) {
	  error ("%s: ioperm(0x%x) failed: %s", Driver, Port, strerror(errno));
	  return -1;
	} 
      } else {
	if (iopl(0)!=0) {
	  error ("%s: iopl(0) failed: %s", Driver, strerror(errno));
	  return -1;
	}
      }
    }

  return 0;
}


unsigned char drv_generic_parport_wire_ctrl (const char *name, const char *deflt)
{
  unsigned char w;
  char wire[256];
  char *s;
  
  qprintf(wire, sizeof(wire), "Wire.%s", name);
  s=cfg_get (Section, wire, deflt);
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
    error ("%s: unknown signal <%s> for wire <%s>", Driver, s, name);
    error ("%s: should be STROBE, AUTOFD, INIT, SELECT or GND", Driver);
    return 0xff;
  }
  free(s);

  if (w&PARPORT_CONTROL_STROBE) {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:STROBE]", Driver, name);
  }
  if (w&PARPORT_CONTROL_AUTOFD) {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:AUTOFD]", Driver, name);
  }
  if (w&PARPORT_CONTROL_INIT) {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:INIT]", Driver, name);
  }
  if (w&PARPORT_CONTROL_SELECT) {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:SELECT]", Driver, name);
  }
  if (w==0) {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:GND]", Driver, name);
  }
  
  return w;
}


unsigned char drv_generic_parport_wire_data (const char *name, const char *deflt)
{
  unsigned char w;
  char wire[256];
  char *s;
  
  qprintf(wire, sizeof(wire), "Wire.%s", name);
  s=cfg_get (Section, wire, deflt);
  if(strlen(s)==3 && strncasecmp(s,"DB",2)==0 && s[2]>='0' && s[2]<='7') {
    w=s[2]-'0';
  } else if(strcasecmp(s,"GND")==0) {
    w=0;
  } else {
    error ("%s: unknown signal <%s> for wire <%s>", Driver, s, name);
    error ("%s: should be DB0..7 or GND", Driver);
    return 0xff;
  }
  free(s);
  if (w==0) {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:GND]", Driver, name);
  } else {
    info ("%s: wiring: [DISPLAY:%s]<==>[PARPORT:DB%d]", Driver, name, w);
  }
  
  w=1<<w;

  return w;
}


void drv_generic_parport_direction (const int direction)
{
#ifdef WITH_PPDEV
  if (PPdev) {
    ioctl (PPfd, PPDATADIR, &direction);
  } else
#endif
    {
      /* code stolen from linux/parport_pc.h */
      ctr = (ctr & ~0x20) ^ (direction?0x20:0x00);
      outb (ctr, Port+2);
    }
}


unsigned char drv_generic_parport_status (void)
{
  unsigned char mask = 
    PARPORT_STATUS_ERROR    | 
    PARPORT_STATUS_SELECT   |
    PARPORT_STATUS_PAPEROUT |
    PARPORT_STATUS_ACK      |
    PARPORT_STATUS_BUSY;

  unsigned char data;
  
#ifdef WITH_PPDEV
  if (PPdev) {
    ioctl (PPfd, PPRSTATUS, &data);
  } else
#endif
    {
      data = inb (Port+1);
    }
  
  /* clear unused bits */
  data &= mask;
  
  return data;
}


void drv_generic_parport_control (const unsigned char mask, const unsigned char value)
{
  unsigned char val;
  
  /* any signal affected? */
  /* Note: this may happen in case a signal is hardwired to GND */
  if (mask==0) return;

  /* Strobe, Select and AutoFeed are inverted! */
  val = mask & (value ^ (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD));

#ifdef WITH_PPDEV
  if (PPdev) {
    struct ppdev_frob_struct frob;
    frob.mask=mask;
    frob.val=val;
    ioctl (PPfd, PPFCONTROL, &frob);
  } else
#endif
    {
      /* code stolen from linux/parport_pc.h */
      ctr = (ctr & ~mask) ^ val;
      outb (ctr, Port+2);
    }
}


void drv_generic_parport_toggle (const unsigned char bits, const int level, const int delay)
{
  unsigned char value1, value2;

  /* any signal affected? */
  /* Note: this may happen in case a signal is hardwired to GND */
  if (bits==0) return;

  /* prepare value */
  value1=level?bits:0;
  value2=level?0:bits;
  
  /* Strobe, Select and AutoFeed are inverted! */
  value1 = bits & (value1 ^ (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD));
  value2 = bits & (value2 ^ (PARPORT_CONTROL_STROBE|PARPORT_CONTROL_SELECT|PARPORT_CONTROL_AUTOFD));
  
 
#ifdef WITH_PPDEV
  if (PPdev) {
    struct ppdev_frob_struct frob;
    frob.mask=bits;
    
    /* rise */
    frob.val=value1;
    ioctl (PPfd, PPFCONTROL, &frob);
    
    /* pulse width */
    ndelay(delay);      
    
    /* lower */
    frob.val=value2;
    ioctl (PPfd, PPFCONTROL, &frob);
  } else
#endif
    {
      /* rise */
      ctr = (ctr & ~bits) ^ value1;
      outb (ctr, Port+2);

      /* pulse width */
      ndelay(delay);      
      
      /* lower */
      ctr = (ctr & ~bits) ^ value2;
      outb (ctr, Port+2);
    }
}


void drv_generic_parport_data (const unsigned char data)
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
