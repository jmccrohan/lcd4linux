/* $Id: hash.h,v 1.1 2004/01/13 10:03:01 reinelt Exp $
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
 * Revision 1.1  2004/01/13 10:03:01  reinelt
 * new util 'hash' for associative arrays
 * new plugin 'cpuinfo'
 *
 */

#ifndef _HASH_H_
#define _HASH_H_


typedef struct HASH_ITEM {
  char *key;
  char *val;
} HASH_ITEM;

typedef struct HASH {
  int       nItems;
  int       sorted;
  HASH_ITEM *Items;
} HASH;


void  hash_set     (HASH *Hash, char *key, char *val);
void *hash_get     (HASH *Hash, char *key);
void  hash_destroy (HASH *Hash);

#endif
