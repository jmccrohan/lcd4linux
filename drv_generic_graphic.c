/* $Id
 *
 * generic driver helper for graphic displays
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
 * $Log: drv_generic_graphic.c,v $
 * Revision 1.3  2004/02/22 17:35:41  reinelt
 * some fixes for generic graphic driver and T6963
 * removed ^M from plugin_imon (Nico, are you editing under Windows?)
 *
 * Revision 1.2  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.1  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
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
#include "drv_generic_graphic.h"
#include "font_6x8.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static char *Section=NULL;
static char *Driver=NULL;

int DROWS, DCOLS; // display size (pixels!)
int LROWS, LCOLS; // layout size  (pixels!)
int XRES,  YRES;  // pixels of one char cell

unsigned char *drv_generic_graphic_FB = NULL;


// ****************************************
// *** generic Framebuffer stuff        ***
// ****************************************

static void drv_generic_graphic_resizeFB (int rows, int cols)
{
  char *newFB;
  int row, col;
  
  // Layout FB is large enough
  if (rows<=LROWS && cols<=LCOLS)
    return;
  
  // get maximum values
  if (rows<LROWS) rows=LROWS;
  if (cols<LCOLS) cols=LCOLS;
  
  // allocate new Layout FB
  newFB=malloc(cols*rows*sizeof(char));
  memset (newFB, 0, rows*cols*sizeof(char));
  
  // transfer contents
  if (drv_generic_graphic_FB!=NULL) {
    for (row=0; row<LROWS; row++) {
      for (col=0; col<LCOLS; col++) {
	newFB[row*cols+col]=drv_generic_graphic_FB[row*LCOLS+col];
      }
    }
    free (drv_generic_graphic_FB);
  }
  drv_generic_graphic_FB = newFB;
  
  LCOLS    = cols;
  LROWS    = rows;
}


int drv_generic_graphic_draw (WIDGET *W)
{
  WIDGET_TEXT *Text=W->data;
  unsigned char *txt;
  int row, col, len;
  int x, y;

  row=YRES*W->row;
  col=XRES*W->col;
  txt=Text->buffer;
  len=strlen(txt);
  
  // maybe grow layout framebuffer
  drv_generic_graphic_resizeFB (row+YRES, col+XRES*len);
  
  // render text into layout FB
  while (*txt!='\0') {
    int c=*txt;
    for (y=0; y<YRES; y++) {
      int mask=1<<XRES;
      for (x=0; x<XRES; x++) {
	mask>>=1;
	drv_generic_graphic_FB[(row+y)*LCOLS+col+x] = Font_6x8[c][y]&mask ? 1:0;
      }
    }
    col+=XRES;
    txt++;
  }
  
  // flush area
  drv_generic_graphic_real_blit (YRES*W->row, XRES*W->col, YRES, XRES*len);
  
  return 0;
}


// ****************************************
// *** generic icon handling            ***
// ****************************************

int drv_generic_graphic_icon_draw (WIDGET *W)
{
  WIDGET_ICON *Icon = W->data;
  unsigned char *bitmap = Icon->bitmap+YRES*Icon->curmap;
  int row, col;
  int x, y;
  
  row = YRES*W->row;
  col = XRES*W->col;
  
  // maybe grow layout framebuffer
  drv_generic_graphic_resizeFB (row+YRES, col+XRES);
  
  // render icon
  for (y=0; y<YRES; y++) {
    int mask=1<<XRES;
    for (x=0; x<XRES; x++) {
      mask>>=1;
      drv_generic_graphic_FB[(row+y)*LCOLS+col+x] = Icon->visible ? 0 : bitmap[y]&mask ? 1 : 0;
    }
  }

  // flush area
  drv_generic_graphic_real_blit (row, col, YRES, XRES);

  return 0;
  
}


// ****************************************
// *** generic bar handling             ***
// ****************************************

int drv_generic_graphic_bar_draw (WIDGET *W)
{
  WIDGET_BAR *Bar = W->data;
  int row, col, len, res, rev, max, val1, val2;
  int x, y;
  DIRECTION dir;
  
  row = YRES*W->row;
  col = XRES*W->col;
  dir = Bar->direction;
  len = Bar->length;
  
  // maybe grow layout framebuffer
  if (dir & (DIR_EAST|DIR_WEST)) {
    drv_generic_graphic_resizeFB (row+YRES, col+XRES*len);
  } else {
    drv_generic_graphic_resizeFB (row+YRES*len, col+XRES);
  }
  
  res  = dir & (DIR_EAST|DIR_WEST)?XRES:YRES;
  max  = len * res;
  val1 = Bar->val1 * (double)(max);
  val2 = Bar->val2 * (double)(max);
  
  if      (val1<1)   val1=1;
  else if (val1>max) val1=max;
  
  if      (val2<1)   val2=1;
  else if (val2>max) val2=max;
  
  rev=0;
  
  switch (dir) {
  case DIR_WEST:
    val1=max-val1;
    val2=max-val2;
    rev=1;
    
  case DIR_EAST:
    for (y=0; y<YRES; y++) {
      int val=y<YRES/2 ? val1 : val2;
      for (x=0; x<max; x++) {
	drv_generic_graphic_FB[(row+y)*LCOLS+col+x] = x<val ? !rev : rev;
      }
    }
    break;
    
  case DIR_SOUTH:
    val1=max-val1;
    val2=max-val2;
    rev=1;
    
  case DIR_NORTH:
    for (y=0; y<max; y++) {
      for (x=0; x<XRES; x++) {
	int val=x<XRES/2 ? val1 : val2;
  	drv_generic_graphic_FB[(row+y)*LCOLS+col+x] = y<val ? !rev : rev;
      }
    }
    break;
  }
  
  // flush area
  if (dir & (DIR_EAST|DIR_WEST)) {
    drv_generic_graphic_real_blit (row, col, YRES, XRES*len);
  } else {
    drv_generic_graphic_real_blit (row, col, YRES*len, XRES);
  }

  return 0;
}


// ****************************************
// *** generic init/quit                ***
// ****************************************

int drv_generic_graphic_init (char *section, char *driver)
{
  char *font;
  
  Section=section;
  Driver=driver;
  
  font=cfg_get(section, "Font", "6x8");
  if (font==NULL || *font=='\0') {
    error ("%s: no '%s.Font' entry from %s", Driver, section, cfg_source());
    return -1;
  }

  XRES = -1;
  YRES = -1;
  if (sscanf(font, "%dx%d", &XRES, &YRES)!=2 || XRES<1 || YRES<1) {
    error ("%s: bad Font '%s' from %s", Driver, font, cfg_source());
    return -1;
  }

  // init layout framebuffer
  LROWS = 0;
  LCOLS = 0;
  drv_generic_graphic_FB=NULL;
  drv_generic_graphic_resizeFB (DROWS, DCOLS);
  
  // sanity check
  if (drv_generic_graphic_FB==NULL) {
    error ("%s: framebuffer could not be allocated: malloc() failed", Driver);
    return -1;
  }
  
  return 0;
}


int drv_generic_graphic_quit (void) 
{
  if (drv_generic_graphic_FB) {
    free(drv_generic_graphic_FB);
    drv_generic_graphic_FB=NULL;
  }
  
  return (0);
}
