/* $Id: plugin.c,v 1.12 2004/01/15 04:29:45 reinelt Exp $
 *
 * plugin handler for the Evaluator
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin.c,v $
 * Revision 1.12  2004/01/15 04:29:45  reinelt
 * moved lcd4linux.conf.sample to *.old
 * lcd4linux.conf.sample with new layout
 * new plugins 'loadavg' and 'meminfo'
 * text widget have pre- and postfix
 *
 * Revision 1.11  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.10  2004/01/13 10:03:01  reinelt
 * new util 'hash' for associative arrays
 * new plugin 'cpuinfo'
 *
 * Revision 1.9  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.8  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.7  2004/01/10 17:45:26  reinelt
 * changed initialization order so cfg() gets initialized before plugins.
 * This way a plugin's init() can use cfg_get().
 * Thanks to Xavier for reporting this one!
 *
 * Revision 1.6  2004/01/10 17:36:56  reinelt
 *
 * I2C Sensors plugin from Xavier added
 *
 * Revision 1.5  2004/01/06 17:33:45  reinelt
 *
 * Evaluator: functions with variable argument lists
 * Evaluator: plugin_sample.c and README.Plugins added
 *
 * Revision 1.4  2004/01/06 15:19:16  reinelt
 * Evaluator rearrangements...
 *
 * Revision 1.3  2003/12/19 06:27:33  reinelt
 * added XMMS plugin from Markus Keil
 *
 * Revision 1.2  2003/12/19 05:49:23  reinelt
 * extracted plugin_math and plugin_string into extra files
 *
 * Revision 1.1  2003/12/19 05:35:14  reinelt
 * renamed 'client' to 'plugin'
 *
 * Revision 1.1  2003/10/11 06:01:52  reinelt
 *
 * renamed expression.{c,h} to client.{c,h}
 * added config file client
 * new functions 'AddNumericVariable()' and 'AddStringVariable()'
 * new parameter '-i' for interactive mode
 *
 * Revision 1.3  2003/10/06 05:51:15  reinelt
 * functions: min(), max()
 *
 * Revision 1.2  2003/10/06 05:47:27  reinelt
 * operators: ==, \!=, <=, >=
 *
 * Revision 1.1  2003/10/06 04:34:06  reinelt
 * expression evaluator added
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init (void)
 *  initializes the expression evaluator
 *  adds some handy constants and functions
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "plugin.h"


// Prototypes
int plugin_init_math (void);
int plugin_init_string (void);
int plugin_init_cfg (void);
int plugin_init_uname (void);
int plugin_init_loadavg (void);
int plugin_init_cpuinfo (void);
int plugin_init_meminfo (void);
int plugin_init_i2c_sensors (void);
int plugin_init_xmms (void);


int plugin_init (void)
{
  plugin_init_math();
  plugin_init_string();
  plugin_init_cfg();
  plugin_init_uname();
  plugin_init_loadavg();
  plugin_init_cpuinfo();
  plugin_init_meminfo();
  // MR: segfaults here
  // plugin_init_i2c_sensors();
  plugin_init_xmms();
  
  return 0;
}


