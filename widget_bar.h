/* $Id$
 * $URL$
 *
 * bar widget handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 */


#ifndef _WIDGET_BAR_H_
#define _WIDGET_BAR_H_

#include "property.h"

typedef enum { DIR_EAST = 1, DIR_WEST = 2, DIR_NORTH = 4, DIR_SOUTH = 8 } DIRECTION;
typedef enum { STYLE_HOLLOW = 1, STYLE_FIRST = 2, STYLE_LAST = 4 } STYLE;

typedef struct WIDGET_BAR {
    PROPERTY expression1;	/* value (length) of upper half */
    PROPERTY expression2;	/* value (length) of lower half */
    PROPERTY expr_min;		/* explicit minimum value */
    PROPERTY expr_max;		/* explicit maximum value */
    DIRECTION direction;	/* bar direction */
    STYLE style;		/* bar style (hollow) */
    int length;			/* bar length */
    int update;			/* update interval (msec) */
    double val1;		/* bar value, 0.0 ... 1.0 */
    double val2;		/* bar value, 0.0 ... 1.0 */
    double min;			/* minimum value */
    double max;			/* maximum value */
    RGBA color[2];		/* bar colors */
    int color_valid[2];		/* bar color is valid */
} WIDGET_BAR;


extern WIDGET_CLASS Widget_Bar;

#endif
