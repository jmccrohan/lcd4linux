/* $Id: drv_generic_text.c,v 1.12 2004/03/03 03:47:04 reinelt Exp $
 *
 * generic driver helper for text-based displays
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
 * $Log: drv_generic_text.c,v $
 * Revision 1.12  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.11  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.10  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.9  2004/02/07 13:45:23  reinelt
 * icon visibility patch #2 from Xavier
 *
 * Revision 1.8  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.7  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.6  2004/01/23 07:04:23  reinelt
 * icons finished!
 *
 * Revision 1.5  2004/01/23 04:53:54  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.4  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.3  2004/01/20 14:25:12  reinelt
 * some reorganization
 * moved drv_generic to drv_generic_serial
 * moved port locking stuff to drv_generic_serial
 *
 * Revision 1.2  2004/01/20 12:45:47  reinelt
 * "Default screen" working with MatrixOrbital
 *
 * Revision 1.1  2004/01/20 05:36:59  reinelt
 * moved text-display-specific stuff to drv_generic_text
 * moved all the bar stuff from drv_generic_bar to generic_text
 *
 */

/* 
 *
 * exported fuctions:
 *
 * Fixme: document me!
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

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


typedef struct {
  int val1;
  int val2;
  DIRECTION dir;
  int segment;
} BAR;

typedef struct {
  int val1;
  int val2;
  DIRECTION dir;
  int used;
  int ascii;
} SEGMENT;

static char *Section=NULL;
static char *Driver=NULL;


int DROWS, DCOLS; // display size
int LROWS, LCOLS; // layout size
int XRES,  YRES;  // pixels of one char cell
int GOTO_COST;    // number of bytes a goto command requires
int CHARS, CHAR0; // number of user-defineable characters, ASCII of first char
int ICONS;        // number of user-defineable characters reserved for icons


static unsigned char *LayoutFB    = NULL;
static unsigned char *DisplayFB   = NULL;

static int nSegment=0;
static int fSegment=0;
static SEGMENT Segment[128];
static BAR *BarFB = NULL;



// ****************************************
// *** generic Framebuffer stuff        ***
// ****************************************

static void drv_generic_text_resizeFB (int rows, int cols)
{
  char *newFB;
  BAR *newBar;
  int i, row, col;
  
  // Layout FB is large enough
  if (rows<=LROWS && cols<=LCOLS)
    return;
  
  // get maximum values
  if (rows<LROWS) rows=LROWS;
  if (cols<LCOLS) cols=LCOLS;
  
  // allocate new Layout FB
  newFB=malloc(cols*rows*sizeof(char));
  memset (newFB, ' ', rows*cols*sizeof(char));

  // transfer contents
  if (LayoutFB!=NULL) {
    for (row=0; row<LROWS; row++) {
      for (col=0; col<LCOLS; col++) {
	newFB[row*cols+col]=LayoutFB[row*LCOLS+col];
      }
    }
    free (LayoutFB);
  }
  LayoutFB = newFB;

  
  // resize Bar buffer
  if (BarFB) {

    newBar=malloc (rows*cols*sizeof(BAR));

    for (i=0; i<rows*cols; i++) {
      newBar[i].val1    = -1;
      newBar[i].val2    = -1;
      newBar[i].dir     =  0;
      newBar[i].segment = -1;
    }
    
    // transfer contents
    for (row=0; row<LROWS; row++) {
      for (col=0; col<LCOLS; col++) {
	newBar[row*cols+col]=BarFB[row*LCOLS+col];
      }
    }

    free (BarFB);
    BarFB=newBar;
  }
  
  LCOLS    = cols;
  LROWS    = rows;
}



int drv_generic_text_draw (WIDGET *W)
{
  WIDGET_TEXT *Text=W->data;
  char *txt, *fb1, *fb2;
  int row, col, len, end;
  
  row=W->row;
  col=W->col;
  txt=Text->buffer;
  len=strlen(txt);
  end=col+len;
  
  // maybe grow layout framebuffer
  drv_generic_text_resizeFB (row+1, col+len);

  fb1 = LayoutFB  + row*LCOLS;
  fb2 = DisplayFB + row*DCOLS;
  
  // transfer new text into layout buffer
  memcpy (fb1+col, txt, len);
  
  if (row<DROWS) {
    for (; col<=end && col<DCOLS; col++) {
      int pos1, pos2, equal;
      if (fb1[col]==fb2[col]) continue;
      drv_generic_text_real_goto (row, col);
      for (pos1=col, pos2=pos1, col++, equal=0; col<=end && col<DCOLS; col++) {
	if (fb1[col]==fb2[col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes several bytes, too.
	  if (++equal>GOTO_COST) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      memcpy                      (fb2+pos1, fb1+pos1, pos2-pos1+1);
      drv_generic_text_real_write (fb2+pos1,           pos2-pos1+1);
    }
  }

  return 0;
}


// ****************************************
// *** generic icon handling            ***
// ****************************************

int drv_generic_text_icon_draw (WIDGET *W)
{
  static int icon_counter=0;
  WIDGET_ICON *Icon = W->data;
  int row, col;
  unsigned char ascii;
  
  row = W->row;
  col = W->col;
  
  // maybe grow layout framebuffer
  drv_generic_text_resizeFB (row+1, col+1);
  
  // icon deactivated?
  if (Icon->ascii==-2) return 0;
  
  // ASCII already assigned?
  if (Icon->ascii==-1) {
    if (icon_counter>=ICONS) {
      error ("cannot process icon '%s': out of icons", W->name);
      Icon->ascii=-2;
      return -1;
    }
    icon_counter++;
    Icon->ascii=CHAR0+CHARS-icon_counter;
  }

  // maybe redefine icon
  if (Icon->curmap!=Icon->prvmap) {
    drv_generic_text_real_defchar(Icon->ascii, Icon->bitmap+YRES*Icon->curmap);
  }

  // use blank if invisible
  ascii=Icon->visible?Icon->ascii:' ';

  // transfer icon into layout buffer
  LayoutFB[row*LCOLS+col]=ascii;

  // maybe send icon to the display
  if (DisplayFB[row*DCOLS+col]!=ascii) {
    DisplayFB[row*DCOLS+col]=ascii;
    drv_generic_text_real_goto (row, col);
    drv_generic_text_real_write (DisplayFB+row*DCOLS+col, 1);
  }

  return 0;
  
}


// ****************************************
// *** generic bar handling             ***
// ****************************************

static void drv_generic_text_bar_clear(void)
{
  int i;
  
  for (i=0; i<LROWS*LCOLS; i++) {
    BarFB[i].val1    = -1;
    BarFB[i].val2    = -1;
    BarFB[i].dir     =  0;
    BarFB[i].segment = -1;
  }

  for (i=0; i<nSegment;i++) {
    Segment[i].used = 0;
  }
}


static void drv_generic_text_bar_create_bar (int row, int col, DIRECTION dir, int len, int val1, int val2)
{
  int rev=0;

  switch (dir) {
  case DIR_WEST:
    val1 = len-val1;
    val2 = len-val2;
    rev  = 1;
    
  case DIR_EAST:
    while (len > 0 && col < LCOLS) {
      BarFB[row*LCOLS+col].dir=dir;
      BarFB[row*LCOLS+col].segment=-1;
      if (val1 >= XRES) {
	BarFB[row*LCOLS+col].val1 = rev?0:XRES;
	val1 -= XRES;
      } else {
	BarFB[row*LCOLS+col].val1 = rev?XRES-val1:val1;
	val1 = 0;
      }
      if (val2 >= XRES) {
	BarFB[row*LCOLS+col].val2 = rev?0:XRES;
	val2 -= XRES;
      } else {
	BarFB[row*LCOLS+col].val2 = rev?XRES-val2:val2;
	val2 = 0;
      }
      len--;
      col++;
    }
    break;
    
  case DIR_SOUTH:
    val1 = len-val1;
    val2 = len-val2;
    rev  = 1;
    
  case DIR_NORTH:
    while (len > 0 && row < LROWS) {
      BarFB[row*LCOLS+col].dir=dir;
      BarFB[row*LCOLS+col].segment=-1;
      if (val1 >= YRES) {
	BarFB[row*LCOLS+col].val1 = rev?0:YRES;
	val1 -= YRES;
      } else {
	BarFB[row*LCOLS+col].val1 = rev?YRES-val1:val1;
	val1 = 0;
      }
      if (val2 >= YRES) {
	BarFB[row*LCOLS+col].val2 = rev?0:YRES;
	val2 -= YRES;
      } else {
	BarFB[row*LCOLS+col].val2 = rev?YRES-val2:val2;
	val2 = 0;
      }
      len--;
      row++;
    }
    break;
    
  }
}


static void drv_generic_text_bar_create_segments (void)
{
  int i, j, n;
  int res, l1, l2;
  
  /* find first unused segment */
  for (i=fSegment; i<nSegment && Segment[i].used; i++);
  
  /* pack unused segments */
  for (j=i+1; j<nSegment; j++) {
    if (Segment[j].used)
      Segment[i++]=Segment[j];
  }
  nSegment=i;
  
  /* create needed segments */
  for (n=0; n<LROWS*LCOLS; n++) {
    if (BarFB[n].dir==0) continue;
    res=BarFB[n].dir & (DIR_EAST|DIR_WEST) ? XRES:YRES;
    for (i=0; i<nSegment; i++) {
      if (Segment[i].dir & BarFB[n].dir) {
	l1 = Segment[i].val1; if (l1>res) l1=res;
	l2 = Segment[i].val2; if (l2>res) l2=res;
	if (l1 == BarFB[n].val1 && l2 == BarFB[n].val2) break;
      }
    }
    if (i==nSegment) {
      nSegment++;
      Segment[i].val1=BarFB[n].val1;
      Segment[i].val2=BarFB[n].val2;
      Segment[i].dir=BarFB[n].dir;
      Segment[i].used=0;
      Segment[i].ascii=-1;
    }
    BarFB[n].segment=i;
  }
}


static int drv_generic_text_bar_segment_error (int i, int j)
{
  int res;
  int i1, i2, j1, j2;
  
  if (i==j) return 65535;
  if (!(Segment[i].dir & Segment[j].dir)) return 65535;
  
  res = Segment[i].dir&(DIR_EAST|DIR_WEST) ? XRES:YRES;
  
  i1=Segment[i].val1; if (i1>res) i1=res;
  i2=Segment[i].val2; if (i2>res) i2=res;
  j1=Segment[j].val1; if (j1>res) j1=res;
  j2=Segment[j].val2; if (j2>res) j2=res;
  
  if (i1==0   && j1!=0)  return 65535;
  if (i2==0   && j2!=0)  return 65535;
  if (i1==res && j1<res) return 65535;
  if (i2==res && j2<res) return 65535;
  if (i1==1   && j1!=1 && i2 > 0)  return 65535;
  if (i2==1   && j2!=1 && j1 > 0)  return 65535;
  if (i1==i2  && j1!=j2) return 65535;
  
  return (i1-j1)*(i1-j1)+(i2-j2)*(i2-j2);
}


static void drv_generic_text_bar_pack_segments (void)
{
  int i, j, n, min;
  int pack_i, pack_j;
  int pass1=1;
  int error[nSegment][nSegment];
  
  if (nSegment<=fSegment+CHARS-ICONS) {
    return;
  }
  
  for (i=0; i<nSegment; i++) {
    for (j=0; j<nSegment; j++) {
      error[i][j]=drv_generic_text_bar_segment_error(i,j);
    }
  }
  
  while (nSegment>fSegment+CHARS-ICONS) {
    
    min=65535;
    pack_i=-1;
    pack_j=-1;
    for (i=fSegment; i<nSegment; i++) {
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
	error ("unable to compact bar characters");
	nSegment=CHARS-ICONS;
	break;
      }
    } 

#if 0
    debug ("pack_segment: n=%d i=%d j=%d min=%d", nSegment, pack_i, pack_j, min);
    debug ("Pack_segment: i1=%d i2=%d j1=%d j2=%d\n", 
	   Segment[pack_i].val1, Segment[pack_i].val2, 
	   Segment[pack_j].val1, Segment[pack_j].val2);
#endif
    
    nSegment--;
    Segment[pack_i]=Segment[nSegment];
    
    for (i=0; i<nSegment; i++) {
      error[pack_i][i]=error[nSegment][i];
      error[i][pack_i]=error[i][nSegment];
    }
    
    for (n=0; n<LROWS*LCOLS; n++) {
      if (BarFB[n].segment==pack_i)   BarFB[n].segment=pack_j;
      if (BarFB[n].segment==nSegment) BarFB[n].segment=pack_i;
    }
  }
}


static void drv_generic_text_bar_define_chars(void)
{
  int c, i, j;
  unsigned char buffer[8];
  
  for (i=fSegment; i<nSegment; i++) {
    if (Segment[i].used) continue;
    if (Segment[i].ascii!=-1) continue;
    for (c=0; c<CHARS-ICONS; c++) {
      for (j=fSegment; j<nSegment; j++) {
	if (Segment[j].ascii==c) break;
      }
      if (j==nSegment) break;
    }
    Segment[i].ascii=c;
    switch (Segment[i].dir) {
    case DIR_WEST:
      for (j=0; j<4; j++) {
	buffer[j  ]=(1<<Segment[i].val1)-1;
	buffer[j+4]=(1<<Segment[i].val2)-1;
      }
      break;
    case DIR_EAST:
      for (j=0; j<4; j++) {
	buffer[j  ]=255<<(XRES-Segment[i].val1);
	buffer[j+4]=255<<(XRES-Segment[i].val2);
      }
      break;
    case DIR_NORTH:
      for (j=0; j<Segment[i].val1; j++) {
	buffer[7-j]=(1<<XRES)-1;
      }
      for (; j<YRES; j++) {
	buffer[7-j]=0;
      }
      break;
    case DIR_SOUTH:
      for (j=0; j<Segment[i].val1; j++) {
	buffer[j]=(1<<XRES)-1;
      }
      for (; j<YRES; j++) {
	buffer[j]=0;
      }
      break;
    }
    drv_generic_text_real_defchar(CHAR0+c, buffer);
  }
}


int drv_generic_text_bar_draw (WIDGET *W)
{
  WIDGET_BAR *Bar = W->data;
  int row, col, len, res, max, val1, val2;
  int c, n, s;
  DIRECTION dir;
  
  row = W->row;
  col = W->col;
  dir = Bar->direction;
  len = Bar->length;

  // maybe grow layout framebuffer
  // bars *always* grow heading North or East!
  if (dir & (DIR_EAST|DIR_WEST)) {
    drv_generic_text_resizeFB (row+1, col+len);
  } else {
    drv_generic_text_resizeFB (row+1, col+1);
  }

  res  = dir & (DIR_EAST|DIR_WEST)?XRES:YRES;
  max  = len * res;
  val1 = Bar->val1 * (double)(max);
  val2 = Bar->val2 * (double)(max);
  
  if      (val1<1)   val1=1;
  else if (val1>max) val1=max;
  
  if      (val2<1)   val2=1;
  else if (val2>max) val2=max;
  
  // create this bar
  drv_generic_text_bar_create_bar (row, col, dir, len, val1, val2);

  // process all bars
  drv_generic_text_bar_create_segments ();
  drv_generic_text_bar_pack_segments ();
  drv_generic_text_bar_define_chars();
  
  // reset usage flags
  for (s=0; s<nSegment; s++) {
    Segment[s].used=0;
  }
  
  // set usage flags
  for (n=0; n<LROWS*LCOLS; n++) {
    if ((s=BarFB[n].segment)!=-1) Segment[s].used=1;
  }

  // transfer bars into layout buffer
  for (n=0; n<LCOLS*LROWS; n++) {
    s=BarFB[n].segment;
    if (s==-1) continue;
    c=Segment[s].ascii;
    if (c==-1) continue;
    if (s>=fSegment) c+=CHAR0; // ascii offset for user-defineable chars
    if(c==LayoutFB[n]) continue;
    LayoutFB[n]=c;
  }
  
  // transfer differences to the display
  for (row=0; row<DROWS; row++) {
    for (col=0; col<DCOLS; col++) {
      int pos1, pos2, equal;
      if (LayoutFB[row*LCOLS+col]==DisplayFB[row*DCOLS+col]) continue;
      drv_generic_text_real_goto (row, col);
      for (pos1=col, pos2=pos1, col++, equal=0; col<DCOLS; col++) {
	if (LayoutFB[row*LCOLS+col]==DisplayFB[row*DCOLS+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes several bytes, too.
	  if (++equal>GOTO_COST) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      memcpy                      (DisplayFB+row*DCOLS+pos1, LayoutFB+row*LCOLS+pos1, pos2-pos1+1);
      drv_generic_text_real_write (DisplayFB+row*DCOLS+pos1,                          pos2-pos1+1);
    }
  }
  
  return 0;

}


// ****************************************
// *** generic init/quit                ***
// ****************************************

int drv_generic_text_init (char *section, char *driver)
{

  Section=section;
  Driver=driver;

  // init display framebuffer
  DisplayFB = (char*)malloc(DCOLS*DROWS*sizeof(char));
  memset (DisplayFB, ' ', DROWS*DCOLS*sizeof(char));
  
  // init layout framebuffer
  LROWS = 0;
  LCOLS = 0;
  LayoutFB=NULL;
  drv_generic_text_resizeFB (DROWS, DCOLS);
  
  // sanity check
  if (LayoutFB==NULL || DisplayFB==NULL) {
    error ("%s: framebuffer could not be allocated: malloc() failed", Driver);
    return -1;
  }
  
  return 0;
}


int drv_generic_text_icon_init (void)
{
  if (cfg_number(Section, "Icons", 0, 0, CHARS, &ICONS)<0) return -1;
  if (ICONS>0) {
    info ("%s: reserving %d of %d user-defined characters for icons", Driver, ICONS, CHARS);
  }
  return 0;
}


int drv_generic_text_bar_init (void)
{
  if (BarFB) free (BarFB);
  
  if ((BarFB=malloc (LROWS*LCOLS*sizeof(BAR)))==NULL) {
    error ("bar buffer allocation failed: out of memory");
    return -1;
  }
  
  nSegment=0;
  fSegment=0;
  
  drv_generic_text_bar_clear();
  
  return 0;
}


void drv_generic_text_bar_add_segment(int val1, int val2, DIRECTION dir, int ascii)
{
  Segment[fSegment].val1=val1;
  Segment[fSegment].val2=val2;
  Segment[fSegment].dir=dir;
  Segment[fSegment].used=0;
  Segment[fSegment].ascii=ascii;
  
  fSegment++;
  nSegment=fSegment;
}


int drv_generic_text_quit (void) {
  
  if (LayoutFB) {
    free(LayoutFB);
    LayoutFB=NULL;
  }
  
  if (DisplayFB) {
    free(DisplayFB);
    DisplayFB=NULL;
  }
  
  if (BarFB) {
    free (BarFB);
    BarFB=NULL;
  }
  widget_unregister();

  return (0);
}
