/* $Id: icon.c,v 1.3 2003/09/01 04:09:34 reinelt Exp $
 *
 * generic icon and heartbeat handling
 *
 * written 2003 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: icon.c,v $
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "icon.h"


static int ROWS=0;
static int COLS=0;
static int XRES=0;
static int YRES=0;
static int CHARS;
static int ICONS=0;

static int *Screen=NULL;
static char *Bitmap=NULL;

static int icon_read_bitmap (int num, char *bitmap)
{
  int row, col, len;
  char  key[15];
  char *val;
  char map;
  
  for (row=0; row<YRES; row++) {
    snprintf (key, sizeof(key), "Icon%d.Bitmap%d", num+1, row+1);
    val=cfg_get(key, "");
    len=strlen(val);
    map=0;
    debug ("read_bitmap: num=%d row=%d val=<%s> len=%d", num, row, val, len);
    for (col=0; col<XRES; col++) {
      map<<=1;
      if (col<len && val[col]=='*') {
	map|=1;
      }
    }
    *(bitmap+row-1)=map;
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

  if (Screen) {
    free (Screen);
  }
  
  if ((Screen=malloc(ROWS*COLS*sizeof(*Screen)))==NULL) {
    error ("icon buffer allocation failed: out of memory");
    return -1;
  }

  icon_clear();

  if (Bitmap) {
    free (Bitmap);
  }

  if ((Bitmap=malloc(YRES*icons*sizeof(*Bitmap)))==NULL) {
    error ("icon bitmap allocation failed: out of memory");
    return -1;
  }

  memset (Bitmap, 0, YRES*icons*sizeof(*Bitmap));
  
  for (n=0; n<icons; n++) {
    icon_read_bitmap(n, Bitmap+YRES*n);
    // icons use last ascii codes from userdef chars
    defchar (CHARS-n-1, Bitmap+YRES*n);
  }

  return 0;
}


void icon_clear(void)
{
  int n;

  for (n=0; n<ROWS*COLS; n++) {
    Screen[n]=-1;
  }
  
}


int icon_draw (int num, int row, int col)
{
  // icons use last ascii codes from userdef chars
  Screen[row*COLS+col]=CHARS-num-1;
  return 0;
}


int icon_peek (int row, int col)
{
  return Screen[row*COLS+col];
}
