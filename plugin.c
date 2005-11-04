/* $Id: plugin.c,v 1.39 2005/11/04 04:53:10 reinelt Exp $
 *
 * plugin handler for the Evaluator
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
 * $Log: plugin.c,v $
 * Revision 1.39  2005/11/04 04:53:10  reinelt
 * sample plugin activated
 *
 * Revision 1.38  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.37  2005/05/02 10:29:20  reinelt
 * preparations for python bindings and python plugin
 *
 * Revision 1.36  2005/04/03 07:07:51  reinelt
 * added statfs plugin
 *
 * Revision 1.35  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.34  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.33  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.32  2004/06/07 06:56:55  reinelt
 *
 * added test plugin from Andy Baxter
 *
 * Revision 1.31  2004/05/29 00:27:23  reinelt
 *
 * added plugin_diskstats.c
 *
 * Revision 1.30  2004/05/22 18:30:02  reinelt
 *
 * added plugin 'uptime'
 *
 * Revision 1.29  2004/05/20 07:47:51  reinelt
 * added plugin_time
 *
 * Revision 1.28  2004/04/12 11:12:26  reinelt
 * added plugin_isdn, removed old ISDN client
 * fixed some real bad bugs in the evaluator
 *
 * Revision 1.27  2004/04/09 06:09:55  reinelt
 * big configure rework from Xavier
 *
 * Revision 1.25  2004/04/07 08:29:05  hejl
 * New plugin for wireless info
 *
 * Revision 1.24  2004/03/19 06:37:47  reinelt
 * asynchronous thread handling started
 *
 * Revision 1.23  2004/03/14 07:11:42  reinelt
 * parameter count fixed for plugin_dvb()
 * plugin_APM (battery status) ported
 *
 * Revision 1.22  2004/03/13 06:49:20  reinelt
 * seti@home plugin ported to NextGeneration
 *
 * Revision 1.21  2004/03/10 07:16:15  reinelt
 * MySQL plugin from Javier added
 *
 * Revision 1.20  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.19  2004/02/18 14:45:42  nicowallmeier
 * Imon/Telmon plugin ported
 *
 * Revision 1.18  2004/02/10 07:42:35  reinelt
 * cut off all old-style files which are no longer used with NextGeneration
 *
 * Revision 1.17  2004/02/10 06:54:39  reinelt
 * DVB plugin ported
 *
 * Revision 1.16  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.15  2004/01/27 08:13:39  reinelt
 * ported PPP token to plugin_ppp
 *
 * Revision 1.14  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
 * Revision 1.13  2004/01/16 05:04:53  reinelt
 * started plugin proc_stat which should parse /proc/stat
 * which again is a paint in the a**
 * thinking over implementation methods of delta functions
 * (CPU load, ...)
 *
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "plugin.h"


/* Prototypes */
int plugin_init_cfg(void);
void plugin_exit_cfg(void);
int plugin_init_math(void);
void plugin_exit_math(void);
int plugin_init_string(void);
void plugin_exit_string(void);
int plugin_init_test(void);
void plugin_exit_test(void);
int plugin_init_time(void);
void plugin_exit_time(void);

int plugin_init_apm(void);
void plugin_exit_apm(void);
int plugin_init_cpuinfo(void);
void plugin_exit_cpuinfo(void);
int plugin_init_diskstats(void);
void plugin_exit_diskstats(void);
int plugin_init_dvb(void);
void plugin_exit_dvb(void);
int plugin_init_exec(void);
void plugin_exit_exec(void);
int plugin_init_i2c_sensors(void);
void plugin_exit_i2c_sensors(void);
int plugin_init_imon(void);
void plugin_exit_imon(void);
int plugin_init_isdn(void);
void plugin_exit_isdn(void);
int plugin_init_loadavg(void);
void plugin_exit_loadavg(void);
int plugin_init_meminfo(void);
void plugin_exit_meminfo(void);
int plugin_init_mysql(void);
void plugin_exit_mysql(void);
int plugin_init_netdev(void);
void plugin_exit_netdev(void);
int plugin_init_pop3(void);
void plugin_exit_pop3(void);
int plugin_init_ppp(void);
void plugin_exit_ppp(void);
int plugin_init_proc_stat(void);
void plugin_exit_proc_stat(void);
int plugin_init_python(void);
void plugin_exit_python(void);
int plugin_init_sample(void);
void plugin_exit_sample(void);
int plugin_init_seti(void);
void plugin_exit_seti(void);
int plugin_init_statfs(void);
void plugin_exit_statfs(void);
int plugin_init_uname(void);
void plugin_exit_uname(void);
int plugin_init_uptime(void);
void plugin_exit_uptime(void);
int plugin_init_wireless(void);
void plugin_exit_wireless(void);
int plugin_init_xmms(void);
void plugin_exit_xmms(void);


int plugin_init(void)
{
    plugin_init_cfg();
    plugin_init_math();
    plugin_init_string();
    plugin_init_test();
    plugin_init_time();

#ifdef PLUGIN_APM
    plugin_init_apm();
#endif
#ifdef PLUGIN_CPUINFO
    plugin_init_cpuinfo();
#endif
#ifdef PLUGIN_DISKSTATS
    plugin_init_diskstats();
#endif
#ifdef PLUGIN_DVB
    plugin_init_dvb();
#endif
#ifdef PLUGIN_EXEC
    plugin_init_exec();
#endif
#ifdef PLUGIN_I2C_SENSORS
    plugin_init_i2c_sensors();
#endif
#ifdef PLUGIN_IMON
    plugin_init_imon();
#endif
#ifdef PLUGIN_ISDN
    plugin_init_isdn();
#endif
#ifdef PLUGIN_LOADAVG
    plugin_init_loadavg();
#endif
#ifdef PLUGIN_MEMINFO
    plugin_init_meminfo();
#endif
#ifdef PLUGIN_MYSQL
    plugin_init_mysql();
#endif
#ifdef PLUGIN_NETDEV
    plugin_init_netdev();
#endif
#ifdef PLUGIN_POP3
    plugin_init_pop3();
#endif
#ifdef PLUGIN_PPP
    plugin_init_ppp();
#endif
#ifdef PLUGIN_PROC_STAT
    plugin_init_proc_stat();
#endif
#ifdef PLUGIN_PYTHON
    plugin_init_python();
#endif
#ifdef PLUGIN_SAMPLE
    plugin_init_sample();
#endif
#ifdef PLUGIN_SETI
    plugin_init_seti();
#endif
#ifdef PLUGIN_STATFS
    plugin_init_statfs();
#endif
#ifdef PLUGIN_UNAME
    plugin_init_uname();
#endif
#ifdef PLUGIN_UPTIME
    plugin_init_uptime();
#endif
#ifdef PLUGIN_WIRELESS
    plugin_init_wireless();
#endif
#ifdef PLUGIN_XMMS
    plugin_init_xmms();
#endif

    return 0;
}


void plugin_exit(void)
{
#ifdef PLUGIN_APM
    plugin_exit_apm();
#endif
#ifdef PLUGIN_CPUINFO
    plugin_exit_cpuinfo();
#endif
#ifdef PLUGIN_DISKSTATS
    plugin_exit_diskstats();
#endif
#ifdef PLUGIN_DVB
    plugin_exit_dvb();
#endif
#ifdef PLUGIN_EXEC
    plugin_exit_exec();
#endif
#ifdef PLUGIN_I2C_SENSORS
    plugin_exit_i2c_sensors();
#endif
#ifdef PLUGIN_IMON
    plugin_exit_imon();
#endif
#ifdef PLUGIN_ISDN
    plugin_exit_isdn();
#endif
#ifdef PLUGIN_LOADAVG
    plugin_exit_loadavg();
#endif
#ifdef PLUGIN_MEMINFO
    plugin_exit_meminfo();
#endif
#ifdef PLUGIN_MYSQL
    plugin_exit_mysql();
#endif
#ifdef PLUGIN_NETDEV
    plugin_exit_netdev();
#endif
#ifdef PLUGIN_POP3
    plugin_exit_pop3();
#endif
#ifdef PLUGIN_PPP
    plugin_exit_ppp();
#endif
#ifdef PLUGIN_PROC_STAT
    plugin_exit_proc_stat();
#endif
#ifdef PLUGIN_PYTHON
    plugin_exit_python();
#endif
#ifdef PLUGIN_SAMPLE
    plugin_exit_sample();
#endif
#ifdef PLUGIN_SETI
    plugin_exit_seti();
#endif
#ifdef PLUGIN_STATFS
    plugin_exit_statfs();
#endif
#ifdef PLUGIN_UNAME
    plugin_exit_uname();
#endif
#ifdef PLUGIN_UPTIME
    plugin_exit_uptime();
#endif
#ifdef PLUGIN_WIRELESS
    plugin_exit_wireless();
#endif
#ifdef PLUGIN_XMMS
    plugin_exit_xmms();
#endif

    plugin_exit_cfg();
    plugin_exit_math();
    plugin_exit_string();
    plugin_exit_test();
    plugin_exit_time();

    DeleteFunctions();
    DeleteVariables();
}
