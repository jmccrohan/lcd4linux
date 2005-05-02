/* $Id: plugin_python.c,v 1.1 2005/05/02 10:29:20 reinelt Exp $
 *
 * Python plugin
 *
 * Copyright 2005 Fixme!
 * Copyright 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: plugin_python.c,v $
 * Revision 1.1  2005/05/02 10:29:20  reinelt
 * preparations for python bindings and python plugin
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_python (void)
 *  adds a python interpreter
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"

// Fixme: #include "lcd4linux_wrap.h"

static void my_exec (RESULT *result, RESULT *arg1)
{
#if 0
  const char* value = pyt_exec_str(R2S(arg1));
#else
  const char *value = "Fixme";
#endif
  SetResult(&result, R_STRING, value); 
}

int plugin_init_python (void)
{
  AddFunction ("python::exec", 1, my_exec);
  return 0;
}

void plugin_exit_python (void) 
{
  /* empty */
}
