/* $Id: M50530.c,v 1.5 2002/08/19 10:51:06 reinelt Exp $
 *
 * driver for display modules based on the M50530 chip
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
 * $Log: M50530.c,v $
 * Revision 1.5  2002/08/19 10:51:06  reinelt
 * M50530 driver using new generic bar functions
 *
 * Revision 1.4  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.3  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.2  2002/04/30 07:20:15  reinelt
 *
 * implemented the new ndelay(nanoseconds) in all parallel port drivers
 *
 * Revision 1.1  2001/09/11 05:31:37  reinelt
 * M50530 driver
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD M50530[]
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

#if defined (HAVE_LINUX_PARPORT_H) && defined (HAVE_LINUX_PPDEV_H)
#define WITH_PPDEV
#include <linux/parport.h>
#include <linux/ppdev.h>
#else
#error The M50530 driver needs ppdev
#error cannot compile M50530 driver
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


static char *PPdev=NULL;
static int   PPfd=-1;

static char Txt[8][40];
static int  GPO=0;


static void M5_command (unsigned int cmd, int delay)
{
  unsigned char data;
  struct ppdev_frob_struct frob;
    
  // put data on DB1..DB8
  data=cmd&0xff;
  ioctl(PPfd, PPWDATA, &data);
    
  // set I/OC1 (Select inverted)
  // set I/OC2 (AutoFeed inverted)
  frob.mask=PARPORT_CONTROL_SELECT | PARPORT_CONTROL_AUTOFD;
  frob.val=0;
  if (!(cmd & 0x200)) {
    frob.val|=PARPORT_CONTROL_SELECT;
  }
  if (!(cmd & 0x100)) {
    frob.val|=PARPORT_CONTROL_AUTOFD;
  }
  ioctl (PPfd, PPFCONTROL, &frob);

  // Control data setup time
  ndelay(200);

  // rise EX (Strobe, inverted)
  frob.mask=PARPORT_CONTROL_STROBE;
  frob.val=0;
  ioctl (PPfd, PPFCONTROL, &frob);
    
  // EX signal pulse width
  // Fixme: why 500 ns? Datasheet says 200ns
  ndelay(500);

  // lower EX (Strobe, inverted)
  frob.mask=PARPORT_CONTROL_STROBE;
  frob.val=PARPORT_CONTROL_STROBE;
  ioctl (PPfd, PPFCONTROL, &frob);
    
  // wait
  udelay(delay);

}


static void M5_write (unsigned char *string, int len)
{
  unsigned int cmd;

  while (len--) {
    cmd=*string++;
    M5_command (0x100|cmd, 20);
  }
}


static void M5_setGPO (int bits)
{
  if (Lcd.gpos>0) {

    struct ppdev_frob_struct frob;
  
    // put data on DB1..DB8
    ioctl(PPfd, PPWDATA, &bits);

    // 74HCT573 set-up time
    ndelay(20);
    
    // toggle INIT
    frob.mask=PARPORT_CONTROL_INIT;
    frob.val=PARPORT_CONTROL_INIT; // rise
    ioctl (PPfd, PPFCONTROL, &frob);

    // 74HCT573 enable pulse width
    ndelay(24);

    frob.val=0; // lower
    ioctl (PPfd, PPFCONTROL, &frob);
  }
}


static int M5_open (void)
{

  debug ("using ppdev %s", PPdev);
  PPfd=open(PPdev, O_RDWR);
  if (PPfd==-1) {
    error ("open(%s) failed: %s", PPdev, strerror(errno));
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
    error ("ioctl(%s, PPCLAIM) failed: %d %s", PPdev, errno, strerror(errno));
    return -1;
  }

  M5_command (0x00FA, 20); // set function mode
  M5_command (0x0020, 20); // set display mode
  M5_command (0x0050, 20); // set entry mode
  M5_command (0x0030, 20); // set display mode
  M5_command (0x0001, 1250); // clear display

  return 0;
}


static void M5_define_char (int ascii, char *buffer)
{
  M5_command (0x300+192+8*ascii, 20);
  M5_write (buffer, 8);
}


int M5_clear (void)
{
  int row, col;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
    }
  }

  bar_clear();

  GPO=0;
  M5_setGPO (GPO);           // All GPO's off

  M5_command (0x0001, 1250); // clear display
  return 0;
}


int M5_init (LCD *Self)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s, *e;
  
  if (PPdev) {
    free (PPdev);
    PPdev=NULL;
  }
  
  s=cfg_get ("Port");
  if (s==NULL || *s=='\0') {
    error ("M50530: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  PPdev=strdup(s);
  
  s=cfg_get("Size");
  if (s==NULL || *s=='\0') {
    error ("M50530: no 'Size' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("M50530: bad size '%s'",s);
    return -1;
  }

  s=cfg_get ("GPOs");
  if (s==NULL) {
    gpos=0;
  } else {
    gpos=strtol(s, &e, 0);
    if (*e!='\0' || gpos<0 || gpos>8) {
      error ("M50530: bad GPOs '%s' in %s", s, cfg_file());
      return -1;
    }    
  }
  
  Self->rows=rows;
  Self->cols=cols;
  Self->gpos=gpos;
  Lcd=*Self;
  
  udelay_init();

  if (M5_open()!=0)
    return -1;
  
  bar_init(rows, cols, XRES, YRES, CHARS);
  bar_add_segment(0,0,255,32); // ASCII 32 = blank

  M5_clear();
  
  return 0;
}


void M5_goto (int row, int col)
{
  int pos;

  pos=row*48+col;
  if (row>3) pos-=168;
  M5_command (0x300|pos, 20);
}


int M5_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int M5_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int M5_gpo (int num, int val)
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


int M5_flush (void)
{
  unsigned char buffer[256];
  unsigned char *p;
  int c, row, col;
  
  bar_process(M5_define_char);

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	if (c!=32) c+=248; //blank
	Txt[row][col]=(char)(c);
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      M5_goto (row, col);
      for (p=buffer; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      M5_write (buffer, p-buffer);
    }
  }

  M5_setGPO(GPO);

  return 0;
}


int M5_quit (void)
{
  debug ("closing ppdev %s", PPdev);
  if (ioctl(PPfd, PPRELEASE)) {
    error ("ioctl(%s, PPRELEASE) failed: %s", PPdev, strerror(errno));
  }
  if (close(PPfd)==-1) {
    error ("close(%s) failed: %s", PPdev, strerror(errno));
    return -1;
  }
  return 0;
}


LCD M50530[] = {
  { name: "M50530",
    rows:  0,
    cols:  0,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  M5_init,
    clear: M5_clear,
    put:   M5_put,
    bar:   M5_bar,
    gpo:   M5_gpo,
    flush: M5_flush,
    quit:  M5_quit 
  },
  { NULL }
};
