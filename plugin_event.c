/* $Id: plugin_event.c -1   $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/plugin_event.c $
 *
 * plugin template
 *
 * Copyright (C) 2003 Ed Martin <edman007@edman007.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * int plugin_init_event (void)
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
#include "event.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif



/* function 'trigger' */
/* takes one argument, a string */
/* triggers the event */

static void my_trigger(RESULT * result, RESULT * arg1)
{
    char *param;

    /* Get Parameter */
    /* R2N stands for 'Result to Number' */
    param = R2S(arg1);
    named_event_trigger(param);

    char *value = "";

    /* store result */
    /* when called with R_NUMBER, it assumes the */
    /* next parameter to be a pointer to double */
    SetResult(&result, R_NUMBER, &value);
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_event(void)
{

    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("event::trigger", 1, my_trigger);


    return 0;
}

void plugin_exit_event(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}
