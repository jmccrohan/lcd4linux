/* $Id$
 * $URL$
 *
 * I2C sensors plugin
 *
 * Copyright (C) 2003, 2004 Xavier Vello <xavier66@free.fr>
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
 * int plugin_init_i2c_sensors (void)
 *  adds function i2c_sensors() to retrieve informations from
 *  the i2c sensors via sysfs or procfs interface
 *
 * -- WARNING --
 * This plugin should detect where your sensors are at startup.
 * If you can't get any token to work, ensure you don't get
 * an error message with "lcd4linux -Fvvv".
 *
 * If so, try to force the path to your sensors in the conf like this :
 * for sysfs:  i2c_sensors-path '/sys/bus/i2c/devices/0-6000/'
 * for procfs:  i2c_sensors-path '/proc/sys/dev/sensors/via686a-isa-6000'
 *     /!\ these path are for my system, change the last dir according to yours
 */

/*
 * Available tokens :  # represents an int from 1 to 3 (or more)
 *  temp_input# -> temperature of sensor # (in ∞C)
 *  temp_max# and temp_hyst# -> max and min of sensor #
 *  in_input#, in_min# and in_max# -> voltages
 *  fan_input# -> speed (in RPM) of fan #
 *  fan_min# and fan_div#
 *  
 * Tokens avaible only via sysfs if suported by your sensors:
 *  curr_input#, curr_min# and curr_max# -> value of current (in amps)
 *  pwm#
 *  temp_crit# -> critical value of sensor #
 *  vid -> cpu core voltage
 *     and maybe others
 *     (see /usr/src/linux/Documentation/i2c/sysfs-interface on linux 2.6)
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"
#include "hash.h"
#include "qprintf.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char *path = NULL;
static HASH I2Csensors;

static const char *procfs_tokens[4][3] = {
    {"temp_hyst", "temp_max", "temp_input"},	/* for temp# */
    {"in_min", "in_max", "in_input"},	/* for in# */
    {"fan_div1", "fan_div2", "fan_div3"},	/* for fan_div */
    {"fan_min", "fan_input", ""}	/* for fan# */
};

static int (*parse_i2c_sensors) (const char *key);

	/***********************************************\
	* Parsing for new 2.6 kernels 'sysfs' interface *
	\***********************************************/

static int parse_i2c_sensors_sysfs(const char *key)
{
    char val[32];
    char buffer[32];
    char file[64];
    FILE *stream;

    strcpy(file, path);
    strcat(file, key);

    stream = fopen(file, "r");
    if (stream == NULL) {
	error("i2c_sensors: fopen(%s) failed: %s", file, strerror(errno));
	return -1;
    }
    fgets(buffer, sizeof(buffer), stream);
    fclose(stream);

    if (buffer[0] == '\0') {
	error("i2c_sensors: %s empty ?!", file);
	return -1;
    }

    /* now the formating stuff, depending on the file : */
    /* Some values must be divided by 1000, the others */
    /* are parsed directly (we just remove the \n). */
    if (!strncmp(key, "temp", 4) || !strncmp(key, "curr", 4) || !strncmp(key, "in", 2) || !strncmp(key, "vid", 3)) {
	snprintf(val, sizeof(val), "%f", strtod(buffer, NULL) / 1000.0);
    } else {
	qprintf(val, sizeof(val), "%s", buffer);
	/* we supress this nasty \n at the end */
	val[strlen(val) - 1] = '\0';
    }

    hash_put(&I2Csensors, key, val);

    return 0;

}

	/************************************************\
	* Parsing for old 2.4 kernels 'procfs' interface *
	\************************************************/

static int parse_i2c_sensors_procfs(const char *key)
{
    char file[64];
    FILE *stream;
    char buffer[32];

    char *value;
    char *running;
    int pos = 0;
    const char delim[3] = " \n";
    char final_key[32];
    const char *number = &key[strlen(key) - 1];
    int tokens_index;
    /* debug("%s  ->  %s", key, number); */
    strcpy(file, path);

    if (!strncmp(key, "temp_", 5)) {
	tokens_index = 0;
	strcat(file, "temp");
	strcat(file, number);
    } else if (!strncmp(key, "in_", 3)) {
	tokens_index = 1;
	strcat(file, "in");
	strcat(file, number);
    } else if (!strncmp(key, "fan_div", 7)) {
	tokens_index = 2;
	strcat(file, "fan_div");
	number = "";
    } else if (!strncmp(key, "fan_", 4)) {
	tokens_index = 3;
	strcat(file, "fan");
	strcat(file, number);
    } else {
	return -1;
    }

    stream = fopen(file, "r");
    if (stream == NULL) {
	error("i2c_sensors: fopen(%s) failed: %s", file, strerror(errno));
	return -1;
    }
    fgets(buffer, sizeof(buffer), stream);
    fclose(stream);

    if (buffer[0] == '\0') {
	error("i2c_sensors: %s empty ?!", file);
	return -1;
    }

#ifndef __MAC_OS_X_VERSION_10_3
    running = strndup(buffer, sizeof(buffer));
#else
    // there is no strndup in OSX
    running = strdup(buffer);
#endif
    while (1) {
	value = strsep(&running, delim);
	/* debug("%s pos %i -> %s", file, pos , value); */
	if (!value || !strcmp(value, "")) {
	    /* debug("%s pos %i -> BREAK", file, pos); */
	    break;
	} else {
	    qprintf(final_key, sizeof(final_key), "%s%s", procfs_tokens[tokens_index][pos], number);
	    /* debug ("%s -> %s", final_key, value); */
	    hash_put(&I2Csensors, final_key, value);
	    pos++;
	}
    }
    free(running);
    return 0;
}

	/*****************************************\
	* Common functions (path search and init) *
	\*****************************************/


static void my_i2c_sensors_path(const char *method)
{
    struct dirent *dir;
    struct dirent *file;
    const char *base;
    char dname[64];
    DIR *fd1;
    DIR *fd2;
    int done;

    if (!strcmp(method, "sysfs")) {
	base = "/sys/bus/i2c/devices/";
    } else if (!strcmp(method, "procfs")) {
	base = "/proc/sys/dev/sensors/";
	/*base="/sensors_2.4/";             // fake dir to test without rebooting 2.4 ;) */
    } else {
	return;
    }

    fd1 = opendir(base);
    if (!fd1) {
	return;
    }

    while ((dir = readdir(fd1))) {
	/* Skip non-directories and '.' and '..' */
	if ((dir->d_type != DT_DIR && dir->d_type != DT_LNK) || strcmp(dir->d_name, ".") == 0
	    || strcmp(dir->d_name, "..") == 0) {
	    continue;
	}

	/* dname is the absolute path */
	strcpy(dname, base);
	strcat(dname, dir->d_name);
	strcat(dname, "/");

	fd2 = opendir(dname);
	done = 0;
	while ((file = readdir(fd2))) {
	    /* FIXME : do all sensors have a temp_input1 ? */
	    if (!strcmp(file->d_name, "temp_input1") || !strcmp(file->d_name, "temp1_input")
		|| !strcmp(file->d_name, "temp1")) {
		path = realloc(path, strlen(dname) + 1);
		strcpy(path, dname);
		done = 1;
		break;
	    }
	}
	closedir(fd2);
	if (done)
	    break;
    }
    closedir(fd1);
}


static int configure_i2c_sensors(void)
{
    static int configured = 0;
    char *path_cfg;

    if (configured != 0)
	return configured;

    path_cfg = cfg_get(NULL, "i2c_sensors-path", "");
    if (path_cfg == NULL || *path_cfg == '\0') {
	/* debug("No path to i2c sensors found in the conf, calling my_i2c_sensors_path()"); */
	my_i2c_sensors_path("sysfs");
	if (!path)
	    my_i2c_sensors_path("procfs");

	if (!path) {
	    error("i2c_sensors: unable to autodetect i2c sensors!");
	    configured = -1;
	    return configured;
	}

	debug("using i2c sensors at %s (autodetected)", path);

    } else {
	if (path_cfg[strlen(path_cfg) - 1] != '/') {
	    /* the headless user forgot the trailing slash :/ */
	    error("i2c_sensors: please add a trailing slash to %s from %s", path_cfg, cfg_source());
	    path_cfg = realloc(path_cfg, strlen(path_cfg) + 2);
	    strcat(path_cfg, "/");
	}
	debug("using i2c sensors at %s (from %s)", path_cfg, cfg_source());
	path = realloc(path, strlen(path_cfg) + 1);
	strcpy(path, path_cfg);
    }
    if (path_cfg)
	free(path_cfg);

    /* we activate the function only if there's a possibly path found */
    if (strncmp(path, "/sys", 4) == 0) {
	parse_i2c_sensors = parse_i2c_sensors_sysfs;
    } else if (strncmp(path, "/proc", 5) == 0) {
	parse_i2c_sensors = parse_i2c_sensors_procfs;
    } else {
	error("i2c_sensors: unknown path %s, should start with /sys or /proc", path);
	configured = -1;
	return configured;
    }

    hash_create(&I2Csensors);

    configured = 1;
    return configured;
}


void my_i2c_sensors(RESULT * result, RESULT * arg)
{
    int age;
    char *key;
    char *val;

    if (configure_i2c_sensors() < 0) {
	SetResult(&result, R_STRING, "??");
	return;
    }

    key = R2S(arg);
    age = hash_age(&I2Csensors, key);
    if (age < 0 || age > 250) {
	parse_i2c_sensors(key);
    }
    val = hash_get(&I2Csensors, key, NULL);
    if (val) {
	SetResult(&result, R_STRING, val);
    } else {
	SetResult(&result, R_STRING, "??");
    }
}


int plugin_init_i2c_sensors(void)
{
    AddFunction("i2c_sensors", 1, my_i2c_sensors);
    return 0;
}


void plugin_exit_i2c_sensors(void)
{
    hash_destroy(&I2Csensors);
}
