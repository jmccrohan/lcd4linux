/* $Id: widget.h,v 1.5 2004/01/11 18:26:02 reinelt Exp $
 *
 * generic widget handling
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
 * $Log: widget.h,v $
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


struct WIDGET; // forward declaration


typedef struct WIDGET_CLASS {
  char *name;
  int (*init)   (struct WIDGET *Self);
  int (*update) (struct WIDGET *Self); // Fixme: do we really need this?
  int (*draw)   (struct WIDGET *Self);
  int (*quit)   (struct WIDGET *Self);
} WIDGET_CLASS;


typedef struct WIDGET{
  char         *name;
  WIDGET_CLASS *class;
  void         *data;
} WIDGET;



int widget_register (WIDGET_CLASS *widget);


// some basic widgets
WIDGET_CLASS Widget_Text;

#endif
