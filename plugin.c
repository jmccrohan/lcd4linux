/* $Id: plugin.c,v 1.2 2003/12/19 05:49:23 reinelt Exp $
 *
 * plugin handler for the Evaluator
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin.c,v $
 * Revision 1.2  2003/12/19 05:49:23  reinelt
 * extracted plugin_math and plugin_string into extra files
 *
 * Revision 1.1  2003/12/19 05:35:14  reinelt
 * renamed 'client' to 'plugin'
 *
 * Revision 1.1  2003/10/11 06:01:52  reinelt
 *
 * renamed expression.{c,h} to client.{c,h}
 * added config file client
 * new functions 'AddNumericVariable()' and 'AddStringVariable()'
 * new parameter '-i' for interactive mode
 *
 * Revision 1.3  2003/10/06 05:51:15  reinelt
 * functions: min(), max()
 *
 * Revision 1.2  2003/10/06 05:47:27  reinelt
 * operators: ==, \!=, <=, >=
 *
 * Revision 1.1  2003/10/06 04:34:06  reinelt
 * expression evaluator added
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init (void)
 *  initializes the expression evaluator
 *  adds some handy constants and functions
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "plugin.h"


int plugin_init_math (void);
int plugin_init_string (void);


int plugin_init (void)
{
  // math plugin
  plugin_init_math();
  plugin_init_string();
  
  
  return 0;
}


