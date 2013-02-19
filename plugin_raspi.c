/* $Id$
 * $URL$
 *
 * plugin raspi
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2013 Volker Gerng <v.gering@t-online.de>
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
 * int plugin_init_raspi (void)
 *  adds functions to get information about internal sensors of raspberry pi
 *
 */


#include "config.h"

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


#define RASPI_FREQ_PATH   "/sys/devices/system/cpu/cpu0/cpufreq/"
#define RASPI_FREQ_VALUE  "cpuinfo_cur_freq"
#define RASPI_FREQ_IDFILE "scaling_driver"
#define RASPI_TEMP_PATH   "/sys/class/thermal/thermal_zone0/"
#define RASPI_TEMP_VALUE  "temp"
#define RASPI_TEMP_IDFILE "type"
#define RASPI_TEMP_ID     "bcm2835_thermal"

#define _cat(a,b)     (a##b)
#define strings(a, b) "_cat(a,b)"


char tmpstr[128];


/* Note: all local functions should be declared 'static' */

/* reading an positive integer value from path, -1 on error */
static int readValue(char *path) {
    int value = -1;
    FILE *fp;

    fp = fopen(path, "r");
    if (NULL != fp) {
        fgets(tmpstr, sizeof(tmpstr), fp);
        fclose(fp);
	if (1 != sscanf(tmpstr, "%i", &value)) {
            error("[raspi] error reading integer value from %s\n", path);
        }
    } else {
        error("[raspi] error opening %s: %s\n", path, strerror(errno));
    }

    return value;
}


/* reads a string from path */
static char* readStr(char *path) {
    FILE *fp;

    memset(tmpstr, 0, sizeof(tmpstr));
    fp = fopen(path, "r");
    if (NULL != fp) {
        fgets(tmpstr, sizeof(tmpstr), fp);
        fclose(fp);
    } else {
        error("[raspi] error reading text value from %s: %s\n", path, strerror(errno));
    }

    return tmpstr;
}


/* reads the actual cpu frequency of the bcm2708 cpu */
static void my_cpufreq(RESULT * result)
{
    snprintf(tmpstr, sizeof(tmpstr), "%s%s", RASPI_FREQ_PATH, RASPI_FREQ_VALUE);
    double value = readValue(tmpstr) / 1000.0L;

    info("[raspi] actual cpu frequency: %.2f MHz", value);
    SetResult(&result, R_NUMBER, &value);
}


/* reads the actual cpu temperature in degree Celsius */
static void my_cputemp(RESULT * result)
{
    snprintf(tmpstr, sizeof(tmpstr), "%s%s", RASPI_TEMP_PATH, RASPI_TEMP_VALUE);
    double value = readValue(tmpstr) / 1000.0L;

    info("[raspi] actual cpu temperature: %.1f C", value);
    SetResult(&result, R_NUMBER, &value);
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_raspi(void)
{
    char checkFile[128];

    AddFunction("raspi::cpufreq", 0, my_cpufreq);
    AddFunction("raspi::cputemp", 0, my_cputemp);

    snprintf(checkFile, sizeof(checkFile), "%s%s", RASPI_TEMP_PATH, RASPI_TEMP_IDFILE);
    if (strncmp( readStr(checkFile), RASPI_TEMP_ID, strlen(RASPI_TEMP_ID) ) != 0) {
        error("Warning: no raspberry pi thermal sensor found: value of '%s' is '%s', should be '%s'",
              checkFile, readStr(checkFile), RASPI_TEMP_IDFILE);
    }

    return 0;
}

void plugin_exit_raspi(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}
