/* $Id: plugin_netdev.c,v 1.9 2004/06/17 06:23:43 reinelt Exp $
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
 * Revision 1.9  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.8  2004/05/27 03:39:47  reinelt
 *
 * changed function naming scheme to plugin::function
 *
 * Revision 1.7  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.6  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
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
#include "qprintf.h"
#include "hash.h"


static HASH NetDev;
static FILE *stream = NULL;

// Fixme: use new & fast hash code!!!

static void hash_put3 (char *key1, char *key2, char *key3, char *val) 
{
  char key[32];
  
  qprintf(key, sizeof(key), "%s.%s.%s", key1, key2, key3);
  hash_put_delta (&NetDev, key, val);
}


static int parse_netdev (void)
{
  int age;
  int line;
  int  RxTx=0; // position of Receive/Transmit switch
  
  // reread every 10 msec only
  age=hash_age(&NetDev, NULL);
  if (age>0 && age<=10) return 0;
  
  if (stream==NULL) stream=fopen("/proc/net/dev", "r");
  if (stream==NULL) {
    error ("fopen(/proc/net/dev) failed: %s", strerror(errno));
    return -1;
  }

  rewind(stream);    
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
	  hash_put3 (iface, (h-buffer) < RxTx ? "Rx" : "Tx", head[col], h);
	}
	h=t?t+1:NULL;
	col++;
      }
    }
  }
  
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
  
  value  = hash_get_regex(&NetDev, key, NULL, delay);
  
  SetResult(&result, R_NUMBER, &value); 
}

static void my_netdev_fast(RESULT *result, RESULT *arg1, RESULT *arg2)
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
  
  value  = hash_get_delta(&NetDev, key, NULL, delay);
  
  SetResult(&result, R_NUMBER, &value); 
}


int plugin_init_netdev (void)
{
  hash_create(&NetDev);
  AddFunction ("netdev",       2, my_netdev);
  AddFunction ("netdev::fast", 2, my_netdev_fast);
  return 0;
}

void plugin_exit_netdev(void) 
{
  if(stream!=NULL) {
    fclose (stream);
    stream=NULL;
  }
  hash_destroy(&NetDev);
}
