/* $Id: hash.h,v 1.3 2004/01/16 07:26:25 reinelt Exp $
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

typedef struct {
  char *key;
  char *val;
} HASH_ITEM;


typedef struct {
  int       sorted;
  int       nItems;
  HASH_ITEM *Items;
} HASH;


typedef struct {
  struct timeval time;
  double val;
} FILTER_SLOT;

typedef struct {
  char        *key;
  int         nSlots;
  FILTER_SLOT *Slots;
} FILTER_ITEM;

void  hash_set     (HASH *Hash, char *key, char *val);
char *hash_get     (HASH *Hash, char *key);
void  hash_destroy (HASH *Hash);

#endif
