/* $Id: plugin_ppp.c,v 1.1 2004/01/27 08:13:39 reinelt Exp $
 *
 * plugin for ppp throughput
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
 * $Log: plugin_ppp.c,v $
 * Revision 1.1  2004/01/27 08:13:39  reinelt
 * ported PPP token to plugin_ppp
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_ppp (void)
 *  adds ppp() function
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#ifdef HAVE_NET_IF_PPP_H
#include <net/if_ppp.h>
#else
#warning if_ppp.h not found. PPP support deactivated.
#endif

#include "debug.h"
#include "plugin.h"
#include "hash.h"


#ifdef HAVE_NET_IF_PPP_H

static HASH PPP = { 0, };

static int get_ppp_stats (void)
{
  int age;
  int unit;
  unsigned ibytes, obytes;
  static int fd=-2;
  struct ifpppstatsreq req;
  char key[8];

  // reread every 10 msec only
  age=hash_age(&PPP, NULL, NULL);
  if (age>0 && age<=10) return 0;
  
  // open socket only once
  if (fd==-2) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) {
      error ("socket() failed: %s", strerror(errno));
      return -1;
    }
  }
  
  for (unit=0; unit<8; unit++) {
    debug ("query unit %d", unit);
    memset (&req, 0, sizeof (req));
    req.stats_ptr = (caddr_t) &req.stats;
    snprintf (req.ifr__name, sizeof(req.ifr__name), "ppp%d", unit);
    
    if (ioctl(fd, SIOCGPPPSTATS, &req) == 0) {
      ibytes=req.stats.p.ppp_ibytes;
      obytes=req.stats.p.ppp_obytes;
    } else {
      debug ("ioctl() retuned 0");
      ibytes=obytes=0;
    }
    // Fixme: hash_set_delta wants a string, we already have an integer
    snprintf (key, sizeof(key), "Rx:%d", unit);
    hash_set_delta (&PPP, key, ibytes);
    snprintf (key, sizeof(key), "Tx:%d", unit);
    hash_set_delta (&PPP, key, obytes);

  }
  return 0;
}


static void my_ppp (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  double value;
  
  debug ("get stats...");
  if (get_ppp_stats()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  debug ("get stats done.");
  
  value=hash_get_delta(&PPP, R2S(arg1), R2N(arg2));
  SetResult(&result, R_NUMBER, &value); 
}

#endif


int plugin_init_ppp (void)
{
#ifdef HAVE_NET_IF_PPP_H
  AddFunction ("ppp", 3, my_ppp);
#endif
  return 0;
}

