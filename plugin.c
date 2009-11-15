/* $Id$
 * $URL$
 *
 * plugin handler for the Evaluator
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
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


char *Plugins[] = {
    "cfg",
    "math",
    "string",
    "test",
    "time",
#ifdef PLUGIN_APM
    "apm",
#endif
#ifdef PLUGIN_ASTERISK
    "asterisk",
#endif
#ifdef PLUGIN_BUTTON_EXEC
    "button_exec",
#endif
#ifdef PLUGIN_CPUINFO
    "cpuinfo",
#endif
#ifdef PLUGIN_DBUS
    "dbus",
#endif
#ifdef PLUGIN_DISKSTATS
    "diskstats",
#endif
#ifdef PLUGIN_DVB
    "dvb",
#endif
#ifdef PLUGIN_EXEC
    "exec",
#endif
#ifdef PLUGIN_EVENT
    "event",
#endif
#ifdef PLUGIN_FIFO
    "fifo",
#endif
#ifdef PLUGIN_FILE
    "file",
#endif
#ifdef PLUGIN_GPS
    "gps",
#endif
#ifdef PLUGIN_HDDTEMP
    "hddtemp",
#endif
#ifdef PLUGIN_I2C_SENSORS
    "i2c_sensors",
#endif
#ifdef PLUGIN_ICONV
    "iconv",
#endif
#ifdef PLUGIN_IMON
    "imon",
#endif
#ifdef PLUGIN_ISDN
    "isdn",
#endif
#ifdef PLUGIN_KVV
    "kvv",
#endif
#ifdef PLUGIN_LOADAVG
    "loadavg",
#endif
#ifdef PLUGIN_MEMINFO
    "meminfo",
#endif
#ifdef PLUGIN_MPD
    "mpd",
#endif
#ifdef PLUGIN_MPRIS_DBUS
    "mpris_dbus",
#endif
#ifdef PLUGIN_MYSQL
    "mysql",
#endif
#ifdef PLUGIN_NETDEV
    "netdev",
#endif
#ifdef PLUGIN_NETINFO
    "netinfo",
#endif
#ifdef PLUGIN_POP3
    "pop3",
#endif
#ifdef PLUGIN_PPP
    "ppp",
#endif
#ifdef PLUGIN_PROC_STAT
    "proc_stat",
#endif
#ifdef PLUGIN_PYTHON
    "python",
#endif
#ifdef PLUGIN_SAMPLE
    "sample",
#endif
#ifdef PLUGIN_SETI
    "seti",
#endif
#ifdef PLUGIN_STATFS
    "statfs",
#endif
#ifdef PLUGIN_UNAME
    "uname",
#endif
#ifdef PLUGIN_UPTIME
    "uptime",
#endif
#ifdef PLUGIN_W1RETAP
    "w1retap",
#endif
#ifdef PLUGIN_WIRELESS
    "wireless",
#endif
#ifdef PLUGIN_XMMS
    "xmms",
#endif
    NULL,
};


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
int plugin_init_asterisk(void);
void plugin_exit_asterisk(void);
int plugin_init_button_exec(void);
void plugin_exit_button_exec(void);
int plugin_init_cpuinfo(void);
void plugin_exit_cpuinfo(void);
int plugin_init_dbus(void);
void plugin_exit_dbus(void);
int plugin_init_diskstats(void);
void plugin_exit_diskstats(void);
int plugin_init_dvb(void);
void plugin_exit_dvb(void);
int plugin_init_exec(void);
void plugin_exit_exec(void);
int plugin_init_event(void);
void plugin_exit_event(void);
int plugin_init_fifo(void);
void plugin_exit_fifo(void);
int plugin_init_file(void);
void plugin_exit_file(void);
int plugin_init_gps(void);
void plugin_exit_gps(void);
int plugin_init_hddtemp(void);
void plugin_exit_hddtemp(void);
int plugin_init_i2c_sensors(void);
void plugin_exit_i2c_sensors(void);
int plugin_init_imon(void);
void plugin_exit_imon(void);
int plugin_init_iconv(void);
void plugin_exit_iconv(void);
int plugin_init_isdn(void);
void plugin_exit_isdn(void);
int plugin_init_kvv(void);
void plugin_exit_kvv(void);
int plugin_init_loadavg(void);
void plugin_exit_loadavg(void);
int plugin_init_meminfo(void);
void plugin_exit_meminfo(void);
int plugin_init_mpd(void);
void plugin_exit_mpd(void);
int plugin_init_mysql(void);
void plugin_exit_mysql(void);
int plugin_init_netdev(void);
void plugin_exit_netdev(void);
int plugin_init_netinfo(void);
void plugin_exit_netinfo(void);
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
int plugin_init_w1retap(void);
void plugin_exit_w1retap(void);
int plugin_init_wireless(void);
void plugin_exit_wireless(void);
int plugin_init_xmms(void);
void plugin_exit_xmms(void);


int plugin_list(void)
{
    int i;

    printf("available plugins:\n  ");

    for (i = 0; Plugins[i]; i++) {
	printf("%s", Plugins[i]);
	if (Plugins[i + 1])
	    printf(", ");
    }
    printf("\n");
    return 0;
}


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
#ifdef PLUGIN_ASTERISK
    plugin_init_asterisk();
#endif
#ifdef PLUGIN_BUTTON_EXEC
    plugin_init_button_exec();
#endif
#ifdef PLUGIN_CPUINFO
    plugin_init_cpuinfo();
#endif
#ifdef PLUGIN_DBUS
    plugin_init_dbus();
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
#ifdef PLUGIN_EVENT
    plugin_init_event();
#endif
#ifdef PLUGIN_FIFO
    plugin_init_fifo();
#endif
#ifdef PLUGIN_FILE
    plugin_init_file();
#endif
#ifdef PLUGIN_GPS
    plugin_init_gps();
#endif
#ifdef PLUGIN_HDDTEMP
    plugin_init_hddtemp();
#endif
#ifdef PLUGIN_I2C_SENSORS
    plugin_init_i2c_sensors();
#endif
#ifdef PLUGIN_ICONV
    plugin_init_iconv();
#endif
#ifdef PLUGIN_IMON
    plugin_init_imon();
#endif
#ifdef PLUGIN_ISDN
    plugin_init_isdn();
#endif
#ifdef PLUGIN_KVV
    plugin_init_kvv();
#endif
#ifdef PLUGIN_LOADAVG
    plugin_init_loadavg();
#endif
#ifdef PLUGIN_MEMINFO
    plugin_init_meminfo();
#endif
#ifdef PLUGIN_MPD
    plugin_init_mpd();
#endif
#ifdef PLUGIN_MPRIS_DBUS
    plugin_init_mpris_dbus();
#endif
#ifdef PLUGIN_MYSQL
    plugin_init_mysql();
#endif
#ifdef PLUGIN_NETDEV
    plugin_init_netdev();
#endif
#ifdef PLUGIN_NETINFO
    plugin_init_netinfo();
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
#ifdef PLUGIN_W1RETAP
    plugin_init_w1retap();
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
#ifdef PLUGIN_ASTERISK
    plugin_exit_asterisk();
#endif
#ifdef PLUGIN_BUTTON_EXEC
    plugin_exit_button_exec();
#endif
#ifdef PLUGIN_CPUINFO
    plugin_exit_cpuinfo();
#endif
#ifdef PLUGIN_DBUS
    plugin_exit_dbus();
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
#ifdef PLUGIN_EVENT
    plugin_exit_event();
#endif
#ifdef PLUGIN_FIFO
    plugin_exit_fifo();
#endif
#ifdef PLUGIN_FILE
    plugin_exit_file();
#endif
#ifdef PLUGIN_GPS
    plugin_exit_gps();
#endif
#ifdef PLUGIN_I2C_SENSORS
    plugin_exit_i2c_sensors();
#endif
#ifdef PLUGIN_ICONV
    plugin_exit_iconv();
#endif
#ifdef PLUGIN_IMON
    plugin_exit_imon();
#endif
#ifdef PLUGIN_ISDN
    plugin_exit_isdn();
#endif
#ifdef PLUGIN_KVV
    plugin_exit_kvv();
#endif
#ifdef PLUGIN_LOADAVG
    plugin_exit_loadavg();
#endif
#ifdef PLUGIN_MEMINFO
    plugin_exit_meminfo();
#endif
#ifdef PLUGIN_MPD
    plugin_exit_mpd();
#endif
#ifdef PLUGIN_MPRIS_DBUS
    plugin_exit_mpris_dbus();
#endif
#ifdef PLUGIN_MYSQL
    plugin_exit_mysql();
#endif
#ifdef PLUGIN_NETDEV
    plugin_exit_netdev();
#endif
#ifdef PLUGIN_NETINFO
    plugin_exit_netinfo();
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
#ifdef PLUGIN_W1RETAP
    plugin_exit_w1retap();
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
