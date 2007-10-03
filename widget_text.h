/* $Id$
 * $URL$
 *
 * simple text widget handling
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


#ifndef _WIDGET_TEXT_H_
#define _WIDGET_TEXT_H_

#include "property.h"

typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT, ALIGN_MARQUEE, ALIGN_AUTOMATIC } TEXT_ALIGN;

typedef struct WIDGET_TEXT {
    PROPERTY prefix;		/* label on the left side */
    PROPERTY postfix;		/* label on the right side */
    PROPERTY value;		/* value of text widget */
    PROPERTY style;		/* text style (plain/bold/slant) */
    char *string;		/* formatted value */
    char *buffer;		/* string with 'width+1' bytes allocated  */
    int width;			/* field width */
    int precision;		/* number of digits after the decimal point */
    TEXT_ALIGN align;		/* alignment: L(eft), C(enter), R(ight), M(arquee), A(utomatic) */
    int update;			/* update interval */
    int scroll;			/* marquee starting point */
    int speed;			/* marquee scrolling speed */
} WIDGET_TEXT;


extern WIDGET_CLASS Widget_Text;

#endif
