/* $Id: HD44780.c,v 1.26 2003/02/22 07:53:09 reinelt Exp $
 *
 * driver for display modules based on the HD44780 chip
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
 * $Log: HD44780.c,v $
 * Revision 1.26  2003/02/22 07:53:09  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.25  2002/08/19 09:11:34  reinelt
 * changed HD44780 to use generic bar functions
 *
 * Revision 1.24  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.23  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.22  2002/08/17 14:14:21  reinelt
 *
 * USBLCD fixes
 *
 * Revision 1.21  2002/04/30 07:20:15  reinelt
 *
 * implemented the new ndelay(nanoseconds) in all parallel port drivers
 *
 * Revision 1.20  2001/09/09 12:26:03  reinelt
 * GPO bug: INIT is _not_ inverted
 *
 * Revision 1.19  2001/03/15 15:49:22  ltoetsch
 * fixed compile HD44780.c, cosmetics
 *
 * Revision 1.18  2001/03/15 09:47:13  reinelt
 *
 * some fixes to ppdef
 * off-by-one bug in processor.c fixed
 *
 * Revision 1.17  2001/03/14 16:47:41  reinelt
 * minor cleanups
 *
 * Revision 1.16  2001/03/14 15:30:53  reinelt
 *
 * make ppdev compatible to earlier kernels
 *
 * Revision 1.15  2001/03/14 15:14:59  reinelt
 * added ppdev parallel port access
 *
 * Revision 1.14  2001/03/12 13:44:58  reinelt
 *
 * new udelay() using Time Stamp Counters
 *
 * Revision 1.13  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.12  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.11  2001/02/13 12:43:24  reinelt
 *
 * HD_gpo() was missing
 *
 * Revision 1.10  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.9  2000/10/20 07:17:07  reinelt
 *
 *
 * corrected a bug in HD_goto()
 * Thanks to Gregor Szaktilla <gregor@szaktilla.de>
 *
 * Revision 1.8  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.7  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.6  2000/07/31 10:43:44  reinelt
 *
 * some changes to support kernel-2.4 (different layout of various files in /proc)
 *
 * Revision 1.5  2000/07/31 06:46:35  reinelt
 *
 * eliminated some compiler warnings with glibc
 *
 * Revision 1.4  2000/04/15 16:56:52  reinelt
 *
 * moved delay loops to udelay.c
 * renamed -d (debugging) switch to -v (verbose)
 * new switch -d to calibrate delay loop
 * 'Delay' entry for HD44780 back again
 * delay loops will not calibrate automatically, because this will fail with hich CPU load
 *
 * Revision 1.3  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 * Revision 1.2  2000/04/13 06:09:52  reinelt
 *
 * added BogoMips() to system.c (not used by now, maybe sometimes we can
 * calibrate our delay loop with this value)
 *
 * added delay loop to HD44780 driver. It seems to be quite fast now. Hopefully
 * no compiler will optimize away the delay loop!
 *
 * Revision 1.1  2000/04/12 08:05:45  reinelt
 *
 * first version of the HD44780 driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD HD44780[]
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
#endif


#if !defined(WITH_OUTB) && !defined(WITH_PPDEV)
#error neither outb() nor ppdev() possible
#error cannot compile HD44780 driver
#endif

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "udelay.h"

#define XRES 5
#define YRES 8
#define CHARS 8

static LCD Lcd;


static unsigned short Port=0;

static char *PPdev=NULL;

#ifdef WITH_PPDEV
static int PPfd=-1;
#endif

static char Txt[4][40];
static int  GPO=0;

#ifdef WITH_PPDEV
static void HD_toggle (int bit, int inv, int delay)
{
  struct ppdev_frob_struct frob;
  frob.mask=bit;

  // rise
  frob.val=inv?0:bit;
  ioctl (PPfd, PPFCONTROL, &frob);
  
  // pulse width
  ndelay(delay);      

  // lower
  frob.val=inv?bit:0;
  ioctl (PPfd, PPFCONTROL, &frob);
}
#endif

static void HD_command (unsigned char cmd, int delay)
{
#ifdef WITH_PPDEV
  if (PPdev) {

    struct ppdev_frob_struct frob;
    
    // put data on DB1..DB8
    ioctl(PPfd, PPWDATA, &cmd);
    
    // clear RS (inverted)
    frob.mask=PARPORT_CONTROL_AUTOFD;
    frob.val=PARPORT_CONTROL_AUTOFD;
    ioctl (PPfd, PPFCONTROL, &frob);
    
    // Address set-up time
    ndelay(40);

    // send command
    // Enable cycle time = 230ns
    HD_toggle(PARPORT_CONTROL_STROBE, 1, 230);
    
    // wait for command completion
    udelay(delay);

  } else

#endif

    {
      outb (cmd, Port);    // put data on DB1..DB8
      outb (0x03, Port+2); // clear RS = bit 2 invertet
      ndelay(40);          // Address set-up time
      outb (0x02, Port+2); // set Enable = bit 0 invertet
      ndelay(230);         // Enable cycle time
      outb (0x03, Port+2); // clear Enable
      udelay (delay);      // wait for command completion
    }
}

static void HD_write (char *string, int len, int delay)
{
#ifdef WITH_PPDEV
  if (PPdev) {

    struct ppdev_frob_struct frob;

    // set RS (inverted)
    frob.mask=PARPORT_CONTROL_AUTOFD;
    frob.val=0;
    ioctl (PPfd, PPFCONTROL, &frob);
    
    // Address set-up time
    ndelay(40);
    
    while (len--) {

      // put data on DB1..DB8
      ioctl(PPfd, PPWDATA, string++);
      
      // send command
      HD_toggle(PARPORT_CONTROL_STROBE, 1, 230);

      // wait for command completion
      udelay(delay);
    }
    
  } else

#endif

    {
      outb (0x01, Port+2); // set RS = bit 2 invertet
      ndelay(40);          // Address set-up time
      while (len--) {
	outb (*string++, Port); // put data on DB1..DB8
	outb (0x00, Port+2);    // set Enable = bit 0 invertet
	ndelay(230);            // Enable cycle time
	outb (0x01, Port+2);    // clear Enable
	udelay (delay);
      }
    }
}

static void HD_setGPO (int bits)
{
  if (Lcd.gpos>0) {

#ifdef WITH_PPDEV


    if (PPdev) {
      
      // put data on DB1..DB8
      ioctl(PPfd, PPWDATA, &bits);

      // 74HCT573 set-up time
      ndelay(20);
      
      // toggle INIT
      // 74HCT573 enable pulse width = 24ns
      HD_toggle(PARPORT_CONTROL_INIT, 0, 24);
      
    } else
      
#endif
      
      {
	outb (bits, Port);    // put data on DB1..DB8
	ndelay(20);           // 74HCT573 set-up time
	outb (0x05, Port+2);  // set INIT = bit 2 invertet
	ndelay(24);           // 74HCT573 enable pulse width
	outb (0x03, Port+2);  // clear INIT
      }
  }
}


static int HD_open (void)
{
#ifdef WITH_PPDEV

  if (PPdev) {
    debug ("using ppdev %s", PPdev);
    PPfd=open(PPdev, O_RDWR);
    if (PPfd==-1) {
      error ("HD44780: open(%s) failed: %s", PPdev, strerror(errno));
      return -1;
    }

#if 0
    if (ioctl(PPfd, PPEXCL)) {
      debug ("ioctl(%s, PPEXCL) failed: %s", PPdev, strerror(errno));
    } else {
      debug ("ioctl(%s, PPEXCL) succeded.");
    }
#endif

    if (ioctl(PPfd, PPCLAIM)) {
      error ("HD44780: ioctl(%s, PPCLAIM) failed: %d %s", PPdev, errno, strerror(errno));
      return -1;
    }
  } else

#endif

    {
      debug ("using raw port 0x%x", Port);
      if (ioperm(Port, 3, 1)!=0) {
	error ("HD44780: ioperm(0x%x) failed: %s", Port, strerror(errno));
	return -1;
      }
    }
  
  HD_command (0x30, 4100); // 8 Bit mode, wait 4.1 ms
  HD_command (0x30, 100);  // 8 Bit mode, wait 100 us
  HD_command (0x30, 4100); // 8 Bit mode, wait 4.1 ms
  HD_command (0x38, 40);   // 8 Bit mode, 1/16 duty cycle, 5x8 font
  HD_command (0x08, 40);   // Display off, cursor off, blink off
  HD_command (0x0c, 1640); // Display on, cursor off, blink off, wait 1.64 ms
  HD_command (0x06, 40);   // curser moves to right, no shift

  return 0;
}


static void HD_define_char (int ascii, char *buffer)
{
  HD_command (0x40|8*ascii, 40);
  HD_write (buffer, 8, 120); // 120 usec delay for CG RAM write
}


int HD_clear (void)
{
  int row, col;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
    }
  }

  bar_clear();

  GPO=0;
  HD_setGPO (GPO);         // All GPO's off
  HD_command (0x01, 1640); // clear display
  return 0;
}


int HD_init (LCD *Self)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s, *e;
  
  s=cfg_get ("Port",NULL);
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  PPdev=NULL;
  if ((Port=strtol(s, &e, 0))==0 || *e!='\0') {
#ifdef WITH_PPDEV
    Port=0;
    PPdev=s;
#else
    error ("HD44780: bad port '%s' in %s", s, cfg_file());
    return -1;
#endif
  }
  
#ifdef USE_OLD_UDELAY
  s=cfg_get ("Delay",NULL);
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Delay' entry in %s", cfg_file());
    return -1;
  }
  if ((loops_per_usec=strtol(s, &e, 0))==0 || *e!='\0') {
    error ("HD44780: bad delay '%s' in %s", s, cfg_file());
    return -1;
  }    
#endif
  
  s=cfg_get("Size",NULL);
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Size' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("HD44780: bad size '%s'",s);
    return -1;
  }

  s=cfg_get ("GPOs",NULL);
  if (s==NULL) {
    gpos=0;
  }
  else if ((gpos=strtol(s, &e, 0))==0 || *e!='\0' || gpos<0 || gpos>8) {
    error ("HD44780: bad GPOs '%s' in %s", s, cfg_file());
    return -1;
  }    
  
  Self->rows=rows;
  Self->cols=cols;
  Self->gpos=gpos;
  Lcd=*Self;
  
#ifndef USE_OLD_UDELAY
  udelay_init();
#endif

  if (HD_open()!=0)
    return -1;
  
  bar_init(rows, cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block
  
  HD_clear();
  
  return 0;
}


void HD_goto (int row, int col)
{
  int pos;

  pos=(row%2)*64+(row/2)*20+col;
  HD_command (0x80|pos, 40);
}


int HD_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int HD_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int HD_gpo (int num, int val)
{
  if (num>=Lcd.gpos) 
    return -1;

  if (val) {
    GPO |= 1<<num;     // set bit
  } else {
    GPO &= ~(1<<num);  // clear bit
  }
  return 0;
}


int HD_flush (void)
{
  char buffer[256];
  char *p;
  int c, row, col;
  
  bar_process(HD_define_char);

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	Txt[row][col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      HD_goto (row, col);
      for (p=buffer; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      HD_write (buffer, p-buffer, 40); // 40 usec delay for write
    }
  }

  HD_setGPO(GPO);

  return 0;
}


int HD_quit (void)
{
#ifdef WITH_PPDEV
  if (PPdev) {
    debug ("closing ppdev %s", PPdev);
    if (ioctl(PPfd, PPRELEASE)) {
      error ("HD44780: ioctl(%s, PPRELEASE) failed: %s", PPdev, strerror(errno));
    }
    if (close(PPfd)==-1) {
      error ("HD44780: close(%s) failed: %s", PPdev, strerror(errno));
      return -1;
    }
  } else 
#endif    
   {
    debug ("closing raw port 0x%x", Port);
    if (ioperm(Port, 3, 0)!=0) {
      error ("HD44780: ioperm(0x%x) failed: %s", Port, strerror(errno));
      return -1;
    }
  }
  return 0;
}


LCD HD44780[] = {
  { name: "HD44780",
    rows:  0,
    cols:  0,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  HD_init,
    clear: HD_clear,
    put:   HD_put,
    bar:   HD_bar,
    gpo:   HD_gpo,
    flush: HD_flush,
    quit:  HD_quit 
  },
  { NULL }
};
