/* $Id: hash.c,v 1.3 2004/01/16 07:26:25 reinelt Exp $
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
 * $Log: hash.c,v $
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

/* 
 * exported functions:
 *
 * hash_anything
 *   Fixme: document me!
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "hash.h"


// bsearch compare function for hash entries
static int hash_lookup_f (const void *a, const void *b)
{
  char *key=(char*)a;
  HASH_ITEM *item=(HASH_ITEM*)b;

  return strcmp(key, item->key);
}


// qsort compare function for hash tables
static int hash_sort_f (const void *a, const void *b)
{
  HASH_ITEM *ha=(HASH_ITEM*)a;
  HASH_ITEM *hb=(HASH_ITEM*)b;

  return strcasecmp(ha->key, hb->key);
}


// insert a key/val pair into the hash table
// the tbale will be searched linearly if the entry 
// does already exist, for the table may not be sorted.
// the table will be flagged 'unsorted' afterwards
void hash_set (HASH *Hash, char *key, char *val)
{
  int i;

  // entry already exists?
  for (i=0;i<Hash->nItems; i++) {
    if (strcmp(key, Hash->Items[i].key)==0) {
      if (Hash->Items[i].val) free (Hash->Items[i].val);
      Hash->Items[i].val=strdup(val);
      return;
    }
  }
  
  // add entry
  Hash->sorted=0;
  Hash->nItems++;
  Hash->Items=realloc(Hash->Items,Hash->nItems*sizeof(HASH_ITEM));
  Hash->Items[i].key=strdup(key);
  Hash->Items[i].val=strdup(val);
}


char *hash_get (HASH *Hash, char *key)
{
  HASH_ITEM *Item;
  
  if (!Hash->sorted) {
    qsort(Hash->Items, Hash->nItems, sizeof(HASH_ITEM), hash_sort_f);
    Hash->sorted=1;
  }
  
  Item=bsearch(key, Hash->Items, Hash->nItems, sizeof(HASH_ITEM), hash_lookup_f);

  return Item?Item->val:NULL;
}


void hash_destroy (HASH *Hash)
{
  int i;

  if (Hash->Items) {
    
    // free all entries
    for (i=0;i<Hash->nItems; i++) {
      if (Hash->Items[i].key) free (Hash->Items[i].key);
      if (Hash->Items[i].val) free (Hash->Items[i].val);
    }
    
    // free hash table
    free (Hash->Items);
  }
  
  Hash->nItems=0;
  Hash->sorted=0;
  Hash->Items=NULL;
}

