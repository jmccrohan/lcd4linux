/* $Id: hash.h,v 1.10 2004/03/03 04:44:16 reinelt Exp $
 *
 * hashes (associative arrays)
 *
 * Copyright 1999-2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: hash.h,v $
 * Revision 1.10  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.9  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.8  2004/02/27 06:07:55  reinelt
 * hash improvements from Martin
 *
 * Revision 1.7  2004/01/21 14:29:03  reinelt
 * new helper 'hash_get_regex' which delivers the sum over regex matched items
 * new function 'disk()' which uses this regex matching
 *
 * Revision 1.6  2004/01/21 10:48:17  reinelt
 * hash_age function added
 *
 * Revision 1.5  2004/01/18 09:01:45  reinelt
 * /proc/stat parsing finished
 *
 * Revision 1.4  2004/01/16 11:12:26  reinelt
 * some bugs in plugin_xmms fixed, parsing moved to own function
 * plugin_proc_stat nearly finished
 *
 * Revision 1.3  2004/01/16 07:26:25  reinelt
 * moved various /proc parsing to own functions
 * made some progress with /proc/stat parsing
 *
 * Revision 1.2  2004/01/16 05:04:53  reinelt
 * started plugin proc_stat which should parse /proc/stat
 * which again is a paint in the a**
 * thinking over implementation methods of delta functions
 * (CPU load, ...)
 *
 * Revision 1.1  2004/01/13 10:03:01  reinelt
 * new util 'hash' for associative arrays
 * new plugin 'cpuinfo'
 *
 */

#ifndef _HASH_H_
#define _HASH_H_


// struct timeval
#include <sys/time.h>

typedef struct timeval timeval;

typedef struct {
  timeval time;
  double  val;
} HASH_SLOT;


typedef struct {
  char      *key;
  char      *val;
  timeval   time;
  int       root;
  HASH_SLOT *Slot;
} HASH_ITEM;


typedef struct {
  int       sorted;
  timeval   time;
  int       nItems;
  HASH_ITEM *Items;
} HASH;


void   hash_set       (HASH *Hash, char *key, char *val);
void   hash_set_delta (HASH *Hash, char *key, char *val);
int    hash_age       (HASH *Hash, char *key, char **value);
char  *hash_get       (HASH *Hash, char *key);
double hash_get_delta (HASH *Hash, char *key, int delay);
double hash_get_regex (HASH *Hash, char *key, int delay);
void   hash_destroy   (HASH *Hash);
#endif
