/* $Id$
 * $URL$
 *
 * config file stuff
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
 *
 * $Log: cfg.h,v $
 * Revision 1.13  2005/05/08 04:32:43  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.12  2005/01/18 06:30:21  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.11  2004/06/20 10:09:53  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.10  2004/01/30 20:57:55  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.9  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.8  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.7  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.6  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.5  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.4  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.3  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.2  2000/04/03 04:46:38  reinelt
 *
 * added '-c key=val' option
 *
 * Revision 1.1  2000/03/10 11:40:47  reinelt
 * *** empty log message ***
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 *
 */

#ifndef _CFG_H_
#define _CFG_H_

int cfg_init(const char *file);
char *cfg_source(void);
int cfg_cmd(const char *arg);
char *cfg_list(const char *section);
char *cfg_get_raw(const char *section, const char *key, const char *defval);
char *cfg_get(const char *section, const char *key, const char *defval);
int cfg_number(const char *section, const char *key, const int defval, const int min, const int max, int *value);
int cfg_exit(void);

#endif
