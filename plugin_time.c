/* $Id$
 * $URL$
 *
 * time plugin
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
 */

/* 
 * exported functions:
 *
 * int plugin_init_time (void)
 *  adds some handy time functions
 *
 */


#include "config.h"

#include <time.h>

#include "debug.h"
#include "plugin.h"


static void my_time(RESULT * result)
{
    double value = time(NULL);
    SetResult(&result, R_NUMBER, &value);
}


static void my_strftime(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char value[256];
    time_t t = R2N(arg2);

    value[0] = '\0';
    strftime(value, sizeof(value), R2S(arg1), localtime(&t));

    SetResult(&result, R_STRING, value);
}


int plugin_init_time(void)
{

    /* register some basic time functions */
    AddFunction("time", 0, my_time);
    AddFunction("strftime", 2, my_strftime);

    return 0;
}

void plugin_exit_time(void)
{
    /* empty */
}
