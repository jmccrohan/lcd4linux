/* $Id: widget_bar.h,v 1.1 2004/01/18 21:25:16 reinelt Exp $
 *
 * bar widget handling
 *
 * Copyright 2003,2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: widget_bar.h,v $
 * Revision 1.1  2004/01/18 21:25:16  reinelt
 * Framework for bar widget opened
 *
 */


#ifndef _WIDGET_BAR_H_
#define _WIDGET_BAR_H_

typedef enum { DIR_EAST, DIR_WEST, DIR_NORTH, DIR_SOUTH } DIRECTION;

typedef struct WIDGET_BAR {
  char      *expression1;  // expression that delivers the value
  char      *expression2;  // expression that delivers the value
  DIRECTION  direction;    // bar direction
  int        length;       // bar length
  int        update;       // update interval (msec)
  
} WIDGET_BAR;


extern WIDGET_CLASS Widget_Bar;

#endif
