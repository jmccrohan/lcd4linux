/* $Id: HD44780.c,v 1.2 2000/04/13 06:09:52 reinelt Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <asm/io.h>

#include "cfg.h"
#include "display.h"

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
static unsigned long  Delay;

static char Txt[4][40];
static BAR  Bar[4][40];

static int nSegment=2;
static SEGMENT Segment[128] = {{ len1:0,   len2:0,   type:255, used:0, ascii:32 },
			       { len1:255, len2:255, type:255, used:0, ascii:255 }};

static void HD_delay (unsigned long usec)
{
  unsigned long i=usec*Delay/2;
  while (i--);
}

static void HD_command (unsigned char cmd, int delay)
{
  outb (cmd, Port);    // put data on DB1..DB8
  outb (0x02, Port+2); // set Enable = bit 0 invertet
  HD_delay(1);
  outb (0x03, Port+2); // clear Enable
  HD_delay(delay);
}

static void HD_write (char *string, int len, int delay)
{
  while (len--) {
    outb (*string++, Port); // put data on DB1..DB8
    outb (0x00, Port+2); // set Enable = bit 0 invertet
    HD_delay(1);
    outb (0x01, Port+2); // clear Enable
    HD_delay(delay);
  }
}

static int HD_open (void)
{
  if (ioperm(Port, 3, 1)!=0) {
    fprintf (stderr, "HD44780: ioperm() failed: %s\n", strerror(errno));
    return -1;
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
  int error[nSegment][nSegment];
  
  if (nSegment>CHARS+2) {

    for (i=2; i<nSegment; i++) {
      for (j=0; j<nSegment; j++) {
	error[i][j]=HD_segment_diff(i,j);
      }
    }
    
    while (nSegment>CHARS+2) {
      min=65535;
      pack_i=-1;
      pack_j=-1;
      for (i=2; i<nSegment; i++) {
	if (pass1 && Segment[i].used) continue;
	for (j=0; j<nSegment; j++) {
	  if (error[i][j]<min) {
	    min=error[i][j];
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
	  fprintf (stderr, "HD44780: unable to compact bar characters\n");
	  nSegment=CHARS;
	  break;
	}
      } 
      
      nSegment--;
      Segment[pack_i]=Segment[nSegment];
      
      for (i=0; i<nSegment; i++) {
	error[pack_i][i]=error[nSegment][i];
	error[i][pack_i]=error[i][nSegment];
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
  HD_command (0x01, 1640); // clear display
  return 0;
}

static void HD_quit (int signal); //forward declaration

int HD_init (LCD *Self)
{
  int rows=-1, cols=-1;
  char *s, *e;
  
  s=cfg_get ("Port");
  if (s==NULL || *s=='\0') {
    fprintf (stderr, "HD44780: no 'Port' entry in %s\n", cfg_file());
    return -1;
  }
  if ((Port=strtol(s, &e, 0))==0 || *e!='\0') {
    fprintf (stderr, "HD44780: bad port '%s' in %s\n", s, cfg_file());
    return -1;
  }    

  s=cfg_get("Size");
  if (s==NULL || *s=='\0') {
    fprintf (stderr, "HD44780: no 'Size' entry in %s\n", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    fprintf(stderr,"HD44780: bad size '%s'\n",s);
    return -1;
  }

  s=cfg_get ("Delay");
  if (s==NULL || *s=='\0') {
    fprintf (stderr, "HD44780: no 'Delay' entry in %s\n", cfg_file());
    return -1;
  }
  if ((Delay=strtol(s, &e, 0))==0 || *e!='\0' || Delay<1) {
    fprintf (stderr, "HD44780: bad delay '%s' in %s\n", s, cfg_file());
    return -1;
  }    

  Self->rows=rows;
  Self->cols=cols;
  Lcd=*Self;

  if (HD_open()!=0)
    return -1;
  
  HD_clear();

  signal(SIGINT,  HD_quit);
  signal(SIGQUIT, HD_quit);
  signal(SIGTERM, HD_quit);

  return 0;
}

void HD_goto (int row, int col)
{
  int pos;
  pos=(row%2)*64+col;
  if (row>2) pos+=20;
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
  return 0;
}

int lcd_hello (void); // prototype from lcd4linux.c

static void HD_quit (int signal)
{
  HD_clear();
  lcd_hello();
  // Fixme: ioperm rückgängig machen?
  exit (0);
}

LCD HD44780[] = {
  { "HD44780", 0, 0, XRES, YRES, BARS, HD_init, HD_clear, HD_put, HD_bar, HD_flush },
  { NULL }
};
