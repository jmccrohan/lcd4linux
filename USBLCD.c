/* $Id: USBLCD.c,v 1.2 2002/08/17 14:14:21 reinelt Exp $
 *
 * Driver for USBLCD ( see http://www.usblcd.de )
 * This Driver is based on HD44780.c
 *
 * Copyright 2002 by Robin Adams, Adams IT Services ( info@usblcd.de )
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
 * $Log: USBLCD.c,v $
 * Revision 1.2  2002/08/17 14:14:21  reinelt
 *
 * USBLCD fixes
 *
 * Revision 1.0  2002/07/08 12:16:10  radams
 *
 * first version of the USBLCD driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD UDBLCD[]
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
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <sys/time.h>

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "udelay.h"

#define GET_HARD_VERSION	1
#define GET_DRV_VERSION		2

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

int usblcd_file;

static char *Port=NULL;


static char Txt[4][40];
static BAR  Bar[4][40];

static int nSegment=2;
static SEGMENT Segment[128] = {{ len1:0,   len2:0,   type:255, used:0, ascii:32 },
			       { len1:255, len2:255, type:255, used:0, ascii:255 }};


static void USBLCD_command (unsigned char cmd)
{
  char a=0; 
  
  struct timeval now, end;
  gettimeofday (&now, NULL);

  write(usblcd_file,&a,1);
  write(usblcd_file,&cmd,1);

  gettimeofday (&end, NULL);

  debug ("command %x: %d usec", cmd, 1000000*(end.tv_sec-now.tv_sec)+end.tv_usec-now.tv_usec);
}

static void USBLCD_write (char *string, int len)
{
  int len0=len;
  int dur;
  struct timeval now, end;
  gettimeofday (&now, NULL);
  
#if 0
  while (len--) {
    char a=*string++;
    if(a==0) write(usblcd_file,&a,1);
    write(usblcd_file,&a,1);
  }
#else
  write(usblcd_file,string,len);

#endif
  gettimeofday (&end, NULL);
  dur=1000000*(end.tv_sec-now.tv_sec)+end.tv_usec-now.tv_usec;
  debug ("write (%d): %d usec (%d usec/byte)", len0, dur, dur/len0);

}


static int USBLCD_open (void)
{
  char buf[128];
  int major,minor;

  usblcd_file=open(Port,O_WRONLY);
  if (usblcd_file==-1) {
    error ("USBLCD: open(%s) failed: %s", Port, strerror(errno));
    return -1;
  }

  memset(buf,0,sizeof(buf));

  if( ioctl(usblcd_file,GET_DRV_VERSION, buf)!=0) {
    error ("USBLCD: ioctl() failed, could not get Driver Version!");
    return -2;
  } ;

  debug("Driver Version: %s",buf);

  if( sscanf(buf,"USBLCD Driver Version %d.%d",&major,&minor)!=2) {
    error("USBLCD: could not read Driver Version!");
    return -4;
  };
 
  if(major!=1) {
    error("USBLCD: Driver Version not supported!");
    return -4;
  }

  memset(buf,0,sizeof(buf));

  if( ioctl(usblcd_file,GET_HARD_VERSION, buf)!=0) {
    error ("USBLCD: ioctl() failed, could not get Hardware Version!");
    return -3;
  } ;

  debug("Hardware Version: %s",buf);

  if( sscanf(buf,"%d.%d",&major,&minor)!=2) {
    error("USBLCD: could not read Hardware Version!");
    return -4;
  };

  if(major!=1) {
    error("USBLCD: Hardware Version not supported!");
    return -4;
  }

  USBLCD_command (0x29); // 8 Bit mode, 1/16 duty cycle, 5x8 font
  USBLCD_command (0x08); // Display off, cursor off, blink off
  USBLCD_command (0x0c); // Display on, cursor off, blink off
  USBLCD_command (0x06); // curser moves to right, no shift

  return 0;
}

static void USBLCD_process_bars (void)
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

static int USBLCD_segment_diff (int i, int j)
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

static void USBLCD_compact_bars (void)
{
  int i, j, r, c, min;
  int pack_i, pack_j;
  int pass1=1;
  int deviation[nSegment][nSegment];
  
  if (nSegment>CHARS+2) {

    for (i=2; i<nSegment; i++) {
      for (j=0; j<nSegment; j++) {
	deviation[i][j]=USBLCD_segment_diff(i,j);
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
	  error ("USBLCD: unable to compact bar characters");
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

static void USBLCD_define_chars (void)
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
    USBLCD_command (0x40|8*c);
    USBLCD_write (buffer, 8);
  }
}

int USBLCD_clear (void)
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
  USBLCD_command (0x01); // clear display
  return 0;
}

int USBLCD_init (LCD *Self)
{
  int rows=-1, cols=-1 ;
  char *port,*s ;

  if (Port) {
    free(Port);
    Port=NULL;
  }
  if ((port=cfg_get("Port"))==NULL || *port=='\0') {
    error ("USBLCD: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  if (port[0]=='/') {
    Port=strdup(port);
  } else {
    Port=(char *)malloc(5/*/dev/ */+strlen(port)+1);
    sprintf(Port,"/dev/%s",port);
  }

  debug ("using device %s ", Port);

  s=cfg_get("Size");
  if (s==NULL || *s=='\0') {
    error ("USBLCD: no 'Size' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("USBLCD: bad size '%s'",s);
    return -1;
  }

  Self->rows=rows;
  Self->cols=cols;
  Lcd=*Self;
  
  if (USBLCD_open()!=0)
    return -1;
  
  USBLCD_clear();
  
  return 0;
}

void USBLCD_goto (int row, int col)
{
  int pos=(row%2)*64+(row/2)*20+col;
  USBLCD_command (0x80|pos);
}

int USBLCD_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}

int USBLCD_bar (int type, int row, int col, int max, int len1, int len2)
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

int USBLCD_flush (void)
{
  char buffer[256];
  char *p;
  int s, row, col;
  
  USBLCD_process_bars();
  USBLCD_compact_bars();
  USBLCD_define_chars();
  
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
      USBLCD_goto (row, col);
      for (p=buffer; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      USBLCD_write (buffer, p-buffer);
    }
  }


  return 0;
}

int USBLCD_quit (void)
{
  debug ("closing port %s", Port);
  close(usblcd_file);
  return 0;
}

LCD USBLCD[] = {
  { "USBLCD",0,0,XRES,YRES,BARS,0,USBLCD_init,USBLCD_clear,USBLCD_put,USBLCD_bar,NULL,USBLCD_flush,USBLCD_quit },
  { NULL }
};
