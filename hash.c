/* $Id: hash.c,v 1.18 2004/05/31 16:39:06 reinelt Exp $
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
 * Revision 1.18  2004/05/31 16:39:06  reinelt
 *
 * added NULL display driver (for debugging/profiling purposes)
 * added backlight/contrast initialisation for matrixOrbital
 * added Backlight initialisation for Cwlinux
 *
 * Revision 1.17  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.16  2004/03/03 08:40:07  hejl
 * Fixed memory leak in hash_get_regex
 *
 * Revision 1.15  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.14  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.13  2004/02/27 06:07:55  reinelt
 * hash improvements from Martin
 *
 * Revision 1.12  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.11  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.10  2004/01/27 04:48:57  reinelt
 * bug with hash_age() fixed (thanks to Markus Keil for pointing this out)
 *
 * Revision 1.9  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.8  2004/01/21 14:29:03  reinelt
 * new helper 'hash_get_regex' which delivers the sum over regex matched items
 * new function 'disk()' which uses this regex matching
 *
 * Revision 1.7  2004/01/21 10:48:17  reinelt
 * hash_age function added
 *
 * Revision 1.6  2004/01/20 05:36:59  reinelt
 * moved text-display-specific stuff to drv_generic_text
 * moved all the bar stuff from drv_generic_bar to generic_text
 *
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
//#include <sys/types.h>

#include "debug.h"
#include "hash.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

// number of slots for delta processing
#define DELTA_SLOTS 64

// string buffer chunk size
#define CHUNK_SIZE 16


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
  
  // no key was passed
  if (key==NULL) return NULL;
  
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
  int len = strlen(val);
  
  Item = hash_lookup (Hash, key, 0);

  // entry already exists?
  if (Item != NULL) {
    if (len > Item->len) {
      // buffer is either empty or too small
      if (Item->val) free (Item->val);
      // allocate memory in multiples of CHUNK_SIZE
      // note that length does not count the trailing \0
      Item->len = CHUNK_SIZE*(len/CHUNK_SIZE+1)-1;
      Item->val = malloc(Item->len+1);
    }
    strcpy(Item->val, val);
    goto hash_got_string;
  }

  // add entry
  Hash->sorted = 0;
  Hash->nItems++;
  Hash->Items = realloc(Hash->Items, Hash->nItems*sizeof(HASH_ITEM));

  Item = &(Hash->Items[Hash->nItems-1]);
  
  Item->key  = strdup(key);

  Item->len  = CHUNK_SIZE*(len/CHUNK_SIZE+1)-1;
  Item->val  = malloc(Item->len+1);
  strcpy(Item->val, val);
  
  Item->root = 0;
  Item->Slot = NULL;

 hash_got_string:
  // set timestamps
  gettimeofday(&Hash->time, NULL);
  Item->time = Hash->time;

  return Item;
}


// insert a string into the hash table
void hash_set (HASH *Hash, char *key, char *val)
{
  hash_set_string (Hash, key, val);
}


// insert a string into the hash table
// convert it into a number, and store it in the
// delta table, too
void hash_set_delta (HASH *Hash, char *key, char *val)
{
  double number=atof(val);
  HASH_ITEM *Item;
  
  Item=hash_set_string (Hash, key, val);
  
  // allocate delta table
  if (Item->Slot==NULL) {
    Item->root = 0;
    Item->Slot  = malloc(DELTA_SLOTS*sizeof(HASH_SLOT));
    memset(Item->Slot, 0, DELTA_SLOTS*sizeof(HASH_SLOT));
  }
  
  // move the pointer to the next free slot, wrap around if necessary
  if (--Item->root < 0) Item->root = DELTA_SLOTS-1;

  // set first entry
  gettimeofday(&(Item->Slot[Item->root].time), NULL);
  Item->Slot[Item->root].val=number;

}


// get a string from the hash table
char *hash_get (HASH *Hash, char *key)
{
  HASH_ITEM *Item=hash_lookup(Hash, key, 1);
  return Item?Item->val:NULL;
}


// return the age in milliseconds of an entry from the hash table
// or from the hash table itself if key is NULL
// returns -1 if entry does not exist
// if **value is set, return the value, too
int hash_age (HASH *Hash, char *key, char **value)
{
  HASH_ITEM *Item;
  timeval now, *stamp;
  int age;
  
  if (key!=NULL) {
    Item=hash_lookup(Hash, key, 1);
    if (value) *value=Item?Item->val:NULL;
    if (Item==NULL) {
      return -1;
    }
    stamp=&Item->time;
  } else {
    stamp=&Hash->time;
  }
  
  gettimeofday(&now, NULL);
  
  age = (now.tv_sec - stamp->tv_sec)*1000 + (now.tv_usec - stamp->tv_usec)/1000;

  return age;
}


// get a delta value from the delta table
double hash_get_delta (HASH *Hash, char *key, int delay)
{
  HASH_ITEM *Item;
  timeval now, end;
  int i;
  double dv, dt;
  HASH_SLOT *pSlot;  
  
  // lookup item
  Item=hash_lookup(Hash, key, 1);
  if (Item==NULL) return 0.0;
  if (Item->Slot==NULL) return 0.0;
  
  // if delay is zero, return absolute value
  if (delay==0) return Item->Slot[Item->root].val;
    
  // prepare timing values
  now=Item->Slot[Item->root].time;
  end.tv_sec  = now.tv_sec;
  end.tv_usec = now.tv_usec-1000*delay;
  if (end.tv_usec<0) {
    end.tv_sec--;
    end.tv_usec += 1000000;
  }

  // search delta slot
  for (i=1; i<DELTA_SLOTS; i++) {   
    pSlot = &(Item->Slot[(Item->root+i) % DELTA_SLOTS]);
    if (pSlot->time.tv_sec==0) break;
    if (timercmp(&(pSlot->time), &end, <)) break;
    dt = (now.tv_sec - pSlot->time.tv_sec) + (now.tv_usec - pSlot->time.tv_usec)/1000000.0; 
  }
  
  
  // empty slot => use the one before
  if (pSlot->time.tv_sec==0) {
    i--;
    pSlot = &(Item->Slot[(Item->root+i) % DELTA_SLOTS]);
  }
  
  // not enough slots available...
  if (i==0) return 0.0;

  // delta value, delta time
  dv = Item->Slot[Item->root].val - pSlot->val; 
  dt = (now.tv_sec - pSlot->time.tv_sec) + (now.tv_usec - pSlot->time.tv_usec)/1000000.0; 

  if (dt > 0.0 && dv >= 0.0) return dv/dt;
  return 0.0;
}


// get a delta value from the delta table
// key may contain regular expressions, and the sum 
// of all matching entries is returned.
double hash_get_regex (HASH *Hash, char *key, int delay)
{
  double sum=0.0;
  regex_t preg;
  int i, err;
  
  err=regcomp(&preg, key, REG_ICASE|REG_NOSUB);
  if (err!=0) {
    char buffer[32];
    regerror(err, &preg, buffer, sizeof(buffer));
    error ("error in regular expression: %s", buffer);
    regfree(&preg);
    return 0.0;
  }

  // force the table to be sorted by requesting anything
  hash_lookup(Hash, NULL, 1);
  
  for (i=0;i<Hash->nItems; i++) {
    if (regexec(&preg, Hash->Items[i].key, 0, NULL, 0)==0) {
      sum+=hash_get_delta(Hash, Hash->Items[i].key, delay);
    }
  }
  regfree(&preg);
  return sum;
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
