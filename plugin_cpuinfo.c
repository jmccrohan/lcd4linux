/* $Id: plugin_cpuinfo.c,v 1.1 2004/01/13 10:03:01 reinelt Exp $
 *
 * plugin for /proc/cpuinfo parsing
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
 * $Log: plugin_cpuinfo.c,v $
 * Revision 1.1  2004/01/13 10:03:01  reinelt
 * new util 'hash' for associative arrays
 * new plugin 'cpuinfo'
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_cpuinfo (void)
 *  adds functions to access /proc/cpuinfo
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"


static void my_cpuinfo (RESULT *result, RESULT *arg1)
{
  static HASH CPUinfo = { 0, 0, NULL };
  
  static time_t now=0;
  char *key, *val;
  
  // reread every second only
  if (time(NULL)!=now) {
    FILE *stream;
    
    // destroy previous hash table
    hash_destroy (&CPUinfo);

    stream=fopen("/proc/cpuinfo", "r");
    if (stream==NULL) {
      error ("fopen(/proc/cpuinfo) failed: %s", strerror(errno));
      goto error;
    }
    
    while (!feof(stream)) {
      char buffer[256];
      char *c;
      fgets (buffer, sizeof(buffer), stream);
      c=strchr(buffer, ':');
      if (c==NULL) continue;
      key=buffer; val=c+1;
      // strip leading blanks from key
      while (isspace(*key)) *key++='\0';
      // strip trailing blanks from key
      while (isspace(*--c)) *c='\0';
      // strip leading blanks from value
      while (isspace(*val)) *val++='\0';
      // strip trailing blanks from value
      for (c=val; *c!='\0';c++);
      while (isspace(*--c)) *c='\0';
      
      // add entry to hash table
      hash_set (&CPUinfo, key, val);
      
    }
    
    fclose (stream);
	   
    time(&now);
  }
  
  key=R2S(arg1);
  val=hash_get(&CPUinfo, key);
  if (val==NULL) val="";

 error:
  // val="";
  SetResult(&result, R_STRING, val); 
}


int plugin_init_cpuinfo (void)
{
  AddFunction ("cpuinfo", 1, my_cpuinfo);
  return 0;
}

