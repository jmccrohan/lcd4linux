/* $Id: M50530.c,v 1.3 2002/08/19 04:41:20 reinelt Exp $
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


static char *PPdev=NULL;
static int   PPfd=-1;

static char Txt[8][40];
static BAR  Bar[8][40];
static int  GPO=0;

static int nSegment=1;
static SEGMENT Segment[128] = {{ len1:0, len2:0, type:255, used:0, ascii:32 }};

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
  ndelay(200);

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

static void M5_process_bars (void)
{
  int row, col;
  int i, j;
  
  for (i=1; i<nSegment && Segment[i].used; i++);
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

static int M5_segment_diff (int i, int j)
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

static void M5_compact_bars (void)
{
  int i, j, r, c, min;
  int pack_i, pack_j;
  int pass1=1;
  int deviation[nSegment][nSegment];
  
  if (nSegment>CHARS+1) {

    for (i=1; i<nSegment; i++) {
      for (j=0; j<nSegment; j++) {
	deviation[i][j]=M5_segment_diff(i,j);
      }
    }
    
    while (nSegment>CHARS+1) {
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
	  error ("M50530: unable to compact bar characters");
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

static void M5_define_chars (void)
{
  int c, i, j;
  char buffer[8];

  for (i=1; i<nSegment; i++) {
    if (Segment[i].used) continue;
    if (Segment[i].ascii!=-1) continue;
    for (c=0; c<CHARS; c++) {
      for (j=1; j<nSegment; j++) {
	if (Segment[j].ascii==248+c) break;
      }
      if (j==nSegment) break;
    }
    Segment[i].ascii=248+c;
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
    M5_command (0x300+192+8*c, 20);
    M5_write (buffer, 8);
  }
}

int M5_clear (void)
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
  }
  else if ((gpos=strtol(s, &e, 0))==0 || *e!='\0' || gpos<0 || gpos>8) {
    error ("M50530: bad GPOs '%s' in %s", s, cfg_file());
    return -1;
  }    
  
  Self->rows=rows;
  Self->cols=cols;
  Self->gpos=gpos;
  Lcd=*Self;
  
  udelay_init();

  if (M5_open()!=0)
    return -1;
  
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
  int s, row, col;
  
  M5_process_bars();
  M5_compact_bars();
  M5_define_chars();
  
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
  { "M50530",0,0,XRES,YRES,BARS,0,M5_init,M5_clear,M5_put,M5_bar,M5_gpo,M5_flush,M5_quit },
  { NULL }
};
