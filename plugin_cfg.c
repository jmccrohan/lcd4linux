/* $Id: plugin_cfg.c,v 1.3 2004/01/29 04:40:02 reinelt Exp $
 *
 * plugin for config file access
 *
 * Copyright 2003, 2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin_cfg.c,v $
 * Revision 1.3  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.2  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.1  2004/01/13 10:03:01  reinelt
 * new util 'hash' for associative arrays
 * new plugin 'cpuinfo'
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_cfg (void)
 *  adds cfg() function for config access
 *  initializes variables from the config file
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "evaluator.h"
#include "plugin.h"
#include "cfg.h"


static void load_variables (void)
{
  char *section="Variables";
  char *list, *key;
  char *expression;
  RESULT result = {0, 0.0, NULL};
  
  list=cfg_list(section);
  key=strtok(list, "|");
  while (key!=NULL) {
    if (strchr(key, '.')!=NULL || strchr(key, ':') !=0) {
      error ("ignoring variable '%s' from %s: structures not allowed", key, cfg_source());
    } else {
      expression=cfg_get_raw (section, key, "");
      if (expression!=NULL && *expression!='\0') {
	if (Eval(expression, &result)==0) {
	  debug ("Variable %s = '%s' (%f)", key, R2S(&result), R2N(&result));
	  SetVariable (key, &result);
	  DelResult (&result);
	} else {
	  error ("error evaluating variable '%s' from %s", key, cfg_source());
	}
      }
    }
    key=strtok(NULL, "|");
  }
  free (list);
  
}


static void my_cfg (RESULT *result, int argc, RESULT *argv[])
{
  int i, len;
  char *value;
  char *buffer;
  
  // calculate key length
  len=0;
  for (i=0; i<argc; i++) {
    len+=strlen(R2S(argv[i]))+1;
  }
  
  // allocate key buffer
  buffer=malloc(len+1);
  
  // prepare key buffer
  *buffer='\0';
  for (i=0; i<argc; i++) {
    strcat (buffer, ".");
    strcat (buffer, R2S(argv[i]));
  }
  
  // buffer starts with '.', so cut off first char
  value=cfg_get("", buffer+1, "");
  
  // free buffer again
  free (buffer);
  
  // store result
  SetResult(&result, R_STRING, value); 
}


int plugin_init_cfg (void)
{
  // load "Variables" section from cfg
  load_variables();

  // register plugin
  AddFunction ("cfg", -1, my_cfg);
  
  return 0;
}
