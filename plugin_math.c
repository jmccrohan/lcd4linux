/* $Id: plugin_math.c,v 1.8 2005/04/05 04:46:06 reinelt Exp $
 *
 * math plugin
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * Revision 1.8  2005/04/05 04:46:06  reinelt
 * ceil/floor patch from Maxime
 *
 * Revision 1.7  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.6  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.5  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.4  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
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

static void my_floor (RESULT *result, RESULT *arg)
{
  double value = floor(R2N(arg));
  SetResult(&result, R_NUMBER, &value);
}

static void my_ceil (RESULT *result, RESULT *arg)
{
  double value = ceil(R2N(arg));
  SetResult(&result, R_NUMBER, &value);
}


int plugin_init_math (void)
{
  /* set some handy constants */
  SetVariableNumeric ("Pi", M_PI);
  SetVariableNumeric ("e",  M_E);
  
  /* register some basic math functions */
  AddFunction ("sqrt", 1, my_sqrt);
  AddFunction ("exp",  1, my_exp);
  AddFunction ("ln",   1, my_ln);
  AddFunction ("log",  1, my_log);
  AddFunction ("sin",  1, my_sin);
  AddFunction ("cos",  1, my_cos);
  AddFunction ("tan",  1, my_tan);
  
  /* min, max */
  AddFunction ("min",  2, my_min);
  AddFunction ("max",  2, my_max);

  /* floor, ceil */
  AddFunction ("floor", 1, my_floor);
  AddFunction ("ceil",  1, my_ceil);

  return 0;
}

void plugin_exit_math(void) 
{
}
