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
