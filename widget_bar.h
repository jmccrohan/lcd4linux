/* $Id: widget_bar.h,v 1.5 2004/06/26 12:05:00 reinelt Exp $
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
 * Revision 1.5  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.4  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.3  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
 * Revision 1.2  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 * Revision 1.1  2004/01/18 21:25:16  reinelt
 * Framework for bar widget opened
 *
 */


#ifndef _WIDGET_BAR_H_
#define _WIDGET_BAR_H_

typedef enum { DIR_EAST=1, DIR_WEST=2, DIR_NORTH=4, DIR_SOUTH=8 } DIRECTION;

typedef struct WIDGET_BAR {
  char      *expression1;  /* expression that delivers the value */
  char      *expression2;  /* expression that delivers the value */
  char      *expr_min;     /* expression that delivers the minimum value */
  char      *expr_max;     /* expression that delivers the maximum value */
  void      *tree1;        /* pre-compiled expression that delivers the value */
  void      *tree2;        /* pre-compiled expression that delivers the value */
  void      *tree_min;     /* pre-compiled expression that delivers the minimum value */
  void      *tree_max;     /* pre-compiled expression that delivers the maximum value */
  DIRECTION  direction;    /* bar direction */
  int        length;       /* bar length */
  int        update;       /* update interval (msec) */
  double     val1;         /* bar value, 0.0 ... 1.0 */
  double     val2;         /* bar value, 0.0 ... 1.0 */
  double     min;          /* minimum value */
  double     max;          /* maximum value */
} WIDGET_BAR;


extern WIDGET_CLASS Widget_Bar;

#endif
