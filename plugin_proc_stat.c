/* $Id: plugin_proc_stat.c,v 1.18 2004/03/11 06:39:59 reinelt Exp $
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
 * Revision 1.18  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.17  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.16  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.15  2004/02/04 19:10:51  reinelt
 * Crystalfontz driver nearly finished
 *
 * Revision 1.14  2004/02/01 19:37:40  reinelt
 * got rid of every strtok() incarnation.
 *
 * Revision 1.13  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.12  2004/01/27 08:13:39  reinelt
 * ported PPP token to plugin_ppp
 *
 * Revision 1.11  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
 * Revision 1.10  2004/01/22 08:55:30  reinelt
 * fixed unhandled kernel-2.6 entries in /prco/stat
 *
 * Revision 1.9  2004/01/21 14:29:03  reinelt
 * new helper 'hash_get_regex' which delivers the sum over regex matched items
 * new function 'disk()' which uses this regex matching
 *
 * Revision 1.8  2004/01/21 11:31:23  reinelt
 * two bugs with hash_age() ixed
 *
 * Revision 1.7  2004/01/21 10:48:17  reinelt
 * hash_age function added
 *
 * Revision 1.6  2004/01/20 12:45:47  reinelt
 * "Default screen" working with MatrixOrbital
 *
 * Revision 1.5  2004/01/18 09:01:45  reinelt
 * /proc/stat parsing finished
 *
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


static HASH Stat = { 0, };
static FILE *stream = NULL;


static void hash_set1 (char *key1, char *val) 
{
  hash_set_delta (&Stat, key1, val);
}


static void hash_set2 (char *key1, char *key2, char *val) 
{
  char key[32];
  
  qprintf(key, sizeof(key), "%s.%s", key1, key2);
  hash_set1 (key, val);
}


static void hash_set3 (char *key1, char *key2, char *key3, char *val) 
{
  char key[32];
  
  qprintf(key, sizeof(key), "%s.%s.%s", key1, key2, key3);
  hash_set1 (key, val);
}


static int parse_proc_stat (void)
{
  int age;
  
  // reread every 10 msec only
  age=hash_age(&Stat, NULL, NULL);
  if (age>0 && age<=10) return 0;
  
  if (stream==NULL) stream=fopen("/proc/stat", "r");
  if (stream==NULL) {
    error ("fopen(/proc/stat) failed: %s", strerror(errno));
    return -1;
  }
  
  rewind(stream);
  
  while (!feof(stream)) {
    char buffer[1024];
    if (fgets (buffer, sizeof(buffer), stream) == NULL) break;
    
    if (strncmp(buffer, "cpu", 3)==0) {
      char *key[]  = { "user", "nice", "system", "idle" }; 
      char delim[] = " \t\n";
      char *cpu, *beg, *end; 
      int i;
      
      cpu=buffer;
      
      // skip "cpu" or "cpu0" block
      if ((end=strpbrk(buffer, delim))!=NULL) *end='\0'; 
      beg=end?end+1:NULL;
      
      for (i=0; i<4 && beg!=NULL; i++) {
	while (strchr(delim, *beg)) beg++; 
	if ((end=strpbrk(beg, delim))) *end='\0'; 
	hash_set2 (cpu, key[i], beg); 
	beg=end?end+1:NULL;
      }
    } 
    else if (strncmp(buffer, "page ", 5)==0) {
      char *key[]  = { "in", "out" }; 
      char delim[] = " \t\n";
      char *beg, *end;
      int i;
      
      for (i=0, beg=buffer+5; i<2 && beg!=NULL; i++) {
	while (strchr(delim, *beg)) beg++; 
	if ((end=strpbrk(beg, delim))) *end='\0'; 
	hash_set2 ("page", key[i], beg); 
	beg=end?end+1:NULL;
      }
    } 
    
    else if (strncmp(buffer, "swap ", 5)==0) {
      char *key[]  = { "in", "out" }; 
      char delim[] = " \t\n";
      char *beg, *end;
      int i;
      
      for (i=0, beg=buffer+5; i<2 && beg!=NULL; i++) {
	while (strchr(delim, *beg)) beg++; 
	if ((end=strpbrk(beg, delim))) *end='\0'; 
	hash_set2 ("swap", key[i], beg); 
	beg=end?end+1:NULL;
      }
    } 

    else if (strncmp(buffer, "intr ", 5)==0) {
      char delim[]=" \t\n";
      char *beg, *end, num[4];
      int i;
      
      for (i=0, beg=buffer+5; i<17 && beg!=NULL; i++) {
	while (strchr(delim, *beg)) beg++; 
	if ((end=strpbrk(beg, delim))) *end='\0'; 
	if (i==0) 
	  strcpy(num, "sum");
	else 
	  qprintf(num, sizeof(num), "%d", i-1);
	hash_set2 ("intr", num,  beg);
	beg=end?end+1:NULL;
      }
    } 
    else if (strncmp(buffer, "disk_io:", 8)==0) {
      char *key[]  = { "io", "rio", "rblk", "wio", "wblk" }; 
      char delim[] = " ():,\t\n";
      char *dev, *beg, *end, *p;
      int i;
      
      dev=buffer+8;
      while (dev!=NULL) {
	while (strchr(delim, *dev)) dev++; 
	if ((end=strchr(dev, ')'))) *end='\0'; 
	while ((p=strchr(dev, ','))!=NULL) *p=':';
	beg=end?end+1:NULL;
	for (i=0; i<5 && beg!=NULL; i++) {
	  while (strchr(delim, *beg)) beg++; 
	  if ((end=strpbrk(beg, delim))) *end='\0'; 
	  hash_set3 ("disk_io", dev, key[i], beg); 
	  beg=end?end+1:NULL;
	}
	dev=beg;
      } 
    }
    
    else {
      char delim[] = " \t\n";
      char *beg, *end;

      beg=buffer;
      if ((end=strpbrk(beg, delim))) *end='\0'; 
      beg=end?end+1:NULL;
      if ((end=strpbrk(beg, delim))) *end='\0'; 
      while (strchr(delim, *beg)) beg++; 
      hash_set1 (buffer, beg);
    } 
  }
  return 0;
}


static void my_proc_stat (RESULT *result, int argc, RESULT *argv[])
{
  char  *string;
  double number;
  
  if (parse_proc_stat()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  switch (argc) {
  case 1:
    string=hash_get(&Stat, R2S(argv[0]));
    if (string==NULL) string="";
    SetResult(&result, R_STRING, string); 
    break;
  case 2:
    number=hash_get_delta(&Stat, R2S(argv[0]), R2N(argv[1]));
    SetResult(&result, R_NUMBER, &number); 
    break;
  default:
    error ("proc_stat(): wrong number of parameters");
    SetResult(&result, R_STRING, ""); 
  }
}


static void my_cpu (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  char *key;
  int delay;
  double value;
  double cpu_user, cpu_nice, cpu_system, cpu_idle, cpu_total;
  
  if (parse_proc_stat()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  key   = R2S(arg1);
  delay = R2N(arg2);
  
  cpu_user   = hash_get_delta(&Stat, "cpu.user",   delay);
  cpu_nice   = hash_get_delta(&Stat, "cpu.nice",   delay);
  cpu_system = hash_get_delta(&Stat, "cpu.system", delay);
  cpu_idle   = hash_get_delta(&Stat, "cpu.idle",   delay);

  cpu_total  = cpu_user+cpu_nice+cpu_system+cpu_idle;
  
  if      (strcasecmp(key, "user"  )==0) value=cpu_user;
  else if (strcasecmp(key, "nice"  )==0) value=cpu_nice;
  else if (strcasecmp(key, "system")==0) value=cpu_system;
  else if (strcasecmp(key, "idle"  )==0) value=cpu_idle;
  else if (strcasecmp(key, "busy"  )==0) value=cpu_total-cpu_idle;
  
  if (cpu_total>0.0)
    value = 100*value/cpu_total;
  else
    value=0.0;

  SetResult(&result, R_NUMBER, &value); 
}


static void my_disk (RESULT *result, RESULT *arg1, RESULT *arg2, RESULT *arg3)
{
  char *dev, *key, buffer[32];
  int delay;
  double value;
  
  if (parse_proc_stat()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  dev   = R2S(arg1);
  key   = R2S(arg2);
  delay = R2N(arg3);
  
  qprintf(buffer, sizeof(buffer), "disk_io\\.%s\\.%s", dev, key);
  value  = hash_get_regex(&Stat, buffer, delay);
  
  SetResult(&result, R_NUMBER, &value); 
}


int plugin_init_proc_stat (void)
{
  AddFunction ("proc_stat", -1, my_proc_stat);
  AddFunction ("cpu",  2, my_cpu);
  AddFunction ("disk", 3, my_disk);
  return 0;
}

void plugin_exit_proc_stat(void) 
{
  if (stream!=NULL) {
    fclose (stream);
    stream=NULL;
  }
  hash_destroy(&Stat);
}
