/* $Id: plugin_diskstats.c,v 1.4 2004/06/17 10:58:58 reinelt Exp $
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
 * Revision 1.4  2004/06/17 10:58:58  reinelt
 *
 * changed plugin_netdev to use the new fast hash model
 *
 * Revision 1.3  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.2  2004/05/29 01:07:56  reinelt
 * bug in plugin_diskstats fixed
 *
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
#include "hash.h"


static HASH DISKSTATS;
static FILE *stream = NULL;


static int parse_diskstats (void)
{
  int age;
  
  // reread every 10 msec only
  age = hash_age(&DISKSTATS, NULL);
  if (age > 0 && age <= 10) return 0;
  
  if (stream == NULL) stream = fopen("/proc/diskstats", "r");
  if (stream == NULL) {
    error ("fopen(/proc/diskstats) failed: %s", strerror(errno));
    return -1;
  }
  
  rewind(stream);
  
  while (!feof(stream)) {
    char buffer[1024];
    char dev[64];
    char *beg, *end;
    int num, len;
    
    if (fgets (buffer, sizeof(buffer), stream) == NULL) break;
    
    // fetch device name (3rd column) as key
    num = 0;
    beg = buffer;
    end = beg;
    while (*beg) {
      while (*beg == ' ') beg++;
      end = beg + 1;
      while (*end && *end != ' ') end++;
      if (num++ == 2) break;
      beg = end ? end+1 : NULL;
    }
    len = end ? end - beg : strlen(beg);
    
    if (len >= sizeof(dev)) len = sizeof(dev)-1;
    strncpy (dev, beg, len);
    dev[len] = '\0';
    
    hash_put_delta (&DISKSTATS, dev, buffer); 
    
  }
  return 0;
}


static void my_diskstats (RESULT *result, RESULT *arg1, RESULT *arg2, RESULT *arg3)
{
  char *dev, *key;
  int delay;
  double value;
  
  if (parse_diskstats() < 0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  dev   = R2S(arg1);
  key   = R2S(arg2);
  delay = R2N(arg3);
  
  value  = hash_get_regex(&DISKSTATS, dev, key, delay);
  
  SetResult(&result, R_NUMBER, &value); 
}


int plugin_init_diskstats (void)
{
  int i;
  char *header[] = { "major", "minor", "name", 
		     "reads",  "read_merges",  "read_sectors",  "read_ticks",
		     "writes", "write_merges", "write_sectors", "write_ticks",
		     "in_flight", "io_ticks", "time_in_queue", "" };
  
  hash_create (&DISKSTATS);
  hash_set_delimiter (&DISKSTATS, " \n");
  for (i=0; *header[i] != '\0'; i++) {
    hash_set_column (&DISKSTATS, i, header[i]);
  }
  
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
