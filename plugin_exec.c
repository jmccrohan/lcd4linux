/* $Id: plugin_exec.c,v 1.1 2004/03/20 11:49:40 reinelt Exp $
 *
 * plugin for external processes
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old 'exec' client which is
 * Copyright 2001 Leopold Tötsch <lt@toetsch.at>
 *
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
 * $Log: plugin_exec.c,v $
 * Revision 1.1  2004/03/20 11:49:40  reinelt
 * forgot to add plugin_exec.c ...
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_exec (void)
 *  adds functions to start external pocesses
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// #include <string.h>
// #include <ctype.h>
// #include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"
#include "cfg.h"
#include "thread.h"

static HASH EXEC = { 0, };
static int fatal = 0;

static int do_exec (void)
{
  int age;
  
  // if a fatal error occured, do nothing
  if (fatal != 0) return -1;
  
  // reread every 100 msec only
  age=hash_age(&EXEC, NULL, NULL);
  if (age>0 && age<=100) return 0;
  return 0;
} 

static void my_junk (char *name)
{
  int i;
  
  debug ("junk thread starting!");
  
  for (i=0; i<10; i++) {
    debug ("junk look %d", i);
    sleep (1);
  }
  debug ("junk thread done!");
}


static void my_exec (RESULT *result, int argc, RESULT *argv[])
{
  char *key, *val;
  
  if (do_exec()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
  
  // key=R2S(arg1);
  val=hash_get(&EXEC, key);
  if (val==NULL) val="";
  
  SetResult(&result, R_STRING, val); 
}


int plugin_init_exec (void)
{
  int junk;
  
  AddFunction ("exec", -1, my_exec);

  // junk=thread_create ("Junk", my_junk);
  // debug ("junk=%d", junk);

  return 0;
}


void plugin_exit_exec(void) 
{
  hash_destroy(&EXEC);
}
