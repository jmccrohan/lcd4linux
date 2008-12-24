/* $Id: plugin_w1retap.c
 *
 * plugin to perform simple file operations for w1retap
 *
 * Copyright (C) 2007 Jonathan Hudson <jh+w1retap@daria.co.uk>
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
 */

/*
 * This module returns a specific value associated with a key from a
 * file containing one or more lines formatted as KEY=VALUE.
 *
 * It is intended for use with the w1retap weather station logging
 * software <http://www.daria.co.uk/wx/tech.html>, but is applicable
 * to any similarly formatted file.
 */

/* 
 * exported functions:
 *
 * int plugin_init_file (void)
 *  adds various functions
 *
 */


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* function 'my_readkey' */
/* takes two arguments, file name and key */
/* returns value associated with key, trimmed to suitable dec places */

static void my_readkey(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char value[80], val2[80];
    FILE *fp;
    char *reqkey, *pval;
    size_t size;

    *value = 0;
    pval = value;

    reqkey = R2S(arg2);
    fp = fopen(R2S(arg1), "r");

    if (!fp) {
	error("w1retap couldn't open file '%s'", R2S(arg1));
	value[0] = '\0';
    } else {
	size = strlen(reqkey);
	while (!feof(fp) && pval == value) {
	    fgets(val2, sizeof(val2), fp);
	    if (*(val2 + size) == '=') {
		if (strncmp(val2, reqkey, size) == 0) {
		    char *p;
		    p = index(val2, ' ');
		    if (p == NULL) {
			p = index(val2, '\n');
		    }

		    if (p) {
			*p = 0;
			pval = val2 + size + 1;
			{
			    double d;
			    char *ep;
			    d = strtod(pval, &ep);
			    if (ep != pval && (*ep == 0 || isspace(*ep))) {
				if (d > 500)
				    sprintf(val2, "%.0f", d);
				else
				    sprintf(val2, "%.1f", d);
				pval = val2;
			    }
			}
		    }
		}
	    }
	}
	fclose(fp);
    }

    /* store result */
    SetResult(&result, R_STRING, pval);
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_w1retap(void)
{

    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("w1retap::readkey", 2, my_readkey);

    return 0;
}

void plugin_exit_w1retap(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}


/*

#Sample configuration for w1retap
#================================

Display Weather {
   Driver   'Pertelian'
   Port     '/dev/ttyUSB0'
   Size     '20x4'
   Backlight 1
   Icons     0
}

Display XWindow {
    Driver     'X11'
    Size       '120x32'
    Font       '6x8'
    Pixel      '4+1'
    Gap        '-1x-1'
    Border      20
    Foreground '000000ff'
    Background '00000033'
    Basecolor '70c000'
    Buttons   6
    Brightness 133
}

Widget LCDTimer {
   class 'Timer'
   active 1
   update 10000
   expression h=strftime('%H', time()); x=file::readline('/tmp/.lcd',1)+0; b=(h > 5 | h < 23) + x ; LCD::backlight(b)
}


Widget Temp {
    class 'Text'
    expression w1retap::readkey(file, 'OTMP0')
    prefix 'Extr '
    width 10
    align 'L'
    update tick
}

Widget GHouse {
    class 'Text'
    expression w1retap::readkey(file, 'GHT')
    prefix 'GHT '
    width 10
    align 'L'
    update tick
}

Widget House {
    class 'Text'
    expression w1retap::readkey(file, 'OTMP1')
    prefix 'Intr '
    width 10
    align 'L'
    update tick
}
Widget Garage {
    class 'Text'
    expression w1retap::readkey(file, 'OTMP2')
    prefix 'Gge '
    width 10
    align 'L'
    update tick
}
Widget Soil {
    class 'Text'
    expression w1retap::readkey(file, 'STMP1')
    prefix 'Soil '
    width 10
    align 'L'
    update tick
}
Widget Press {
    class 'Text'
    expression w1retap::readkey(file, 'OPRS')
    prefix 'Pres '
    width 10
    align 'L'
    update tick
}

Widget Rain {
    class 'Text'
    expression w1retap::readkey(file1, 'RAIN')
    prefix 'Rain '
    width 20
    align 'L'
    update tick
}

Layout Default {
    Timer1 'LCDTimer'
    Row1 {
        Col1 'Temp'
	Col11 'GHouse'
    }    

    Row2 {
	Col1 'House'
        Col11 'Garage'
    }    

    Row3 {
        Col1 'Soil'
	Col11 'Press'
    }    

    Row4 {
        Col1 'Rain'
    }
}

Variables {
   tick 120000
   file '/tmp/.w1retap.dat'
   file1 '/tmp/.w1rain.txt'
}

Layout 'Default'

Display 'Weather'

*/
