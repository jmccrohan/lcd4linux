/* $Id: hash.h,v 1.17 2004/06/26 12:04:59 reinelt Exp $
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
 * Revision 1.17  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.16  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.15  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.14  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.13  2004/06/13 01:12:52  reinelt
 *
 * debug widgets changed (thanks to Andy Baxter)
 *
 * Revision 1.12  2004/06/01 06:45:30  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.11  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
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

/* struct timeval */
#include <sys/time.h>


typedef struct {
  int   size;
  char *value;
  struct timeval timestamp;
} HASH_SLOT;


typedef struct {
  char *key;
  int   val;
} HASH_COLUMN;

typedef struct {
  char      *key;
  int       index;
  int       nSlot;
  HASH_SLOT *Slot;
} HASH_ITEM;


typedef struct {
  int         sorted;
  struct timeval timestamp;
  int         nItems;
  HASH_ITEM   *Items;
  int         nColumns;
  HASH_COLUMN *Columns;
  char        *delimiter;
} HASH;



void   hash_create        (HASH *Hash);

int    hash_age           (HASH *Hash, const char *key);

void   hash_set_column    (HASH *Hash, const int number, const char *column);
void   hash_set_delimiter (HASH *Hash, const char *delimiter);

char  *hash_get           (HASH *Hash, const char *key, const char *column);
double hash_get_delta     (HASH *Hash, const char *key, const char *column, const int delay);
double hash_get_regex     (HASH *Hash, const char *key, const char *column, const int delay);

void   hash_put           (HASH *Hash, const char *key, const char *value);
void   hash_put_delta     (HASH *Hash, const char *key, const char *value);

void   hash_destroy       (HASH *Hash);


#endif
