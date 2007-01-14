/* $Id$
 * $URL$
 *
 * new layouter framework
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: layout.h,v $
 * Revision 1.5  2006/02/27 07:53:52  reinelt
 * some more graphic issues fixed
 *
 * Revision 1.4  2006/02/07 05:36:13  reinelt
 * Layers added to Layout
 *
 * Revision 1.3  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.2  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.1  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 */

#ifndef _LAYOUT_H_
#define _LAYOUT_H_

/* number of layers */
#define LAYERS 3

int layout_init(const char *section);

#endif
