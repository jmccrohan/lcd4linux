/* $Id: widget.h,v 1.1 2003/09/19 03:51:29 reinelt Exp $
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
 * $Log: widget.h,v $
 * Revision 1.1  2003/09/19 03:51:29  reinelt
 * minor fixes, widget.c added
 *
 */


#ifndef _WIDGET_H_
#define _WIDGET_H_

typedef struct widget {
  int x, y;
  int w, h;
  
} WIDGET;

int widget_init (int rows, int cols, int xres, int yres);
void widget_clear(void);

#endif
