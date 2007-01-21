/* $Id$
 * $URL$
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


static void my_sqrt(RESULT * result, RESULT * arg1)
{
    double value = sqrt(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}

static void my_exp(RESULT * result, RESULT * arg1)
{
    double value = exp(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}

static void my_ln(RESULT * result, RESULT * arg1)
{
    double value = log(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}

static void my_log(RESULT * result, RESULT * arg1)
{
    double value = log10(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}

static void my_sin(RESULT * result, RESULT * arg1)
{
    double value = sin(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}

static void my_cos(RESULT * result, RESULT * arg1)
{
    double value = cos(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}

static void my_tan(RESULT * result, RESULT * arg1)
{
    double value = tan(R2N(arg1));
    SetResult(&result, R_NUMBER, &value);
}


static void my_min(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    double a1 = R2N(arg1);
    double a2 = R2N(arg2);
    double value = a1 < a2 ? a1 : a2;
    SetResult(&result, R_NUMBER, &value);
}

static void my_max(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    double a1 = R2N(arg1);
    double a2 = R2N(arg2);
    double value = a1 > a2 ? a1 : a2;
    SetResult(&result, R_NUMBER, &value);
}

static void my_floor(RESULT * result, RESULT * arg)
{
    double value = floor(R2N(arg));
    SetResult(&result, R_NUMBER, &value);
}

static void my_ceil(RESULT * result, RESULT * arg)
{
    double value = ceil(R2N(arg));
    SetResult(&result, R_NUMBER, &value);
}

static void my_decode(RESULT * result, int argc, RESULT * argv[])
{
    int index;
    
    if (argc < 2) {
	error("decode(): wrong number of parameters");
	SetResult(&result, R_STRING, "");
	return;
    }

    index = R2N(argv[0]);
    
    if (index < 0 || index >= argc-1) {
	SetResult(&result, R_STRING, "");
	return;
    }
    
    CopyResult (&result, argv[index+1]);
}


int plugin_init_math(void)
{
    /* set some handy constants */
    SetVariableNumeric("Pi", M_PI);
    SetVariableNumeric("e", M_E);

    /* register some basic math functions */
    AddFunction("sqrt", 1, my_sqrt);
    AddFunction("exp", 1, my_exp);
    AddFunction("ln", 1, my_ln);
    AddFunction("log", 1, my_log);
    AddFunction("sin", 1, my_sin);
    AddFunction("cos", 1, my_cos);
    AddFunction("tan", 1, my_tan);

    /* min, max */
    AddFunction("min", 2, my_min);
    AddFunction("max", 2, my_max);

    /* floor, ceil */
    AddFunction("floor", 1, my_floor);
    AddFunction("ceil", 1, my_ceil);

    /* decode */
    AddFunction("decode", -1, my_decode);

    return 0;
}

void plugin_exit_math(void)
{
}
