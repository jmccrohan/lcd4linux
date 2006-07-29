/* $Id: plugin_string.c,v 1.9 2006/07/29 21:12:31 lfcorreia Exp $
 *
 * string plugin
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin_string.c,v $
 * Revision 1.9  2006/07/29 21:12:31  lfcorreia
 * Add UPPERCASE string plugin function
 *
 * Revision 1.8  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
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
 * Revision 1.4  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.3  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.2  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.1  2003/12/19 05:50:34  reinelt
 * added plugin_math.c and plugin_string.c
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_string (void)
 *  adds some handy string functions
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "plugin.h"


static void my_strlen(RESULT * result, RESULT * arg1)
{
    double value = strlen(R2S(arg1));
    SetResult(&result, R_NUMBER, &value);
}


int plugin_init_string(void)
{

    /* register some basic string functions */
    AddFunction("strlen", 1, my_strlen);

    return 0;
}

void plugin_exit_string(void)
{
    /* empty */
}

/* 'upcase' function (shamelessly stolen from plugin_sample.c)*/
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

int plugin_init_strupper(void)
{

    /* register my UPPERCASE transforming function */
    AddFunction("strupper", 1, my_upcase);

    return 0;
}

void plugin_exit_strupper(void)
{
    /* empty */
}
