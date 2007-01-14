/* $Id$
 * $URL$
 *
 * generic widget handling
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
 * $Log: widget.h,v $
 * Revision 1.22  2006/08/14 05:54:04  reinelt
 * minor warnings fixed, CFLAGS changed (no-strict-aliasing)
 *
 * Revision 1.21  2006/08/09 17:25:34  harbaum
 * Better bar color support and new bold font
 *
 * Revision 1.20  2006/08/08 20:16:29  harbaum
 * Added "extracolor" (used for e.g. bar border) and RGB support for LEDMATRIX
 *
 * Revision 1.19  2006/02/21 15:55:59  cmay
 * removed new update function for keypad, consolidated it with draw
 *
 * Revision 1.18  2006/02/21 05:50:34  reinelt
 * keypad support from Cris Maj
 *
 * Revision 1.17  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 * Revision 1.16  2006/01/23 06:17:18  reinelt
 * timer widget added
 *
 * Revision 1.15  2005/12/18 16:18:36  reinelt
 * GPO's added again
 *
 * Revision 1.14  2005/11/06 09:17:20  reinelt
 * re-use icons (thanks to Jesus de Santos Garcia)
 *
 * Revision 1.13  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.12  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.11  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.10  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.9  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.8  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.7  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.6  2004/01/13 08:18:20  reinelt
 * timer queues added
 * liblcd4linux deactivated turing transformation to new layout
 *
 * Revision 1.5  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.4  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.3  2004/01/10 17:34:40  reinelt
 * further matrixOrbital changes
 * widgets initialized
 *
 * Revision 1.2  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.1  2003/09/19 03:51:29  reinelt
 * minor fixes, widget.c added
 *
 */


#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "rgb.h"


struct WIDGET;			/* forward declaration */


typedef struct WIDGET_CLASS {
    char *name;
    int type;
    int (*init) (struct WIDGET * Self);
    int (*draw) (struct WIDGET * Self);
    int (*find) (struct WIDGET * Self, void *needle);
    int (*quit) (struct WIDGET * Self);
} WIDGET_CLASS;


typedef struct WIDGET {
    char *name;
    WIDGET_CLASS *class;
    struct WIDGET *parent;
    RGBA fg_color;
    RGBA bg_color;
    int fg_valid;
    int bg_valid;
    int layer;
    int row;
    int col;
    void *data;
} WIDGET;


#define WIDGET_TYPE_RC 1
#define WIDGET_TYPE_XY 2
#define WIDGET_TYPE_GPO 3
#define WIDGET_TYPE_TIMER 4
#define WIDGET_TYPE_KEYPAD 5


int widget_register(WIDGET_CLASS * widget);
void widget_unregister(void);
int widget_add(const char *name, const int type, const int layer, const int row, const int col);
WIDGET *widget_find(int type, void *needle);
int widget_color(const char *section, const char *name, const char *key, RGBA * C);

#endif
