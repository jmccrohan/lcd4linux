/* $Id: plugin_loadavg.c,v 1.1 2004/01/15 04:29:45 reinelt Exp $
 *
 * plugin for load average
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
 * $Log: plugin_loadavg.c,v $
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
 * int plugin_init_loadavg (void)
 *  adds functions for load average
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"


static void my_loadavg (RESULT *result, RESULT *arg1)
{
  int nelem, index;
  double loadavg[3];
  
  nelem=getloadavg(loadavg, 3);
  if (nelem<0) {
    error ("getloadavg() failed!");
    SetResult(&result, R_STRING, "");
    return;
  }

  index=R2N(arg1);
  if (index<1 || index>nelem) {
    error ("loadavg(%d): index out of range!", index);
    SetResult(&result, R_STRING, "");
    return;
  }
  
  SetResult(&result, R_NUMBER, &(loadavg[index-1]));
  return;
  
}


int plugin_init_loadavg (void)
{
  AddFunction ("loadavg", 1, my_loadavg);
  return 0;
}

