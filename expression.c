/* $Id: expression.c,v 1.1 2003/10/06 04:34:06 reinelt Exp $
 *
 * expression handling
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
 * $Log: expression.c,v $
 * Revision 1.1  2003/10/06 04:34:06  reinelt
 * expression evaluator added
 *
 */

/* 
 * exported functions:
 *
 * int EX_init (void)
 *  initializes the expression evaluator
 *  adds some handy constants and functions
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "evaluator.h"
#include "expression.h"



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


static void my_strlen (RESULT *result, RESULT *arg1)
{
  double value=strlen(R2S(arg1));
  SetResult(&result, R_NUMBER, &value); 
}


static void my_cfg (RESULT *result, RESULT *arg1)
{
  char *value=cfg_get(R2S(arg1), "");
  SetResult(&result, R_STRING, value); 
}


int EX_init (void)
{
  // set some handy constants
  AddConstant ("Pi", M_PI);
  AddConstant ("e",  M_E);
  
  // register some basic math functions
  AddFunction ("sqrt", 1, my_sqrt);
  AddFunction ("exp",  1, my_exp);
  AddFunction ("ln",   1, my_ln);
  AddFunction ("log",  1, my_log);
  AddFunction ("sin",  1, my_sin);
  AddFunction ("cos",  1, my_cos);
  AddFunction ("tan",  1, my_tan);
  
  // register some basic string functions
  AddFunction ("strlen", 1, my_strlen);

  // config file access
  AddFunction ("cfg", 1, my_cfg);

  
  return 0;
}


#if 1

int EX_test(int argc, char* argv[])
{
  int ec;
  char line[1024];
  RESULT result = {0, 0.0, NULL};
  
  printf("\nEE> ");
  for(gets(line); !feof(stdin); gets(line)) {
    ec=Eval(line, &result);
    if (result.type==R_NUMBER) {
      printf ("%g\n", R2N(&result));
    } else if (result.type==R_STRING) {
      printf ("<%s>\n", R2S(&result));
    }
    printf("EE> ");
  }

  return 0;
}
#endif
