/* $Id: BeckmannEgle.c,v 1.4 2000/08/10 09:44:09 reinelt Exp $
 *
 * driver for Beckmann+Egle mini terminals
 *
 * Copyright 2000 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: BeckmannEgle.c,v $
 * Revision 1.4  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.3  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.2  2000/04/30 06:40:42  reinelt
 *
 * bars for Beckmann+Egle driver
 *
 * Revision 1.1  2000/04/28 05:19:55  reinelt
 *
 * first release of the Beckmann+Egle driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD BeckmannEgle[]
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "lock.h"
#include "display.h"

#define XRES 5
#define YRES 8
#define CHARS 8
#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 )

typedef struct {
  int cols;
  int rows;
  int type;
} MODEL;

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
static char *Port=NULL;
static int Device=-1;
static int Type=-1;

static MODEL Model[]= {{ 16, 1,  0 },
		       { 16, 2,  1 },
		       { 16, 4,  2 },
		       { 20, 1,  3 },
		       { 20, 2,  4 },
		       { 20, 4,  5 },
		       { 24, 1,  6 },
		       { 24, 2,  7 },
		       { 32, 1,  8 },
		       { 32, 2,  9 },
		       { 40, 1, 10 },
		       { 40, 2, 11 },
		       { 40, 4, 12 },
		       {  0, 0,  0 }};

static char Txt[4][40];
static BAR  Bar[4][40];

static int nSegment=2;
static SEGMENT Segment[128] = {{ len1:0,   len2:0,   type:255, used:0, ascii:32 },
			       { len1:255, len2:255, type:255, used:0, ascii:255 }};


static int BE_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("BeckmannEgle: port %s could not be locked", Port);
    else
      error ("BeckmannEgle: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("BeckmannEgle: open(%s) failed: %s", Port, strerror(errno));
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("BeckmannEgle: tcgetattr(%s) failed: %s", Port, strerror(errno));
    return -1;
  }
  cfmakeraw(&portset);           // 8N1
  portset.c_cflag |= CSTOPB;     // 2 stop bits
  cfsetospeed(&portset, B9600);  // 9600 baud
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("BeckmannEgle: tcsetattr(%s) failed: %s", Port, strerror(errno));
    return -1;
  }
  return fd;
}

static void BE_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("BeckmannEgle: write(%s) failed: %s", Port, strerror(errno));
  }
}

static void BE_process_bars (void)
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

static int BE_segment_diff (int i, int j)
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

static void BE_compact_bars (void)
{
  int i, j, r, c, min;
  int pack_i, pack_j;
  int pass1=1;
  int deviation[nSegment][nSegment];
  
  if (nSegment>CHARS+2) {

    for (i=2; i<nSegment; i++) {
      for (j=0; j<nSegment; j++) {
	deviation[i][j]=BE_segment_diff(i,j);
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
	  error ("BeckmannEgle: unable to compact bar characters");
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

static void BE_define_chars (void)
{
  int c, i, j;
  char buffer[8];
  char cmd[32];
  char *p;
  
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
    p=cmd;
    *p++='\033';
    *p++='&';
    *p++='T';          // enter transparent mode
    *p++='\0';         // write cmd
    *p++=0x40|8*c;     // write CGRAM
    for (i=0; i<YRES; i++) {
      *p++='\1';       // write data
      *p++=buffer[i];  // character bitmap
    }
    *p++='\377';       // leave transparent mode
    BE_write (cmd, p-cmd);
  }
}

int BE_clear (void)
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
  BE_write ("\033&#", 3);
  return 0;
}

int BE_init (LCD *Self)
{
  int i, rows=-1, cols=-1;
  char *port, *s;
  char buffer[5];

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port");
  if (port==NULL || *port=='\0') {
    error ("BeckmannEgle: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  Port=strdup(port);

  s=cfg_get("Type");
  if (s==NULL || *s=='\0') {
    error ("BeckmannEgle: no 'Type' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("BeckmannEgle: bad type '%s'", s);
    return -1;
  }

  Type=-1;
  for (i=0; Model[i].cols; i++) {
    if (Model[i].cols==cols && Model[i].rows==rows) {
      Type=Model[i].type;
      break;
    }
  }
  if (Type==-1) {
    error ("BeckmannEgle: unsupported model '%dx%d'", cols, rows);
    return -1;
  }

  debug ("using %dx%d display at port %s", cols, rows, Port);

  Self->rows=rows;
  Self->cols=cols;
  Lcd=*Self;

  Device=BE_open();
  if (Device==-1) return -1;

  snprintf (buffer, sizeof(buffer), "\033&s%c", Type);
  BE_write (buffer, 4);    // select display type
  BE_write ("\033&D", 3);  // cursor off

  BE_clear();

  return 0;
}

int BE_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}

int BE_bar (int type, int row, int col, int max, int len1, int len2)
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

int BE_flush (void)
{
  char buffer[256]="\033[y;xH";
  char *p;
  int s, row, col;
  
  BE_process_bars();
  BE_compact_bars();
  BE_define_chars();
  
  for (s=0; s<nSegment; s++) {
    Segment[s].used=0;
  }

  for (row=0; row<Lcd.rows; row++) {
    buffer[2]=row;
    for (col=0; col<Lcd.cols; col++) {
      s=Bar[row][col].segment;
      if (s!=-1) {
	Segment[s].used=1;
	Txt[row][col]=Segment[s].ascii;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      buffer[4]=col;
      for (p=buffer+6; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      BE_write (buffer, p-buffer);
    }
  }
  return 0;
}

int BE_quit (void)
{
  debug ("closing port %s", Port);
  close (Device);
  unlock_port(Port);
  return 0;
}

LCD BeckmannEgle[] = {
  { "BLC100x", 0,  0, XRES, YRES, BARS, BE_init, BE_clear, BE_put, BE_bar, BE_flush, BE_quit },
  { NULL }
};
