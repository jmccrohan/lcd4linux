/* $Id: widget_icon.h,v 1.4 2004/02/15 21:43:43 reinelt Exp $
 *
 * icon widget handling
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
 * $Log: widget_icon.h,v $
 * Revision 1.4  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.3  2004/02/04 19:11:44  reinelt
 * icon visibility patch from Xavier
 *
 * Revision 1.2  2004/01/23 07:04:39  reinelt
 * icons finished!
 *
 * Revision 1.1  2004/01/23 04:54:04  reinelt
 * icon widget added (not finished yet!)
 *
 */


#ifndef _WIDGET_ICON_H_
#define _WIDGET_ICON_H_

typedef struct WIDGET_ICON {
  char *speed_expr;      // expression for update interval
  int   speed;           // update interval (msec)
  int   ascii;           // ascii code of icon (depends on the driver)
  int   curmap;          // current bitmap sequence
  int   prvmap;          // previous bitmap sequence 
  int   maxmap;          // number of bitmap sequences
  unsigned char *bitmap; // bitmaps of (animated) icon
  int   visible;         // icon visible?
  char *visible_expr;    // expression for visibility
} WIDGET_ICON;

extern WIDGET_CLASS Widget_Icon;

#endif
