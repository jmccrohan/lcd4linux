/* $Id: hash.c,v 1.5 2004/01/18 09:01:45 reinelt Exp $
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
 * Revision 1.5  2004/01/18 09:01:45  reinelt
 * /proc/stat parsing finished
 *
 * Revision 1.4  2004/01/18 06:54:08  reinelt
 * bug in expr.c fixed (thanks to Xavier)
 * some progress with /proc/stat parsing
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


#define FILTER_SLOTS 64


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


// search an entry in the hash table:
// If the table is flagged "sorted", the entry is looked
// up using the bsearch function. If the table is 
// unsorted, it will be searched in a linear way

static HASH_ITEM* hash_lookup (HASH *Hash, char *key, int sortit)
{
  HASH_ITEM *Item=NULL;
  
  // maybe sort the array
  if (sortit && !Hash->sorted) {
    qsort(Hash->Items, Hash->nItems, sizeof(HASH_ITEM), hash_sort_f);
    Hash->sorted=1;
  }
  
  // lookup using bsearch
  if (Hash->sorted) {
    Item=bsearch(key, Hash->Items, Hash->nItems, sizeof(HASH_ITEM), hash_lookup_f);
  }
  
  // linear search
  if (Item==NULL) {
    int i;
    for (i=0;i<Hash->nItems; i++) {
      if (strcmp(key, Hash->Items[i].key)==0) {
	Item=&(Hash->Items[i]);
	break;
      }
    }
  }
  
  return Item;
  
}


// insert a key/val pair into the hash table
// If the entry does already exist, it will be overwritten,
// and the table stays sorted (if it has been before).
// Otherwise, the entry is appended at the end, and 
// the table will be flagged 'unsorted' afterwards

static HASH_ITEM* hash_set_string (HASH *Hash, char *key, char *val)
{
  HASH_ITEM *Item;
  
  Item=hash_lookup (Hash, key, 0);

  // entry already exists?
  if (Item!=NULL) {
    if (Item->val) free (Item->val);
    Item->val = strdup(val);
    return Item;
  }

  // add entry
  Hash->sorted=0;
  Hash->nItems++;
  Hash->Items=realloc(Hash->Items,Hash->nItems*sizeof(HASH_ITEM));

  Item=&(Hash->Items[Hash->nItems-1]);
  
  Item->key   = strdup(key);
  Item->val   = strdup(val);
  Item->Slot  = NULL;
  
  return Item;
}


// insert a string into the hash table
void hash_set (HASH *Hash, char *key, char *val)
{
  hash_set_string (Hash, key, val);
}


// insert a string into the hash table
// convert it into a number, and store it in the
// filter table, too
void hash_set_filter (HASH *Hash, char *key, char *val)
{
  double number=atof(val);
  HASH_ITEM *Item;
  
  Item=hash_set_string (Hash, key, val);
  
  // allocate filter table
  if (Item->Slot==NULL) {
    Item->Slot  = malloc(FILTER_SLOTS*sizeof(HASH_SLOT));
    memset(Item->Slot, 0, FILTER_SLOTS*sizeof(HASH_SLOT));
  }
  
  // shift filter table
  memmove (Item->Slot+1, Item->Slot, (FILTER_SLOTS-1)*sizeof(HASH_SLOT));

  // set first entry
  gettimeofday(&(Item->Slot[0].time), NULL);
  Item->Slot[0].val=number;
}


// get a string from the hash table
char *hash_get (HASH *Hash, char *key)
{
  HASH_ITEM *Item=hash_lookup(Hash, key, 1);
  return Item?Item->val:NULL;
}

// get a delta value from the filter table
double hash_get_filter (HASH *Hash, char *key, int delay)
{
  HASH_ITEM *Item;
  struct timeval now, end;
  int i;
  double dv, dt;
  
  // lookup item
  Item=hash_lookup(Hash, key, 1);
  if (Item==NULL) return 0.0;
  if (Item->Slot==NULL) return 0.0;
  
  // prepare timing values
  now=Item->Slot[0].time;
  end.tv_sec  = now.tv_sec;
  end.tv_usec = now.tv_usec-1000*delay;
  if (end.tv_usec<0) {
    end.tv_sec--;
    end.tv_usec += 1000000;
  }
  
  // search filter slot
  for (i=1; i<FILTER_SLOTS; i++) {
    if (Item->Slot[i].time.tv_sec==0) break;
    if (timercmp(&Item->Slot[i].time, &end, <)) break;
  }

  // empty slot => use the one before
  if (Item->Slot[i].time.tv_sec==0) i--;
  
  // not enough slots available...
  if (i==0) return 0.0;

  // delta value, delta time
  dv = Item->Slot[0].val - Item->Slot[i].val; 
  dt = (now.tv_sec - Item->Slot[i].time.tv_sec) + (now.tv_usec - Item->Slot[i].time.tv_usec)/1000000.0; 

  if (dt > 0.0 && dv >= 0.0) return dv/dt;
  return 0.0;
}


void hash_destroy (HASH *Hash)
{
  int i;

  if (Hash->Items) {
    
    // free all entries
    for (i=0;i<Hash->nItems; i++) {
      if (Hash->Items[i].key)  free (Hash->Items[i].key);
      if (Hash->Items[i].val)  free (Hash->Items[i].val);
      if (Hash->Items[i].Slot) free (Hash->Items[i].Slot);
    }
    
    // free hash table
    free (Hash->Items);
  }
  
  Hash->nItems=0;
  Hash->sorted=0;
  Hash->Items=NULL;
}

