/* $Id: qprintf.h,v 1.1 2004/02/27 07:06:26 reinelt Exp $
 *
 * simple but quick snprintf() replacement
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * derived from a patch from Martin Hejl which is
 * Copyright 2003 Martin Hejl (martin@hejl.de)
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
 * $Log: qprintf.h,v $
 * Revision 1.1  2004/02/27 07:06:26  reinelt
 * new function 'qprintf()' (simple but quick snprintf() replacement)
 *
 */

#ifndef _QPRINTF_H_
#define _QPRINTF_H_

int qprintf(char *str, size_t size, const char *format, ...);

#endif
