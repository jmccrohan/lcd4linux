/* $Id$
 * $URL$
*
* Handy functions for testing displays and debugging code.
*
* Copyright (C) 2004 Andy Baxter.
*
* Based on sample plugin which is
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

int plugin_init_test(void);

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* used for testing bars - keeps values for a series of 10 bars,
 * which are incremented and decremented between 0 and rmax by
 * amount rdelta every time they are read. Starting value is rstart.
 * rbar gives the number of the test bar. 
 */
static void my_test_bar(RESULT * result, RESULT * rbar, RESULT * rmax, RESULT * rstart, RESULT * rdelta)
{
    static double values[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    static double deltas[10];
    int bar;
    double max, delta, value;

    max = R2N(rmax);
    delta = R2N(rdelta);

    /* the maths is just to stop double rounding errors and bad values. */
    bar = ((int) floor(R2N(rbar) + 0.1)) % 10;
    if (fabs(delta) > 0.1) {
	/* don't move or init the bar if delta=0 (the widget is only browsing) */
	if (values[bar] == -1) {
	    /* first time called. */
	    values[bar] = R2N(rstart);
	    deltas[bar] = delta;
	};
	values[bar] += deltas[bar];
    };
    if (values[bar] < 0 || values[bar] > max) {
	/* turn around. */
	deltas[bar] = -deltas[bar];
	values[bar] += deltas[bar];
    };
    value = values[bar];
    SetResult(&result, R_NUMBER, &value);
}


/* like above, but just switches a value between 1 and -1. Can use to test
 * visibility of icons. 
 */
static void my_test_onoff(RESULT * result, RESULT * arg1)
{
    static int on[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    int i;
    double val;

    i = ((int) floor(R2N(arg1) + 0.1)) % 10;
    on[i] = -on[i];
    val = (double) on[i];

    SetResult(&result, R_NUMBER, &val);
}


int plugin_init_test(void)
{

    AddFunction("test::bar", 4, my_test_bar);
    AddFunction("test::onoff", 1, my_test_onoff);

    return 0;
}

void plugin_exit_test(void)
{
    /* empty */
}
