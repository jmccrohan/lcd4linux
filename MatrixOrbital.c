/* $Id: MatrixOrbital.c,v 1.5 2000/03/13 15:58:24 reinelt Exp $
 *
 *  driver for Matrix Orbital serial display modules
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
 * $Log: MatrixOrbital.c,v $
 * Revision 1.5  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.4  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DISPLAY MatrixOrbital[]
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "cfg.h"
#include "display.h"

#define SPEED 19200
#define XRES 5
#define YRES 8
#define CHARS 8
#define BARS ( BAR_L | BAR_R | BAR_H2 )
// Fixme: BAR_U, BAR_D

static DISPLAY Display;
static char *Port=NULL;
static speed_t Speed;
static int Device=-1;

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

static char Txt[4][40];
static BAR  Bar[4][40];

static int nSegment=2;
static SEGMENT Segment[256] = {{ len1:   0, len2:   0, type:255, used:0, ascii:32 },
			       { len1:XRES, len2:XRES, type:255, used:0, ascii:255 }};


static int MO_open (void)
{
  int fd;
  struct termios portset;
  
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    fprintf (stderr, "MatrixOrbital: open(%s) failed: %s\n", Port, strerror(errno));
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    fprintf (stderr, "MatrixOrbital: tcgetattr(%s) failed: %s\n", Port, strerror(errno));
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    fprintf (stderr, "MatrixOrbital: tcsetattr(%s) failed: %s\n", Port, strerror(errno));
    return -1;
  }
  return fd;
}

static void MO_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    fprintf (stderr, "MatrixOrbital: write(%s) failed: %s\n", Port, strerror(errno));
  }
}

static int MO_contrast (void)
{
  char buffer[4];
  int  contrast;

  contrast=atoi(cfg_get("Contrast"));
  if (contrast==0) contrast=160;
  snprintf (buffer, 4, "\376P%c", contrast);
  MO_write (buffer, 3);
  return 0;
}

static void MO_process_bars (void)
{
  int row, col;
  int i, j;
  
  for (i=2; i<nSegment && Segment[i].used; i++);
  for (j=i+1; j<nSegment; j++) {
    if (Segment[j].used)
      Segment[i++]=Segment[j];
  }
  nSegment=i;
  
  for (row=0; row<Display.rows; row++) {
    for (col=0; col<Display.cols; col++) {
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

#define sqr(x) ((x)*(x))

static void MO_compact_bars (void)
{
  int i, j, r, c, min;
  int pack_i, pack_j;
  int error[nSegment][nSegment];
  
  if (nSegment>CHARS+2) {
    for (i=2; i<nSegment; i++) {
      for (j=0; j<nSegment; j++) {
	error[i][j]=65535;
	if (i==j) continue;
	if (Segment[i].used) continue;
	if (!(Segment[i].type & Segment[j].type)) continue;
	if (Segment[i].len1==0 && Segment[j].len1!=0) continue;
	if (Segment[i].len2==0 && Segment[j].len2!=0) continue;
	if (Segment[i].len1==XRES && Segment[j].len1!=XRES) continue;
	if (Segment[i].len2==XRES && Segment[j].len2!=XRES) continue;
	if (Segment[i].len1==Segment[i].len2 && Segment[j].len1!=Segment[j].len2) continue;
	error[i][j]=sqr(Segment[i].len1-Segment[j].len1)+sqr(Segment[i].len2-Segment[j].len2);
      }
    }

    while (nSegment>CHARS+2) {
      min=65535;
      pack_i=-1;
      pack_j=-1;
      for (i=2; i<nSegment; i++) {
	for (j=0; j<nSegment; j++) {
	  if (error[i][j]<min) {
	    min=error[i][j];
	    pack_i=i;
	    pack_j=j;
	  }
	}
      }
      if (pack_i==-1) {
	fprintf (stderr, "MatrixOrbital: unable to compact bar characters\n");
	nSegment=CHARS;
	break;
      } 

      nSegment--;
      Segment[pack_i]=Segment[nSegment];
      
      for (i=0; i<nSegment; i++) {
	error[pack_i][i]=error[nSegment][i];
	error[i][pack_i]=error[i][nSegment];
      }

      for (r=0; r<Display.rows; r++) {
	for (c=0; c<Display.cols; c++) {
	  if (Bar[r][c].segment==pack_i)
	    Bar[r][c].segment=pack_j;
	  if (Bar[r][c].segment==nSegment)
	    Bar[r][c].segment=pack_i;
	}
      }
    }
  }
}

static void MO_define_chars (void)
{
  int c, i, j;
  char buffer[12]="\376N";

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
    buffer[2]=c;
    switch (Segment[i].type & (BAR_L | BAR_R | BAR_U | BAR_D)) {
    case BAR_L:
      for (j=0; j<4; j++) {
	char Pixel[] = { 0, 1, 3, 7, 15, 31 };
	buffer[j+3]=Pixel[Segment[i].len1];
	buffer[j+7]=Pixel[Segment[i].len2];
      }
      break;
    case BAR_R:
      for (j=0; j<4; j++) {
	char Pixel[] = { 0, 16, 24, 28, 30, 31 };
	buffer[j+3]=Pixel[Segment[i].len1];
	buffer[j+7]=Pixel[Segment[i].len2];
      }
      break;
    }
    MO_write (buffer, 11);
  }
}

int MO_clear (void)
{
  int row, col;

  for (row=0; row<Display.rows; row++) {
    for (col=0; col<Display.cols; col++) {
      Txt[row][col]='\t';
      Bar[row][col].len1=-1;
      Bar[row][col].len2=-1;
      Bar[row][col].type=0;
      Bar[row][col].segment=-1;
    }
  }
  MO_write ("\014", 1);
  return 0;
}

int MO_init (DISPLAY *Self)
{
  char *port;
  char *speed;

  Display=*Self;

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port");
  if (port==NULL || *port=='\0') {
    fprintf (stderr, "MatrixOrbital: no 'Port' entry in %s\n", cfg_file());
    return -1;
  }
  Port=strdup(port);

  speed=cfg_get("Speed");
  if (speed==NULL) speed="19200";
  
  switch (atoi(speed)) {
  case 1200:
    Speed=B1200;
    break;
  case 2400:
    Speed=B2400;
    break;
  case 9600:
    Speed=B9600;
    break;
  case 19200:
    Speed=B19200;
    break;
  default:
    fprintf (stderr, "MatrixOrbital: unsupported speed '%s' in %s\n", speed, cfg_file());
    return -1;
  }    

  Device=MO_open();
  if (Device==-1) return -1;

  MO_clear();
  MO_contrast();

  MO_write ("\376B", 3);  // backlight on
  MO_write ("\376K", 2);  // cursor off
  MO_write ("\376T", 2);  // blink off
  MO_write ("\376D", 2);  // line wrapping off
  MO_write ("\376R", 2);  // auto scroll off
  MO_write ("\376V", 2);  // GPO off

  return 0;
}

int MO_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Display.cols) {
    *p++=*t++;
  }
  return 0;
}

int MO_bar (int type, int row, int col, int max, int len1, int len2)
{
  int rev=0;
  
  if (len1<1) len1=1;
  else if (len1>max) len1=max;
  
  if (len2<1) len2=1;
  else if (len2>max) len2=max;
  
  switch (type & (BAR_L | BAR_R | BAR_U | BAR_D)) {
  case BAR_L:
    len1=max-len1;
    len2=max-len2;
    rev=1;
    
  case BAR_R:
    while (max>0 && col<=Display.cols) {
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
    break;

  case BAR_D:
    break;

  }
  return 0;
}

int MO_flush (void)
{
  char buffer[256]="\376G";
  char *p;
  int s, row, col;
  
  MO_process_bars();
  MO_compact_bars();
  MO_define_chars();
  
  for (s=0; s<nSegment; s++) {
    Segment[s].used=0;
  }

  for (row=0; row<Display.rows; row++) {
    buffer[3]=row+1;
    for (col=0; col<Display.cols; col++) {
      s=Bar[row][col].segment;
      if (s!=-1) {
	Segment[s].used=1;
	Txt[row][col]=Segment[s].ascii;
      }
    }
    for (col=0; col<Display.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      buffer[2]=col+1;
      for (p=buffer+4; col<Display.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      MO_write (buffer, p-buffer);
    }
  }
  return 0;
}


DISPLAY MatrixOrbital[] = {
  { "LCD0821", 2,  8, XRES, YRES, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD1621", 2, 16, XRES, YRES, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD2021", 2, 20, XRES, YRES, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD2041", 4, 20, XRES, YRES, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD4021", 2, 40, XRES, YRES, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush }, 
  { "" }
};
