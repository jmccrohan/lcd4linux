/* $Id: bar.c,v 1.3 2002/08/19 07:52:19 reinelt Exp $
 *
 * generic bar handling
 *
 * Copyright 2002 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: bar.c,v $
 * Revision 1.3  2002/08/19 07:52:19  reinelt
 * corrected type declaration of (*defchar)()
 *
 * Revision 1.2  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.1  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 *
 */

/* 
 * exported functions:
 *
 * int bar_init (int rows, int cols, int xres, int yres, int chars)
 *
 * void bar_clear(void)
 *
 * void bar_add_segment(int len1, int len2, int type, int ascii)
 *
 * int bar_draw (int type, int row, int col, int max, int len1, int len2)
 *
 * int bar_process (void(*defchar)(int ascii, char *matrix))
 *
 * int bar_peek (int row, int col)
 *
 */

#include <stdlib.h>

#include "bar.h"
#include "debug.h"


static int ROWS=0;
static int COLS=0;
static int XRES=0;
static int YRES=0;
static int CHARS=0;

static int nSegment=0;
static int fSegment=0;
static SEGMENT Segment[128];

static BAR *Bar=NULL;


int bar_init (int rows, int cols, int xres, int yres, int chars)
{
  if (rows<1 || cols<1) 
    return -1;
  
  ROWS=rows;
  COLS=cols;
  XRES=xres;
  YRES=yres;
  CHARS=chars;

  if (Bar) {
    free (Bar);
  }
  
  if ((Bar=malloc (ROWS*COLS*sizeof(BAR)))==NULL) {
    return -1;
  }

  bar_clear();

  nSegment=0;
  fSegment=0;

  return 0;
}


void bar_clear(void)
{
  int n;

  for (n=0; n<ROWS*COLS; n++) {
    Bar[n].len1=-1;
    Bar[n].len2=-1;
    Bar[n].type=0;
    Bar[n].segment=-1;
  }

}


void bar_add_segment(int len1, int len2, int type, int ascii)
{
  Segment[fSegment].len1=len1;
  Segment[fSegment].len2=len2;
  Segment[fSegment].type=type;
  Segment[fSegment].used=0;
  Segment[fSegment].ascii=ascii;
  
  fSegment++;
  nSegment=fSegment;
}


int bar_draw (int type, int row, int col, int max, int len1, int len2)
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
    while (max>0 && col<COLS) {
      Bar[row*COLS+col].type=type;
      Bar[row*COLS+col].segment=-1;
      if (len1>=XRES) {
	Bar[row*COLS+col].len1=rev?0:XRES;
	len1-=XRES;
      } else {
	Bar[row*COLS+col].len1=rev?XRES-len1:len1;
	len1=0;
      }
      if (len2>=XRES) {
	Bar[row*COLS+col].len2=rev?0:XRES;
	len2-=XRES;
      } else {
	Bar[row*COLS+col].len2=rev?XRES-len2:len2;
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
    while (max>0 && row<ROWS) {
      Bar[row*COLS+col].type=type;
      Bar[row*COLS+col].segment=-1;
      if (len1>=YRES) {
	Bar[row*COLS+col].len1=rev?0:YRES;
	len1-=YRES;
      } else {
	Bar[row*COLS+col].len1=rev?YRES-len1:len1;
	len1=0;
      }
      if (len2>=YRES) {
	Bar[row*COLS+col].len2=rev?0:YRES;
	len2-=YRES;
      } else {
	Bar[row*COLS+col].len2=rev?YRES-len2:len2;
	len2=0;
      }
      max-=YRES;
      row++;
    }
    break;

  }
  return 0;
}


static void create_segments (void)
{
  int i, j, n;
  
  /* find first unused segment */
  for (i=fSegment; i<nSegment && Segment[i].used; i++);

  /* pack unused segments */
  for (j=i+1; j<nSegment; j++) {
    if (Segment[j].used)
      Segment[i++]=Segment[j];
  }
  nSegment=i;
  
  /* create needed segments */
  for (n=0; n<ROWS*COLS; n++) {
    if (Bar[n].type==0) continue;
    for (i=0; i<nSegment; i++) {
      if (Segment[i].type & Bar[n].type &&
	  Segment[i].len1== Bar[n].len1 &&
	  Segment[i].len2== Bar[n].len2) break;
    }
    if (i==nSegment) {
      nSegment++;
      Segment[i].len1=Bar[n].len1;
      Segment[i].len2=Bar[n].len2;
      Segment[i].type=Bar[n].type;
      Segment[i].used=0;
      Segment[i].ascii=-1;
    }
    Bar[n].segment=i;
  }
}


static int segment_deviation (int i, int j)
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


static void pack_segments (void)
{
  int i, j, n, min;
  int pack_i, pack_j;
  int pass1=1;
  int deviation[nSegment][nSegment];
  
  if (nSegment<=fSegment+CHARS) {
    return;
  }

  for (i=0; i<nSegment-1; i++) {
    for (j=i+1; j<nSegment; j++) {
      int d=segment_deviation(i,j);
      deviation[i][j]=d;
      deviation[j][i]=d;
    }
  }
    
  while (nSegment>fSegment+CHARS) {
    min=65535;
    pack_i=-1;
    pack_j=-1;
    for (i=fSegment; i<nSegment; i++) {
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
	error ("unable to compact bar characters");
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
      
    for (n=0; n<ROWS*COLS; n++) {
      if (Bar[n].segment==pack_i)   Bar[n].segment=pack_j;
      if (Bar[n].segment==nSegment) Bar[n].segment=pack_i;
    }
  }
}


static void define_chars (void(*defchar)(int ascii, char *matrix))
{
  int c, i, j;
  char buffer[8];

  for (i=fSegment; i<nSegment; i++) {
    if (Segment[i].used) continue;
    if (Segment[i].ascii!=-1) continue;
    for (c=0; c<CHARS; c++) {
      for (j=fSegment; j<nSegment; j++) {
	if (Segment[j].ascii==c) break;
      }
      if (j==nSegment) break;
    }
    Segment[i].ascii=c;
    switch (Segment[i].type) {
    case BAR_L:
      for (j=0; j<4; j++) {
#if 0
	char Pixel[] = { 0, 1, 3, 7, 15, 31 };
	buffer[j  ]=Pixel[Segment[i].len1];
	buffer[j+4]=Pixel[Segment[i].len2];
#else
	buffer[j  ]=(1<<Segment[i].len1)-1;
	buffer[j+4]=(1<<Segment[i].len2)-1;
#endif
      }
      break;
    case BAR_R:
      for (j=0; j<4; j++) {
#if 0
	char Pixel[] = { 0, 16, 24, 28, 30, 31 };
	buffer[j  ]=Pixel[Segment[i].len1];
	buffer[j+4]=Pixel[Segment[i].len2];
#else
	buffer[j  ]=255<<(XRES-Segment[i].len1);
	buffer[j+4]=255<<(XRES-Segment[i].len2);
#endif
      }
      break;
    case BAR_U:
      for (j=0; j<Segment[i].len1; j++) {
	buffer[7-j]=(1<<XRES)-1;
      }
      for (; j<YRES; j++) {
	buffer[7-j]=0;
      }
      break;
    case BAR_D:
      for (j=0; j<Segment[i].len1; j++) {
	buffer[j]=(1<<XRES)-1;
      }
      for (; j<YRES; j++) {
	buffer[j]=0;
      }
      break;
    }
    defchar(c, buffer);
  }
}


int bar_process (void(*defchar)(int ascii, char *matrix))
{
  int n, s;
  
  create_segments();
  pack_segments();
  define_chars(defchar);
  
  for (s=0; s<nSegment; s++) {
    Segment[s].used=0;
  }

  for (n=0; n<ROWS*COLS; n++) {
    s=Bar[n].segment;
    if (s!=-1) {
      Segment[s].used=1;
    }
  }
  
  return 0;
}


int bar_peek (int row, int col)
{
  int s;

  s=Bar[row*COLS+col].segment;
  if (s==-1) {
    return -1;
  } else {
    return Segment[s].ascii;
  }
}
