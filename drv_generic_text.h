/* $Id: drv_generic_text.h,v 1.16 2004/11/28 15:50:24 reinelt Exp $
 *
 * generic driver helper for text-based displays
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_generic_text.h,v $
 * Revision 1.16  2004/11/28 15:50:24  reinelt
 * Cwlinux fixes (invalidation of user-defined chars)
 *
 * Revision 1.15  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.14  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.13  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.12  2004/06/05 06:41:40  reinelt
 *
 * chancged splash screen again
 *
 * Revision 1.11  2004/06/02 10:09:22  reinelt
 *
 * splash screen for HD44780
 *
 * Revision 1.10  2004/06/01 06:45:30  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.9  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.8  2004/03/19 09:17:46  reinelt
 *
 * removed the extra 'goto' function, row and col are additional parameters
 * of the write() function now.
 *
 * Revision 1.7  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.6  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.5  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.4  2004/01/23 07:04:24  reinelt
 * icons finished!
 *
 * Revision 1.3  2004/01/23 04:53:55  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.2  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.1  2004/01/20 05:36:59  reinelt
 * moved text-display-specific stuff to drv_generic_text
 * moved all the bar stuff from drv_generic_bar to generic_text
 *
 */


#ifndef _DRV_GENERIC_TEXT_H_
#define _DRV_GENERIC_TEXT_H_


#include <termios.h>
#include "widget.h"


extern int DROWS, DCOLS; /* display size */
extern int LROWS, LCOLS; /* layout size */
extern int XRES,  YRES;  /* pixel width/height of one char  */
extern int CHARS, CHAR0; /* number of user-defineable characters, ASCII of first char */
extern int ICONS;        /* number of user-defineable characters reserved for icons */
extern int GOTO_COST;    /* number of bytes a goto command requires */
extern int INVALIDATE;   /* re-send a modified userdefined char? */

/* these functions must be implemented by the real driver */
void (*drv_generic_text_real_write)(const int row, const int col, const char *data, const int len);
void (*drv_generic_text_real_defchar)(const int ascii, const unsigned char *matrix);

/* generic functions and widget callbacks */
int  drv_generic_text_init            (const char *section, const char *driver);
int  drv_generic_text_greet           (const char *msg1, const char *msg2);
int  drv_generic_text_draw            (WIDGET *W);
int  drv_generic_text_icon_init       (void);
int  drv_generic_text_icon_draw       (WIDGET *W);
int  drv_generic_text_bar_init        (const int single_segments);
void drv_generic_text_bar_add_segment (const int val1, const int val2, const DIRECTION dir, const int ascii);
int  drv_generic_text_bar_draw        (WIDGET *W);
int  drv_generic_text_quit            (void);


#endif
