/* $Id: plugin_diskstats.c,v 1.1 2004/05/29 00:27:23 reinelt Exp $
 *
 * plugin for /proc/diskstats parsing
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin_diskstats.c,v $
 * Revision 1.1  2004/05/29 00:27:23  reinelt
 *
 * added plugin_diskstats.c
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_diskstats (void)
 *  adds functions to access /proc/stat
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"
#include "qprintf.h"
#include "hash.h"


static HASH DISKSTATS = { 0, };
static FILE *stream = NULL;


static void hash_set2 (char *key1, char *key2, char *val) 
{
  char key[32];
  
  qprintf(key, sizeof(key), "%s.%s", key1, key2);
  hash_set_delta (&DISKSTATS, key, val);
}


static int parse_diskstats (void)
{
  int age;
  
  // reread every 10 msec only
  age = hash_age(&DISKSTATS, NULL, NULL);
  if (age > 0 && age <= 10) return 0;
  
  if (stream == NULL) stream = fopen("/proc/diskstats", "r");
  if (stream == NULL) {
    error ("fopen(/proc/diskstats) failed: %s", strerror(errno));
    return -1;
  }
  
  rewind(stream);
  
  while (!feof(stream)) {
    char buffer[1024];
    char *beg, *end;
    char *major = NULL;
    char *minor = NULL;
    char *name  = NULL; 
    char *key[]  = { "reads",  "read_merges",  "read_sectors",  "read_ticks",
		     "writes", "write_merges", "write_sectors", "write_ticks",
		     "in_flight", "io_ticks", "time_in_queue" 
    };
    
    int i;
    
    if (fgets (buffer, sizeof(buffer), stream) == NULL) break;
    
    beg = buffer;
    i = 0;
    while (beg != NULL) {
      while (*beg == ' ') beg++; 
      if ((end = strchr(beg, ' '))) *end = '\0'; 
      switch (i++) {
      case 0:
	major = beg;
	break;
      case 1:
	minor = beg;
	break;
      case 2:
	name = beg;
	hash_set2 ("major", name, major); 
	hash_set2 ("minor", name, minor); 
	break;
      default:
	hash_set2 (key[i-3], name, beg); 
      }
      beg = end ? end+1 : NULL;
    }
  }
  return 0;
}


static void my_diskstats (RESULT *result, RESULT *arg1, RESULT *arg2, RESULT *arg3)
{
  char *dev, *key, buffer[32];
  int delay;
  double value;
  
  if (parse_diskstats() < 0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  dev   = R2S(arg1);
  key   = R2S(arg2);
  delay = R2N(arg3);
  
  qprintf(buffer, sizeof(buffer), "%s\\.%s", dev, key);
  value  = hash_get_regex(&DISKSTATS, buffer, delay);
  
  SetResult(&result, R_NUMBER, &value); 
}


int plugin_init_diskstats (void)
{
  AddFunction ("diskstats", 3, my_diskstats);
  return 0;
}

void plugin_exit_diskstats(void) 
{
  if (stream != NULL) {
    fclose (stream);
    stream = NULL;
  }
  hash_destroy(&DISKSTATS);
}
