/* $Id: widget.c,v 1.1 2003/09/19 03:51:29 reinelt Exp $
 *
 * generic widget handling
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
 * $Log: widget.c,v $
 * Revision 1.1  2003/09/19 03:51:29  reinelt
 * minor fixes, widget.c added
 *
 */

/* 
 * exported functions:
 *
 * int widget_junk(void)
 *   does something
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "widget.h"

static int ROWS=0;
static int COLS=0;
static int XRES=0;
static int YRES=0;

static int *Screen=NULL;
static WIDGET *Widget=NULL;
static int nWidget=0;


int widget_init (int rows, int cols, int xres, int yres)
{
  if (rows<1 || cols<1) 
    return -1;
  
  ROWS=rows;
  COLS=cols;
  XRES=xres;
  YRES=yres;
  
  if ((Screen=malloc(ROWS*COLS*sizeof(*Screen)))==NULL) {
    error ("widget buffer allocation failed: out of memory?");
    return -1;
  }
 
  nWidget=0;
  Widget=NULL;
return 0;
}


void widget_clear (void)
{
  int n;
  
  for (n=0; n<ROWS*COLS; n++) {
    Screen[n]=-1;
  }
  
}


int widget_add ()
{
  nWidget++;
  Widget=realloc(Widget, nWidget*sizeof(*Widget));
  
  return 0;
}


int widget_peek (int row, int col)
{
  if (Screen) 
    return Screen[row*COLS+col];
  else
    return -1;
}
