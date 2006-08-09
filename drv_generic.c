/* $Id: drv_generic.c,v 1.5 2006/08/09 17:25:34 harbaum Exp $
 *
 * generic driver helper
 *
 * Copyright (C) 2006 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_generic.c,v $
 * Revision 1.5  2006/08/09 17:25:34  harbaum
 * Better bar color support and new bold font
 *
 * Revision 1.4  2006/07/31 03:48:09  reinelt
 * preparations for scrolling
 *
 */

/* 
 *
 * exported functions:
 *
 * drv_generic_init (void)
 *   initializes generic stuff and registers plugins
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "plugin.h"
#include "drv_generic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* these values are chars (text displays) or pixels (graphic displays) */

int LROWS = 0;			/* layout size: rows */
int LCOLS = 0;			/* layout size: columns */

int DROWS = 4;			/* display size:  rows */
int DCOLS = 20;			/* display size: columns */

int XRES = 6;			/* pixel widtht of one char */
int YRES = 8;			/* pixel height of one char */

int FONT_STYLE = 0;             /* font style (default = plain) */

void (*drv_generic_blit) () = NULL;


static void my_drows(RESULT * result)
{
    double value = DROWS;
    SetResult(&result, R_NUMBER, &value);
}

static void my_dcols(RESULT * result)
{
    double value = DCOLS;
    SetResult(&result, R_NUMBER, &value);
}

static void my_xres(RESULT * result)
{
    double value = XRES;
    SetResult(&result, R_NUMBER, &value);
}

static void my_yres(RESULT * result)
{
    double value = YRES;
    SetResult(&result, R_NUMBER, &value);
}

static void my_lrows(RESULT * result)
{
    double value = LROWS;
    SetResult(&result, R_NUMBER, &value);
}

static void my_lcols(RESULT * result)
{
    double value = LCOLS;
    SetResult(&result, R_NUMBER, &value);
}

int drv_generic_init(void)
{

    AddFunction("LCD::height", 0, my_drows);
    AddFunction("LCD::width", 0, my_dcols);

    AddFunction("LCD::xres", 0, my_xres);
    AddFunction("LCD::yres", 0, my_yres);

    AddFunction("Layout::height", 0, my_lrows);
    AddFunction("Layout::width", 0, my_lcols);

    return 0;
}
