/* $Id: icon.c,v 1.2 2003/08/24 05:17:58 reinelt Exp $
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

#include "debug.h"
#include "cfg.h"
#include "icon.h"


static int ROWS=0;
static int COLS=0;
static int XRES=0;
static int YRES=0;
static int ICONS=0;

static int *Screen;


#if 0
static int icon_get_bitmap (int num)
{
  int row, col;
  char  key[15];
  char *val;
  
  for (row=1; row<=8; row++) {
    snprintf (key, sizeof(key), "Icons%d.Bitmap%d", num, row);
    val=cfg_get(key);
  }
}
#endif


int icon_init (int rows, int cols, int xres, int yres, int icons)
{
  if (rows<1 || cols<1) 
    return -1;
  
  ROWS=rows;
  COLS=cols;
  XRES=xres;
  YRES=yres;
  ICONS=icons;

  if (Screen) {
    free (Screen);
  }
  
  if ((Screen=malloc(ROWS*COLS*sizeof(*Screen)))==NULL) {
    return -1;
  }

  icon_clear();

  return 0;
}


void icon_clear(void)
{
  int n;

  for (n=0; n<ROWS*COLS; n++) {
    Screen[n]=-1;
  }
  
}


int icon_peek (int row, int col)
{
  return Screen[row*COLS+col];
}
