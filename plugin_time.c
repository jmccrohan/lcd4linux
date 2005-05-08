/* $Id: plugin_time.c,v 1.5 2005/05/08 04:32:45 reinelt Exp $
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
 *
 * $Log: plugin_time.c,v $
 * Revision 1.5  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.4  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.3  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.2  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.1  2004/05/20 07:47:51  reinelt
 * added plugin_time
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
