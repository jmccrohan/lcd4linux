/* $Id: plugin_cfg.c,v 1.7 2004/03/06 20:31:16 reinelt Exp $
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
 * Revision 1.7  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
 * Revision 1.6  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.5  2004/02/01 19:37:40  reinelt
 * got rid of every strtok() incarnation.
 *
 * Revision 1.4  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
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

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static void load_variables (void)
{
  char *section = "Variables";
  char *list, *l, *p;
  char *expression;
  void *tree;
  RESULT result = {0, 0.0, NULL};
  
  list=cfg_list(section);
  l=list;
  while (l!=NULL) {
    while (*l=='|') l++;
    if ((p=strchr(l, '|'))!=NULL) *p='\0';
    if (strchr(l, '.')!=NULL || strchr(l, ':') !=0) {
      error ("ignoring variable '%s' from %s: structures not allowed", l, cfg_source());
    } else {
      expression=cfg_get_raw (section, l, "");
      if (expression!=NULL && *expression!='\0') {
	tree = NULL;
        if (Compile(expression, &tree) == 0 && Eval(tree, &result)==0) {
          debug ("Variable %s = '%s' (%f)", l, R2S(&result), R2N(&result));
          SetVariable (l, &result);
          DelResult (&result);
        } else {
          error ("error evaluating variable '%s' from %s", list, cfg_source());
        }
	DelTree (tree);
      }
    }
    l=p?p+1:NULL;
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
    
  // store result
  SetResult(&result, R_STRING, value); 

  // free buffer again
  free (buffer);
  
  free(value);
}


int plugin_init_cfg (void)
{
  // load "Variables" section from cfg
  load_variables();

  // register plugin
  AddFunction ("cfg", -1, my_cfg);
  
  return 0;
}

void plugin_exit_cfg(void) 
{
	
}
