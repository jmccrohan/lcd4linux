/* $Id: plugin_math.c,v 1.3 2004/03/03 03:47:04 reinelt Exp $
 *
 * math plugin
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
 * $Log: plugin_math.c,v $
 * Revision 1.3  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.2  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.1  2003/12/19 05:50:34  reinelt
 * added plugin_math.c and plugin_string.c
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_math (void)
 *  adds some handy constants and functions
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "debug.h"
#include "plugin.h"


static void my_sqrt (RESULT *result, RESULT *arg1)
{
  double value=sqrt(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}

static void my_exp (RESULT *result, RESULT *arg1)
{
  double value=exp(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}

static void my_ln (RESULT *result, RESULT *arg1)
{
  double value=log(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}

static void my_log (RESULT *result, RESULT *arg1)
{
  double value=log10(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}

static void my_sin (RESULT *result, RESULT *arg1)
{
  double value=sin(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}

static void my_cos (RESULT *result, RESULT *arg1)
{
  double value=cos(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}

static void my_tan (RESULT *result, RESULT *arg1)
{
  double value=tan(R2N(arg1));
  SetResult(&result, R_NUMBER, &value); 
}


static void my_min (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  double a1=R2N(arg1);
  double a2=R2N(arg2);
  double value=a1<a2?a1:a2;
  SetResult(&result, R_NUMBER, &value); 
}

static void my_max (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  double a1=R2N(arg1);
  double a2=R2N(arg2);
  double value=a1>a2?a1:a2;
  SetResult(&result, R_NUMBER, &value); 
}


int plugin_init_math (void)
{
  // set some handy constants
  AddNumericVariable ("Pi", M_PI);
  AddNumericVariable ("e",  M_E);
  
  // register some basic math functions
  AddFunction ("sqrt", 1, my_sqrt);
  AddFunction ("exp",  1, my_exp);
  AddFunction ("ln",   1, my_ln);
  AddFunction ("log",  1, my_log);
  AddFunction ("sin",  1, my_sin);
  AddFunction ("cos",  1, my_cos);
  AddFunction ("tan",  1, my_tan);
  
  // min, max
  AddFunction ("min",  2, my_min);
  AddFunction ("max",  2, my_max);

  return 0;
}

void plugin_exit_math(void) 
{
}
