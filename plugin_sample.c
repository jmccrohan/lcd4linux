/* $Id: plugin_sample.c,v 1.11 2005/11/04 04:53:10 reinelt Exp $
 *
 * plugin template
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
 * $Log: plugin_sample.c,v $
 * Revision 1.11  2005/11/04 04:53:10  reinelt
 * sample plugin activated
 *
 * Revision 1.10  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.9  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.8  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.7  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.6  2004/06/01 06:04:25  reinelt
 *
 * made README.Plugins and plugin_sample up to date.
 *
 * Revision 1.5  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.4  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.3  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.2  2004/01/13 10:03:01  reinelt
 * new util 'hash' for associative arrays
 * new plugin 'cpuinfo'
 *
 * Revision 1.1  2004/01/06 17:33:45  reinelt
 *
 * Evaluator: functions with variable argument lists
 * Evaluator: plugin_sample.c and README.Plugins added
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_sample (void)
 *  adds various functions
 *
 */


/* define the include files you need */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif



/* sample function 'mul2' */
/* takes one argument, a number */
/* multiplies the number by 2.0 */
/* Note: all local functions should be declared 'static' */

static void my_mul2(RESULT * result, RESULT * arg1)
{
    double param;
    double value;

    /* Get Parameter */
    /* R2N stands for 'Result to Number' */
    param = R2N(arg1);

    /* calculate value */
    value = param * 2.0;

    /* store result */
    /* when called with R_NUMBER, it assumes the */
    /* next parameter to be a pointer to double */
    SetResult(&result, R_NUMBER, &value);
}


/* sample function 'mul3' */
/* takes one argument, a number */
/* multiplies the number by 3.0 */
/* same as 'mul2', but shorter */

static void my_mul3(RESULT * result, RESULT * arg1)
{
    /* do it all in one line */
    double value = R2N(arg1) * 3.0;

    /* store result */
    SetResult(&result, R_NUMBER, &value);
}


/* sample function 'diff' */
/* takes two arguments, both numbers */
/* returns |a-b| */

static void my_diff(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    /* do it all in one line */
    double value = R2N(arg1) - R2N(arg2);

    /* some more calculations... */
    if (value < 0)
	value = -value;

    /* store result */
    SetResult(&result, R_NUMBER, &value);
}


/* sample function 'answer' */
/* takes no argument! */
/* returns the answer to all questions */

static void my_answer(RESULT * result)
{
    /* we have to declare a variable because */
    /* SetResult needs a pointer  */
    double value = 42;

    /* store result */
    SetResult(&result, R_NUMBER, &value);
}


/* sample function 'length' */
/* takes one argument, a string */
/* returns the string length */

static void my_length(RESULT * result, RESULT * arg1)
{
    /* Note #1: value *must* be double!  */
    /* Note #2: R2S stands for 'Result to String' */
    double value = strlen(R2S(arg1));

    /* store result */
    SetResult(&result, R_NUMBER, &value);
}



/* sample function 'upcase' */
/* takes one argument, a string */
/* returns the string in upper case letters */

static void my_upcase(RESULT * result, RESULT * arg1)
{
    char *value, *p;

    /* create a local copy of the argument */
    /* Do *NOT* try to modify the original string! */
    value = strdup(R2S(arg1));

    /* process the string */
    for (p = value; *p != '\0'; p++)
	*p = toupper(*p);

    /* store result */
    /* when called with R_STRING, it assumes the */
    /* next parameter to be a pointer to a string */
    /* 'value' is already a char*, so use 'value', not '&value' */
    SetResult(&result, R_STRING, value);

    /* free local copy again */
    /* Note that SetResult() makes its own string copy  */
    free(value);
}


/* sample function 'cat' */
/* takes variable number of arguments, all strings */
/* returns all prameters concatenated */

static void my_concat(RESULT * result, int argc, RESULT * argv[])
{
    int i, len;
    char *value, *part;

    /* start with a empty string */
    value = strdup("");

    /* process all arguments */
    for (i = 0; i < argc; i++) {
	part = R2S(argv[i]);
	len = strlen(value) + strlen(part);
	value = realloc(value, len + 1);
	strcat(value, part);
    }

    /* store result */
    SetResult(&result, R_STRING, value);

    /* free local string */
    free(value);
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_sample(void)
{

    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("sample::mul2", 1, my_mul2);
    AddFunction("sample::mul3", 1, my_mul3);
    AddFunction("sample::answer", 0, my_answer);
    AddFunction("sample::diff", 2, my_diff);
    AddFunction("sample::length", 1, my_length);
    AddFunction("sample::upcase", 1, my_upcase);
    AddFunction("sample::concat", -1, my_concat);

    return 0;
}

void plugin_exit_sample(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}
