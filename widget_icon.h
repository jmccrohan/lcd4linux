/* $Id: widget_icon.h,v 1.7 2004/06/26 12:05:00 reinelt Exp $
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
 * Revision 1.7  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.6  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.5  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
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
  char *speed_expr;      /* expression for update interval */
  void *speed_tree;      /* pre-compiled expression for update interval */
  int   speed;           /* update interval (msec) */
  char *visible_expr;    /* expression for visibility */
  void *visible_tree;    /* pre-compiled expression for visibility */
  int   visible;         /* icon visible? */
  int   ascii;           /* ascii code of icon (depends on the driver) */
  int   curmap;          /* current bitmap sequence */
  int   prvmap;          /* previous bitmap sequence  */
  int   maxmap;          /* number of bitmap sequences */
  unsigned char *bitmap; /* bitmaps of (animated) icon */
} WIDGET_ICON;

extern WIDGET_CLASS Widget_Icon;

#endif
