/* $Id$
 * $URL$
 *
 * debug messages
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

extern int running_foreground;
extern int running_background;
extern int verbose_level;

void message(const int level, const char *format, ...) __attribute__ ((format(__printf__, 2, 3)));;

#define debug(args...) message (2, __FILE__ ": " args)
#define info(args...)  message (1, args)
#define error(args...) message (0, args)

#endif
