/* $Id: HD44780.c,v 1.16 2001/03/14 15:30:53 reinelt Exp $
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
#include "udelay.h"

#define XRES 5
#define YRES 8
#define CHARS 8
#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 )

typedef struct {
  int len1;
  int len2;
  int type;
  int segment;
} BAR;

typedef struct {
  int len1;
  int len2;
  int type;
  int used;
  int ascii;
} SEGMENT;

static LCD Lcd;


static unsigned short Port=0;

static char *PPdev=NULL;
static int   PPfd=-1;

static char Txt[4][40];
static BAR  Bar[4][40];
static int  GPO=0;

static int nSegment=2;
static SEGMENT Segment[128] = {{ len1:0,   len2:0,   type:255, used:0, ascii:32 },
			       { len1:255, len2:255, type:255, used:0, ascii:255 }};

#ifdef WITH_PPDEV
static void HD_toggle (int bit)
{
  struct ppdev_frob_struct frob;

  frob.mask=bit;
  frob.val=0; // rise (inverted!)
  ioctl (PPfd, PPFCONTROL, &frob);
  udelay(1);
  frob.val=bit; // lower (inverted!)
  ioctl (PPfd, PPFCONTROL, &frob);
}
#endif

static void HD_command (unsigned char cmd, int delay)
{
#ifdef WITH_PPDEV
  if (PPdev) {

    struct ppdev_frob_struct frob;
    
    // clear RS (inverted)
    frob.mask=PARPORT_CONTROL_AUTOFD;
    frob.val=PARPORT_CONTROL_AUTOFD;
    ioctl (PPfd, PPFCONTROL, &frob);
    
    // put data on DB1..DB8
    ioctl(PPfd, PPWDATA, &cmd);
    
    // send command
    HD_toggle(PARPORT_CONTROL_STROBE);
    
    // wait
    udelay(delay);

  } else {

#endif

    outb (cmd, Port);    // put data on DB1..DB8
    outb (0x02, Port+2); // set Enable = bit 0 invertet
    udelay (1);
    outb (0x03, Port+2); // clear Enable
    udelay (delay);
    
#ifdef WITH_PPDEV
  }
#endif
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
    
    while (len--) {

      // put data on DB1..DB8
      ioctl(PPfd, PPWDATA, string++);
      
      udelay(1);
      // send command
      HD_toggle(PARPORT_CONTROL_STROBE);

      // wait
      udelay(delay);
    }
    
  } else {

#endif

    while (len--) {
      outb (*string++, Port); // put data on DB1..DB8
      outb (0x00, Port+2);    // set Enable = bit 0 invertet
      udelay (1);
      outb (0x01, Port+2);    // clear Enable
      udelay (delay);
    }
#ifdef WITH_PPDEV
  }
#endif
}
static void HD_setGPO (int bits)
{
  if (Lcd.gpos>0) {

#ifdef WITH_PPDEV
    if (PPdev) {

      // put data on DB1..DB8
      ioctl(PPfd, PPWDATA, &bits);
      
      // toggle INIT
      HD_toggle(PARPORT_CONTROL_INIT);

    } else {
#endif

      outb (bits, Port);    // put data on DB1..DB8
      outb (0x05, Port+2);  // set INIT = bit 2 invertet
      udelay (1);
      outb (0x03, Port+2);  // clear INIT
      udelay (1);

#ifdef WITH_PPDEV
    }
#endif
  }
}

static int HD_open (void)
{

#ifdef WITH_PPDEV
  if (PPdev) {
    debug ("using ppdev %s", PPdev);
    PPfd=open(PPdev, O_RDWR);
    if (PPfd==-1) {
      error ("open(%s) failed: %s", PPdev, strerror(errno));
      return -1;
    }
#if 0
    if (ioctl(PPfd, PPEXCL)) {
      error ("ioctl(%s, PPEXCL) failed: %s", PPdev, strerror(errno));
      return -1;
    }
#endif
    if (ioctl(PPfd, PPCLAIM)) {
      error ("ioctl(%s, PPCLAIM) failed: %s", PPdev, strerror(errno));
      return -1;
    }
  } else {
#endif
    debug ("using raw port 0x%x", Port);
    if (ioperm(Port, 3, 1)!=0) {
      error ("HD44780: ioperm(0x%x) failed: %s", Port, strerror(errno));
      return -1;
    }
#ifdef WITH_PPDEV
  }
#endif

  HD_command (0x30, 4100); // 8 Bit mode, wait 4.1 ms
  HD_command (0x30, 100);  // 8 Bit mode, wait 100 us
  HD_command (0x30, 4100); // 8 Bit mode, wait 4.1 ms
  HD_command (0x38, 40);   // 8 Bit mode, 1/16 duty cycle, 5x8 font
  HD_command (0x08, 40);   // Display off, cursor off, blink off
  HD_command (0x0c, 1640); // Display on, cursor off, blink off, wait 1.64 ms
  HD_command (0x06, 40);   // curser moves to right, no shift
  return 0;
}

static void HD_process_bars (void)
{
  int row, col;
  int i, j;
  
  for (i=2; i<nSegment && Segment[i].used; i++);
  for (j=i+1; j<nSegment; j++) {
    if (Segment[j].used)
      Segment[i++]=Segment[j];
  }
  nSegment=i;
  
  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      if (Bar[row][col].type==0) continue;
      for (i=0; i<nSegment; i++) {
	if (Segment[i].type & Bar[row][col].type &&
	    Segment[i].len1== Bar[row][col].len1 &&
	    Segment[i].len2== Bar[row][col].len2) break;
      }
      if (i==nSegment) {
	nSegment++;
	Segment[i].len1=Bar[row][col].len1;
	Segment[i].len2=Bar[row][col].len2;
	Segment[i].type=Bar[row][col].type;
	Segment[i].used=0;
	Segment[i].ascii=-1;
      }
      Bar[row][col].segment=i;
    }
  }
}

static int HD_segment_diff (int i, int j)
{
  int RES;
  int i1, i2, j1, j2;
  
  if (i==j) return 65535;
  if (!(Segment[i].type & Segment[j].type)) return 65535;
  if (Segment[i].len1==0 && Segment[j].len1!=0) return 65535;
  if (Segment[i].len2==0 && Segment[j].len2!=0) return 65535;
  RES=Segment[i].type & BAR_H ? XRES:YRES;
  if (Segment[i].len1>=RES && Segment[j].len1<RES) return 65535;
  if (Segment[i].len2>=RES && Segment[j].len2<RES) return 65535;
  if (Segment[i].len1==Segment[i].len2 && Segment[j].len1!=Segment[j].len2) return 65535;

  i1=Segment[i].len1; if (i1>RES) i1=RES;
  i2=Segment[i].len2; if (i2>RES) i2=RES;
  j1=Segment[j].len1; if (j1>RES) i1=RES;
  j2=Segment[j].len2; if (j2>RES) i2=RES;
  
  return (i1-i2)*(i1-i2)+(j1-j2)*(j1-j2);
}

static void HD_compact_bars (void)
{
  int i, j, r, c, min;
  int pack_i, pack_j;
  int pass1=1;
  int deviation[nSegment][nSegment];
  
  if (nSegment>CHARS+2) {

    for (i=2; i<nSegment; i++) {
      for (j=0; j<nSegment; j++) {
	deviation[i][j]=HD_segment_diff(i,j);
      }
    }
    
    while (nSegment>CHARS+2) {
      min=65535;
      pack_i=-1;
      pack_j=-1;
      for (i=2; i<nSegment; i++) {
	if (pass1 && Segment[i].used) continue;
	for (j=0; j<nSegment; j++) {
	  if (deviation[i][j]<min) {
	    min=deviation[i][j];
	    pack_i=i;
	    pack_j=j;
	  }
	}
      }
      if (pack_i==-1) {
	if (pass1) {
	  pass1=0;
	  continue;
	} else {
	  error ("HD44780: unable to compact bar characters");
	  nSegment=CHARS;
	  break;
	}
      } 
      
      nSegment--;
      Segment[pack_i]=Segment[nSegment];
      
      for (i=0; i<nSegment; i++) {
	deviation[pack_i][i]=deviation[nSegment][i];
	deviation[i][pack_i]=deviation[i][nSegment];
      }
      
      for (r=0; r<Lcd.rows; r++) {
	for (c=0; c<Lcd.cols; c++) {
	  if (Bar[r][c].segment==pack_i)
	    Bar[r][c].segment=pack_j;
	  if (Bar[r][c].segment==nSegment)
	    Bar[r][c].segment=pack_i;
	}
      }
    }
  }
}

static void HD_define_chars (void)
{
  int c, i, j;
  char buffer[8];

  for (i=2; i<nSegment; i++) {
    if (Segment[i].used) continue;
    if (Segment[i].ascii!=-1) continue;
    for (c=0; c<CHARS; c++) {
      for (j=2; j<nSegment; j++) {
	if (Segment[j].ascii==c) break;
      }
      if (j==nSegment) break;
    }
    Segment[i].ascii=c;
    switch (Segment[i].type) {
    case BAR_L:
      for (j=0; j<4; j++) {
	char Pixel[] = { 0, 1, 3, 7, 15, 31 };
	buffer[j]=Pixel[Segment[i].len1];
	buffer[j+4]=Pixel[Segment[i].len2];
      }
      break;
    case BAR_R:
      for (j=0; j<4; j++) {
	char Pixel[] = { 0, 16, 24, 28, 30, 31 };
	buffer[j]=Pixel[Segment[i].len1];
	buffer[j+4]=Pixel[Segment[i].len2];
      }
      break;
    case BAR_U:
      for (j=0; j<Segment[i].len1; j++) {
	buffer[7-j]=31;
      }
      for (; j<YRES; j++) {
	buffer[7-j]=0;
      }
      break;
    case BAR_D:
      for (j=0; j<Segment[i].len1; j++) {
	buffer[j]=31;
      }
      for (; j<YRES; j++) {
	buffer[j]=0;
      }
      break;
    }
    HD_command (0x40|8*c, 40);
    HD_write (buffer, 8, 120); // 120 usec delay for CG RAM write
  }
}

int HD_clear (void)
{
  int row, col;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
      Bar[row][col].len1=-1;
      Bar[row][col].len2=-1;
      Bar[row][col].type=0;
      Bar[row][col].segment=-1;
    }
  }
  GPO=0;
  HD_setGPO (GPO);         // All GPO's off
  HD_command (0x01, 1640); // clear display
  return 0;
}

int HD_init (LCD *Self)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s, *e;
  
  s=cfg_get ("Port");
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
  s=cfg_get ("Delay");
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Delay' entry in %s", cfg_file());
    return -1;
  }
  if ((loops_per_usec=strtol(s, &e, 0))==0 || *e!='\0') {
    error ("HD44780: bad delay '%s' in %s", s, cfg_file());
    return -1;
  }    
#endif
  
  s=cfg_get("Size");
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Size' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("HD44780: bad size '%s'",s);
    return -1;
  }

  s=cfg_get ("GPOs");
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
  int rev=0;
  
  if (len1<1) len1=1;
  else if (len1>max) len1=max;
  
  if (len2<1) len2=1;
  else if (len2>max) len2=max;
  
  switch (type) {
  case BAR_L:
    len1=max-len1;
    len2=max-len2;
    rev=1;
    
  case BAR_R:
    while (max>0 && col<=Lcd.cols) {
      Bar[row][col].type=type;
      Bar[row][col].segment=-1;
      if (len1>=XRES) {
	Bar[row][col].len1=rev?0:XRES;
	len1-=XRES;
      } else {
	Bar[row][col].len1=rev?XRES-len1:len1;
	len1=0;
      }
      if (len2>=XRES) {
	Bar[row][col].len2=rev?0:XRES;
	len2-=XRES;
      } else {
	Bar[row][col].len2=rev?XRES-len2:len2;
	len2=0;
      }
      max-=XRES;
      col++;
    }
    break;

  case BAR_U:
    len1=max-len1;
    len2=max-len2;
    rev=1;
    
  case BAR_D:
    while (max>0 && row<=Lcd.rows) {
      Bar[row][col].type=type;
      Bar[row][col].segment=-1;
      if (len1>=YRES) {
	Bar[row][col].len1=rev?0:YRES;
	len1-=YRES;
      } else {
	Bar[row][col].len1=rev?YRES-len1:len1;
	len1=0;
      }
      if (len2>=YRES) {
	Bar[row][col].len2=rev?0:YRES;
	len2-=YRES;
      } else {
	Bar[row][col].len2=rev?YRES-len2:len2;
	len2=0;
      }
      max-=YRES;
      row++;
    }
    break;

  }
  return 0;
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
  int s, row, col;
  
  HD_process_bars();
  HD_compact_bars();
  HD_define_chars();
  
  for (s=0; s<nSegment; s++) {
    Segment[s].used=0;
  }

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      s=Bar[row][col].segment;
      if (s!=-1) {
	Segment[s].used=1;
	Txt[row][col]=Segment[s].ascii;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      HD_goto (row, col);
      for (p=buffer; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      HD_write (buffer, p-buffer, 40);
    }
  }

  HD_setGPO(GPO);

  return 0;
}

int HD_quit (void)
{
  if (PPdev) {
    debug ("closing ppdev %s", PPdev);
    if (close(PPfd)==-1) {
      error ("close(%s) failed: %s", PPdev, strerror(errno));
      return -1;
    }
  } else {
    debug ("closing raw port 0x%x", Port);
    if (ioperm(Port, 3, 0)!=0) {
      error ("HD44780: ioperm(0x%x) failed: %s", Port, strerror(errno));
      return -1;
    }
  }
  return 0;
}

LCD HD44780[] = {
  { "HD44780",0,0,XRES,YRES,BARS,0,HD_init,HD_clear,HD_put,HD_bar,HD_gpo,HD_flush,HD_quit },
  { NULL }
};
