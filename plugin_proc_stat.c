/* $Id: plugin_proc_stat.c,v 1.4 2004/01/18 06:54:08 reinelt Exp $
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
 * Revision 1.4  2004/01/18 06:54:08  reinelt
 * bug in expr.c fixed (thanks to Xavier)
 * some progress with /proc/stat parsing
 *
 * Revision 1.3  2004/01/16 11:12:26  reinelt
 * some bugs in plugin_xmms fixed, parsing moved to own function
 * plugin_proc_stat nearly finished
 *
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


static void hash_set1 (char *key1, char *val) 
{
  double number;
  
  number=atof(val);
  
  hash_set (&Stat, key1, val);
}

static void hash_set2 (char *key1, char *key2, char *val) 
{
  char key[32];
  
  snprintf (key, sizeof(key), "%s.%s", key1, key2);
  hash_set1 (key, val);
}

static void hash_set3 (char *key1, char *key2, char *key3, char *val) 
{
  char key[32];
  
  snprintf (key, sizeof(key), "%s.%s.%s", key1, key2, key3);
  hash_set1 (key, val);
}


static int parse_proc_stat (void)
{
  FILE *stream;
  
  // update every 10 msec
  if (!renew(10)) return 0;
  
  // stream=fopen("/proc/stat", "r");
  stream=fopen("proc_stat", "r");
  if (stream==NULL) {
    error ("fopen(/proc/stat) failed: %s", strerror(errno));
    return -1;
  }
  
  while (!feof(stream)) {
    char buffer[1024];
    if (fgets (buffer, sizeof(buffer), stream) ==NULL) break;
    if (strncmp(buffer, "cpu", 3)==0) {
      char *cpu;
      cpu=strtok(buffer, " \t\n");
      hash_set2 (cpu, "user",   strtok(NULL, " \t\n"));
      hash_set2 (cpu, "nice",   strtok(NULL, " \t\n"));
      hash_set2 (cpu, "system", strtok(NULL, " \t\n"));
      hash_set2 (cpu, "idle",   strtok(NULL, " \t\n"));
    } 
    else if (strncmp(buffer, "page ", 5)==0) {
      strtok(buffer, " \t\n");
      hash_set2 ("page", "in",  strtok(NULL, " \t\n"));
      hash_set2 ("page", "out", strtok(NULL, " \t\n"));
    } 
    else if (strncmp(buffer, "swap ", 5)==0) {
      strtok(buffer, " \t\n");
      hash_set2 ("swap", "in",  strtok(NULL, " \t\n"));
      hash_set2 ("swap", "out", strtok(NULL, " \t\n"));
    } 
    else if (strncmp(buffer, "intr ", 5)==0) {
      int i;
      strtok(buffer, " \t\n");
      hash_set2 ("intr", "sum", strtok(NULL, " \t\n"));
      for (i=0; i<16; i++) {
	char buffer[3];
	snprintf(buffer, sizeof(buffer), "%d", i);
	hash_set2 ("intr", buffer, strtok(NULL, " \t\n"));
      }
    } 
    else if (strncmp(buffer, "disk_io:", 8)==0) {
      char *dev=strtok(buffer+8, " \t\n:()");
      while (dev!=NULL) {
	char *p;
	while ((p=strchr(dev, ','))!=NULL) *p=':';
	hash_set3 ("disk_io", dev, "io",   strtok(NULL, " :(,"));
	hash_set3 ("disk_io", dev, "rio",  strtok(NULL, " ,"));
	hash_set3 ("disk_io", dev, "rblk", strtok(NULL, " ,"));
	hash_set3 ("disk_io", dev, "wio",  strtok(NULL, " ,"));
	hash_set3 ("disk_io", dev, "wblk", strtok(NULL, " ,)"));
	dev=strtok(NULL, " \t\n:()");
      }
    } 
    else if (strncmp(buffer, "ctxt ", 5)==0) {
      strtok(buffer, " \t\n");
      hash_set2 ("ctxt", NULL, strtok(NULL, " \t\n"));
    } 
    else if (strncmp(buffer, "btime ", 6)==0) {
      strtok(buffer, " \t\n");
      hash_set2 ("btime", NULL, strtok(NULL, " \t\n"));
    } 
    else if (strncmp(buffer, "processes ", 10)==0) {
      strtok(buffer, " \t\n");
      hash_set1 ("processes", strtok(NULL, " \t\n"));
    } 
    else {
      error ("internal error: unhandled entry '%s' from /proc/stat", strtok(buffer, " \t\n"));
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

