/* $Id: plugin_meminfo.c,v 1.8 2004/06/17 06:23:43 reinelt Exp $
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
 * Revision 1.8  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.7  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.6  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.5  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.4  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"

#include "hash.h"


static HASH MemInfo;
static FILE *stream = NULL;

static int parse_meminfo (void)
{
  int age;
  
  // reread every 10 msec only
  age=hash_age(&MemInfo, NULL);
  if (age>0 && age<=10) return 0;
  
  if (stream==NULL) stream=fopen("/proc/meminfo", "r");
  if (stream==NULL) {
    error ("fopen(/proc/meminfo) failed: %s", strerror(errno));
    return -1;
  }
  
  rewind(stream);
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
      hash_put (&MemInfo, key, val);
    }
  }
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
  val=hash_get(&MemInfo, key, NULL);
  if (val==NULL) val="";
  
  SetResult(&result, R_STRING, val); 
}


int plugin_init_meminfo (void)
{
  hash_create(&MemInfo);
  AddFunction ("meminfo", 1, my_meminfo);
  return 0;
}


void plugin_exit_meminfo(void) 
{
  if (stream != NULL) {
    fclose(stream);
    stream=NULL;
  }
  hash_destroy(&MemInfo);
}
