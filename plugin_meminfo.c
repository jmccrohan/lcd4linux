/* $Id: plugin_meminfo.c,v 1.3 2004/01/21 10:48:17 reinelt Exp $
 *
 * plugin for /proc/meminfo parsing
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
 * $Log: plugin_meminfo.c,v $
 * Revision 1.3  2004/01/21 10:48:17  reinelt
 * hash_age function added
 *
 * Revision 1.2  2004/01/16 07:26:25  reinelt
 * moved various /proc parsing to own functions
 * made some progress with /proc/stat parsing
 *
 * Revision 1.1  2004/01/15 04:29:45  reinelt
 * moved lcd4linux.conf.sample to *.old
 * lcd4linux.conf.sample with new layout
 * new plugins 'loadavg' and 'meminfo'
 * text widget have pre- and postfix
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_meminfo (void)
 *  adds functions to access /proc/meminfo
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"

#include "hash.h"


static HASH MemInfo = { 0, };


static int parse_meminfo (void)
{
  int age;
  int line;
  FILE *stream;
  
  // reread every 100 msec only
  age=hash_age(&MemInfo, NULL, NULL);
  if (age>0 && age<=100) return 0;
  
  stream=fopen("/proc/meminfo", "r");
  if (stream==NULL) {
    error ("fopen(/proc/meminfo) failed: %s", strerror(errno));
    return -1;
  }
    
  line=0;
  while (!feof(stream)) {
    char buffer[256];
    char *c, *key, *val;
    fgets (buffer, sizeof(buffer), stream);
    c=strchr(buffer, ':');
    if (c==NULL) continue;
    key=buffer; val=c+1;
    // strip leading blanks from key
    while (isspace(*key)) *key++='\0';
    // strip trailing blanks from key
    do *c='\0'; while (isspace(*--c));
    // strip leading blanks from value
    while (isspace(*val)) *val++='\0';
    // strip trailing blanks from value
    for (c=val; *c!='\0';c++);
    while (isspace(*--c)) *c='\0';
    // skip lines that do not end with " kB"
    if (*c=='B' && *(c-1)=='k' && *(c-2)==' ') {
      // strip trailing " kB" from value
      *(c-2)='\0';
      // add entry to hash table
      hash_set (&MemInfo, key, val);
    }
  }
  fclose (stream);
  return 0;
}  

static void my_meminfo (RESULT *result, RESULT *arg1)
{
  char *key, *val;
  
  if (parse_meminfo()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  key=R2S(arg1);
  val=hash_get(&MemInfo, key);
  if (val==NULL) val="";
  
  SetResult(&result, R_STRING, val); 
}


int plugin_init_meminfo (void)
{
  AddFunction ("meminfo", 1, my_meminfo);
  return 0;
}

