/* $Id: plugin_proc_stat.c,v 1.2 2004/01/16 07:26:25 reinelt Exp $
 *
 * plugin for /proc/stat parsing
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
 * $Log: plugin_proc_stat.c,v $
 * Revision 1.2  2004/01/16 07:26:25  reinelt
 * moved various /proc parsing to own functions
 * made some progress with /proc/stat parsing
 *
 * Revision 1.1  2004/01/16 05:04:53  reinelt
 * started plugin proc_stat which should parse /proc/stat
 * which again is a paint in the a**
 * thinking over implementation methods of delta functions
 * (CPU load, ...)
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_proc_stat (void)
 *  adds functions to access /proc/stat
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"


static HASH Stat = { 0, 0, NULL };

static int renew(int msec)
{
  static struct timeval end = {0, 0};
  struct timeval now;

  // update every 10 msec
  gettimeofday(&now, NULL);
  if (now.tv_sec==end.tv_sec?now.tv_usec<end.tv_usec:now.tv_sec<end.tv_sec) {
    return 0;
  }
  end.tv_sec  = now.tv_sec;
  end.tv_usec = now.tv_usec + 1000*msec;
  while (end.tv_usec > 1000000) {
    end.tv_usec -= 1000000;
    end.tv_sec++;
  }
  return 1;
}


static void my_hash_set (char *key1, char *key2, char *val) 
{
  char key[16];
  
  snprintf (key, sizeof(key), "%s.%s", key1, key2);
  debug ("Michi: hash_set(%s, %s)", key, val);
  hash_set (&Stat, key, val);
}


static int parse_proc_stat (void)
{
  FILE *stream;
  
  // update every 10 msec
  if (!renew(10)) return 0;
  
  stream=fopen("/proc/stat", "r");
  if (stream==NULL) {
    error ("fopen(/proc/stat) failed: %s", strerror(errno));
    return -1;
  }
  
  while (!feof(stream)) {
    char buffer[256];
    fgets (buffer, sizeof(buffer), stream);
    
    if (strncmp(buffer, "cpu", 3)==0) {
      char *cpu;
      cpu=strtok(buffer, " \t");
      my_hash_set (cpu, "user",   strtok(NULL, " \t"));
      my_hash_set (cpu, "nice",   strtok(NULL, " \t"));
      my_hash_set (cpu, "system", strtok(NULL, " \t"));
      my_hash_set (cpu, "idle",   strtok(NULL, " \t"));
    } 
    else if (strncmp(buffer, "page ", 5)==0) {
      
    } 
    else if (strncmp(buffer, "swap ", 5)==0) {
    } 
    else if (strncmp(buffer, "intr ", 5)==0) {
    } 
    else if (strncmp(buffer, "disk_io:", 8)==0) {
    } 
    else if (strncmp(buffer, "ctxt ", 5)==0) {
    } 
    else if (strncmp(buffer, "btime ", 5)==0) {
    } 
    else if (strncmp(buffer, "processes ", 5)==0) {
    } 
    else {
      error ("unknown line <%s> from /proc/stat");
    }
  }
  fclose (stream);
  return 0;
}

static void my_proc_stat (RESULT *result, RESULT *arg1)
{
  char *key, *val;
  
  if (parse_proc_stat()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }

  key=R2S(arg1);
  val=hash_get(&Stat, key);
  if (val==NULL) val="";

  SetResult(&result, R_STRING, val); 
}


int plugin_init_proc_stat (void)
{
  AddFunction ("stat", 1, my_proc_stat);
  return 0;
}

