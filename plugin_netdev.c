/* $Id: plugin_netdev.c,v 1.5 2004/03/03 03:47:04 reinelt Exp $
 *
 * plugin for /proc/net/dev parsing
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
 * $Log: plugin_netdev.c,v $
 * Revision 1.5  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.4  2004/02/15 07:23:04  reinelt
 * bug in netdev parsing fixed
 *
 * Revision 1.3  2004/02/01 19:37:40  reinelt
 * got rid of every strtok() incarnation.
 *
 * Revision 1.2  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.1  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_netdev (void)
 *  adds functions to access /proc/net/dev
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
#include "qprintf.h"


static HASH NetDev = { 0, };


static void hash_set3 (char *key1, char *key2, char *key3, char *val) 
{
  char key[32];
  
  qprintf(key, sizeof(key), "%s.%s.%s", key1, key2, key3);
  hash_set_delta (&NetDev, key, val);
}


static int parse_netdev (void)
{
  int age;
  int line;
  int  RxTx=0; // position of Receive/Transmit switch
  FILE *stream;
  
  // reread every 10 msec only
  age=hash_age(&NetDev, NULL, NULL);
  if (age>0 && age<=10) return 0;
  
  stream=fopen("/proc/net/dev", "r");
  if (stream==NULL) {
    error ("fopen(/proc/net/dev) failed: %s", strerror(errno));
    return -1;
  }
    
  line=0;
  while (!feof(stream)) {
    char buffer[256];
    char header[256], *h, *t;
    char *head[64];
    char delim[]=" :|\t\n";
    int  col;
    
    if (fgets (buffer, sizeof(buffer), stream) == NULL) break;

    // skip line 1
    if (++line<2) continue;

    // line 2 used for headers
    if (line==2) {
      strncpy(header, buffer, sizeof(header));
      RxTx=(strrchr(header, '|') - header);
      col=0;
      h=header;
      while (h!=NULL) {
	while (strchr(delim, *h)) h++;
	if ((t=strpbrk(h, delim))!=NULL) *t='\0';
	head[col++]=h;
	h=t?t+1:NULL;
      }
    } else {
      char *h, *iface=NULL;
      col=0;
      h=buffer;
      while (h!=NULL) {
	while (strchr(delim, *h)) h++;
	if ((t=strpbrk(h, delim))!=NULL) *t='\0';
	if (col==0) {
	  iface=h;
	} else {
	  hash_set3 (iface, (h-buffer) < RxTx ? "Rx" : "Tx", head[col], h);
	}
	h=t?t+1:NULL;
	col++;
      }
    }
  }
  fclose (stream);
  return 0;
}  

static void my_netdev (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  char *key;
  int delay;
  double value;
  
  if (parse_netdev()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }

  key   = R2S(arg1);
  delay = R2N(arg2);
  
  value  = hash_get_regex(&NetDev, key, delay);
  
  SetResult(&result, R_NUMBER, &value); 
}


int plugin_init_netdev (void)
{
  AddFunction ("netdev", 2, my_netdev);
  return 0;
}

void plugin_exit_netdev(void) 
{
	hash_destroy(&NetDev);
}
