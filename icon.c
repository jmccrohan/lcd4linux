/* $Id: icon.c,v 1.11 2003/10/22 04:32:25 reinelt Exp $
 *
 * generic icon and heartbeat handling
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: icon.c,v $
 * Revision 1.11  2003/10/22 04:32:25  reinelt
 * fixed icon bug found by Rob van Nieuwkerk
 *
 * Revision 1.10  2003/10/22 04:19:16  reinelt
 * Makefile.in for imon.c/.h, some MatrixOrbital clients
 *
 * Revision 1.9  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.8  2003/09/19 03:51:29  reinelt
 * minor fixes, widget.c added
 *
 * Revision 1.7  2003/09/11 04:09:53  reinelt
 * minor cleanups
 *
 * Revision 1.6  2003/09/10 14:01:53  reinelt
 * icons nearly finished\!
 *
 * Revision 1.5  2003/09/10 03:48:23  reinelt
 * Icons for M50530, new processing scheme (Ticks.Text...)
 *
 * Revision 1.4  2003/09/09 05:30:34  reinelt
 * even more icons stuff
 *
 * Revision 1.3  2003/09/01 04:09:34  reinelt
 * icons nearly finished, but MatrixOrbital only
 *
 * Revision 1.2  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.1  2003/08/24 04:31:56  reinelt
 * icon.c icon.h added
 *
 *
 */

/* 
 * exported functions:
 *
 * int icon_init (int rows, int cols, int xres, int yres, int chars, int icons, 
 *                void(*defchar)(int ascii, char *bitmap))
 *   initializes all icons stuff and reads the bitmaps from config file.
 *
 * void icon_clear(void)
 *   clears the icon framebuffer
 *
 * int icon_draw (int num, int seq, int row, int col)
 *   puts icon #num sequence #seq at position row, col in the icon framebuffer
 *
 * int icon_peek (int row, int col)
 *   returns icon# or -1 if none from position row, col 
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "icon.h"


typedef struct BITMAP {
  int  nData;
  int  lData;
  char *Data;
} BITMAP;

static int ROWS=0;
static int COLS=0;
static int XRES=0;
static int YRES=0;
static int CHARS;
static int ICONS=0;

static int  *Screen=NULL;
static struct BITMAP *Bitmap=NULL;
static void(*Defchar)(int ascii, char *bitmap);


static int icon_read_bitmap (int num)
{
  struct BITMAP *bm = Bitmap+num;
  int row, n;
  char  key[15];
  char *val, *v;
  char *map;
  
  for (row=0; row<YRES; row++) {
    snprintf (key, sizeof(key), "Icon%d.Bitmap%d", num+1, row+1);
    val=cfg_get(key, ""); 
    map=bm->Data+row;
    n=0;
    for (v=val; *v!='\0'; v++) {
      if (n>=bm->nData) {
	bm->nData++;
	bm->Data=realloc(bm->Data, bm->nData*YRES*sizeof(char));
	memset (bm->Data+n*YRES, 0, YRES*sizeof(char));
	map=bm->Data+n*YRES+row;
      }
      switch (*v) {
      case '|':
	n++;
	map+=YRES;
	break;
      case '*':
	(*map)<<=1;
	(*map)|=1;
	break;
      default:
	(*map)<<=1;
      }
    }
  }
  return 0;
}


int icon_init (int rows, int cols, int xres, int yres, int chars, int icons, 
		void(*defchar)(int ascii, char *bitmap))
{
  int n;
  
  if (rows<1 || cols<1) 
    return -1;
  
  ROWS=rows;
  COLS=cols;
  XRES=xres;
  YRES=yres;
  CHARS=chars,
  ICONS=icons;

  if ((Screen=malloc(ROWS*COLS*sizeof(*Screen)))==NULL) {
    error ("icon buffer allocation failed: out of memory?");
    return -1;
  }

  if ((Bitmap=malloc(icons*sizeof(*Bitmap)))==NULL) {
    error ("icon allocation failed: out of memory?");
    return -1;
  }
  
  Defchar=defchar;
  
  icon_clear();

  for (n=0; n<icons; n++) {
    Bitmap[n].nData=1;
    Bitmap[n].lData=-1;
    Bitmap[n].Data=malloc(YRES*sizeof(char));
    memset (Bitmap[n].Data, 0, YRES*sizeof(char));
    icon_read_bitmap(n);
    // icons use last ascii codes from userdef chars
    if (Defchar) Defchar (CHARS-n-1, Bitmap[n].Data);
  }
  
  return 0;
}


void icon_clear(void)
{
  int n;

  // reset screen buffer
  for (n=0; n<ROWS*COLS; n++) {
    Screen[n]=-1;
  }

  // reset last bitmap pointer
  for (n=0; n<ICONS; n++) {
    Bitmap[n].lData=-1;
  }

}


int icon_draw (int num, int seq, int row, int col)
{
  if (row>=0 && col>=0) {
    // icons use last ascii codes from userdef chars
    Screen[row*COLS+col]=CHARS-num-1;
  }
  
  if (seq>=0) {
    seq%=Bitmap[num].nData;
    if (seq!=Bitmap[num].lData) {
      Bitmap[num].lData=seq;
      if (Defchar) Defchar (CHARS-num-1, Bitmap[num].Data+seq*YRES);
    }
  }
  
  return 0;
}


int icon_peek (int row, int col)
{
  if (Screen) 
    return Screen[row*COLS+col];
  else
    return -1;
}
