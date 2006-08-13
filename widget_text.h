/* $Id: widget_text.h,v 1.9 2006/08/13 11:38:20 reinelt Exp $
 *
 * simple text widget handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <reinelt@eunet.at>
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
 *
 * $Log: widget_text.h,v $
 * Revision 1.9  2006/08/13 11:38:20  reinelt
 * text widget uses dynamic properties
 *
 * Revision 1.8  2006/08/13 09:53:10  reinelt
 * dynamic properties added (used by 'style' of text widget)
 *
 * Revision 1.7  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.6  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
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
 * Revision 1.2  2004/01/15 07:47:03  reinelt
 * debian/ postinst and watch added (did CVS forget about them?)
 * evaluator: conditional expressions (a?b:c) added
 * text widget nearly finished
 *
 * Revision 1.1  2004/01/15 04:29:45  reinelt
 * moved lcd4linux.conf.sample to *.old
 * lcd4linux.conf.sample with new layout
 * new plugins 'loadavg' and 'meminfo'
 * text widget have pre- and postfix
 *
 */


#ifndef _WIDGET_TEXT_H_
#define _WIDGET_TEXT_H_


#include "property.h"


typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT, ALIGN_MARQUEE } TEXT_ALIGN;

typedef struct WIDGET_TEXT {
    PROPERTY prefix;		/* label on the left side */
    PROPERTY postfix;		/* label on the right side */
    PROPERTY value;             /* value of text widget */
    PROPERTY style;		/* text style (plain/bold/slant) */
    char *string;               /* formatted value */
    char *buffer;		/* string with 'width+1' bytes allocated  */
    int width;			/* field width */
    int precision;		/* number of digits after the decimal point */
    TEXT_ALIGN align;		/* alignment: L(eft), C(enter), R(ight), M(arquee) */
    int update;			/* update interval */
    int scroll;			/* marquee starting point */
    int speed;			/* marquee scrolling speed */
} WIDGET_TEXT;


extern WIDGET_CLASS Widget_Text;

#endif
