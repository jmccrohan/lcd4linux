/* $Id: expr.c,v 1.3 2004/01/12 03:51:01 reinelt Exp $
 *
 * expr ('y*') functions
 * This is only a workaround to make the Evaluator usable until
 * it's fully integrated into lcd4linux.
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: expr.c,v $
 * Revision 1.3  2004/01/12 03:51:01  reinelt
 * evaluating the 'Variables' section in the config file
 *
 * Revision 1.2  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.1  2004/01/05 11:57:38  reinelt
 * added %y tokens to make the Evaluator useable
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#define IN_EXPR
#include "expr.h"
#include "plugin.h"
#include "debug.h"
#include "cfg.h"


int Expr(int index, char string[EXPR_TXT_LEN], double *val)
{
  RESULT result = {0, 0.0, NULL};
  static int errs[EXPRS+1];
  char *expression;
  char yn[4];
  
  if (index < 0 || index > EXPRS)
    return -1; 
  
  if (errs[index])
    return -1;

  sprintf(yn, "y%d", index);
  expression = cfg_get(NULL, yn, NULL);
					    
  if (!expression || !*expression) {
    error("Empty expression for 'y%d'", index);
    errs[index]++;
    return -1;
  }

  Eval(expression, &result);
  strcpy(string, R2S(&result));
  
  debug("%s: <%s> = '%s'",yn, expression, result);

  if (isdigit(*string)) {
    double max, min;
    *val = atof(string);
    sprintf(yn, "Max_y%d", index);
    max = atof(cfg_get(NULL, yn, "100"));
    sprintf(yn, "Min_y%d", index);
    min = atof(cfg_get(NULL, yn, "0"));
    if (max != min)
      *val = (*val - min)/(max - min);
  }

  DelResult (&result);

  return 0;
}

