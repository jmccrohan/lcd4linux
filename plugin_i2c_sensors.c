/* $Id: plugin_i2c_sensors.c,v 1.18 2004/06/17 06:23:43 reinelt Exp $
 *
 * I2C sensors plugin
 *
 * Copyright 2003,2004 Xavier Vello <xavier66@free.fr>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net> 
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
 * $Log: plugin_i2c_sensors.c,v $
 * Revision 1.18  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.17  2004/06/05 14:56:48  reinelt
 *
 * Cwlinux splash screen fixed
 * USBLCD splash screen fixed
 * plugin_i2c qprintf("%f") replaced with snprintf()
 *
 * Revision 1.16  2004/06/01 06:45:30  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.15  2004/05/31 21:05:13  reinelt
 *
 * fixed lots of bugs in the Cwlinux driver
 * do not emit EAGAIN error on the first retry
 * made plugin_i2c_sensors a bit less 'chatty'
 * moved init and exit functions to the bottom of plugin_pop3
 *
 * Revision 1.14  2004/05/09 05:41:42  reinelt
 *
 * i2c fix for kernel 2.6.5 (temp_input1 vs. temp1_input) from Xavier
 *
 * Revision 1.13  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.12  2004/02/16 08:19:44  reinelt
 * i2c_sensors patch from Xavier
 *
 * Revision 1.11  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.10  2004/02/14 12:07:27  nicowallmeier
 * minor bugfix
 *
 * Revision 1.9  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.8  2004/02/14 10:09:50  reinelt
 * I2C Sensors for 2.4 kernels (/proc instead of /sysfs)
 *
 * Revision 1.7  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.6  2004/01/30 07:12:35  reinelt
 * HD44780 busy-flag support from Martin Hejl
 * loadavg() uClibc replacement from Martin Heyl
 * round() uClibc replacement from Martin Hejl
 * warning in i2c_sensors fixed
 *
 * Revision 1.5  2004/01/29 05:55:30  reinelt
 * check for /sys mounted
 *
 * Revision 1.4  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.3  2004/01/27 08:13:39  reinelt
 * ported PPP token to plugin_ppp
 *
 * Revision 1.2  2004/01/27 05:06:10  reinelt
 * i2c update from Xavier
 *
 * Revision 1.1  2004/01/10 17:36:56  reinelt
 *
 * I2C Sensors plugin from Xavier added
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
 *  temp_input# -> temperature of sensor # (in °C)
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

static char *path=NULL;
static HASH I2Csensors;

static const char *procfs_tokens[4][3] = {
  {"temp_hyst", "temp_max", "temp_input"},	// for temp#
  {"in_min", "in_max", "in_input"},		// for in#
  {"fan_div1", "fan_div2", "fan_div3"},		// for fan_div
  {"fan_min", "fan_input", ""}			// for fan#
};

static int (*parse_i2c_sensors)(char *key);

	/***********************************************\
	* Parsing for new 2.6 kernels 'sysfs' interface *
	\***********************************************/

static int parse_i2c_sensors_sysfs(char *key)
{
  char val[32];
  char buffer[32];
  char file[64];
  FILE *stream;

  strcpy(file, path);
  strcat(file, key);

  stream=fopen(file, "r");
  if (stream==NULL) {
    error ("i2c_sensors: fopen(%s) failed: %s", file, strerror(errno));
    return -1;
  }
  fgets (buffer, sizeof(buffer), stream);
  fclose (stream);
  
  if (!buffer) {
    error ("i2c_sensors: %s empty ?!", file);	  
    return -1;
  }

  // now the formating stuff, depending on the file :
  // Some values must be divided by 1000, the others
  // are parsed directly (we just remove the \n).
  if (!strncmp(key, "temp", 4)  ||
      !strncmp(key, "curr", 4)  ||
      !strncmp(key, "in", 2)    ||
      !strncmp(key, "vid", 3)) {
    snprintf(val, sizeof(val), "%f", strtod(buffer, NULL) / 1000.0);   
  } else {
    qprintf(val, sizeof(val), "%s", buffer); 
    // we supress this nasty \n at the end
    val[strlen(val)-1]='\0';
  } 
 
  hash_put (&I2Csensors, key, val);

  return 0; 

}

	/************************************************\
	* Parsing for old 2.4 kernels 'procfs' interface *
	\************************************************/

static int parse_i2c_sensors_procfs(char *key)
{
  char file[64];
  FILE *stream;
  char buffer[32];

  char *value;
  char *running;
  int pos=0;
  const char delim[3]=" \n";
  char final_key[32];
  char *number = &key[strlen(key)-1];
  int tokens_index;
  // debug("%s  ->  %s", key, number);
  strcpy(file, path);

  if (!strncmp(key, "temp_", 5)) {
    tokens_index=0;
    strcat(file, "temp");
    strcat(file, number);
  } else if (!strncmp(key, "in_", 3)) {
    tokens_index=1;
    strcat(file, "in");
    strcat(file, number);   
  } else if (!strncmp(key, "fan_div", 7)) {
    tokens_index=2;
    strcat(file, "fan_div");
    number = "";
  } else if (!strncmp(key, "fan_", 4)) {
    tokens_index=3;
    strcat(file, "fan");
    strcat(file, number);     
  } else {
    return -1;
  }

  stream=fopen(file, "r");
  if (stream==NULL) {
    error ("i2c_sensors: fopen(%s) failed: %s", file, strerror(errno));
    return -1;
  }
  fgets (buffer, sizeof(buffer), stream);
  fclose (stream);
  
  if (!buffer) {
    error ("i2c_sensors: %s empty ?!",file);	  
    return -1;
  }

  running=strdupa(buffer);
  while(1) {
    value = strsep (&running, delim);
    // debug("%s pos %i -> %s", file, pos , value);
    if (!value || !strcmp(value, "")) {
      // debug("%s pos %i -> BREAK", file, pos);
      break;
    } else {
      qprintf (final_key, sizeof(final_key), "%s%s", procfs_tokens[tokens_index][pos], number);
      // debug ("%s -> %s", final_key, value);
      hash_put (&I2Csensors, final_key, value);
      pos++;
    }
  }
  return 0;
}  

	/*****************************************\
	* Common functions (path search and init) *
	\*****************************************/

void my_i2c_sensors(RESULT *result, RESULT *arg)
{
  int age;
  char *val;
  char *key=R2S(arg);  
  
  age=hash_age(&I2Csensors, key);
  if (age<0 || age>250) {
    parse_i2c_sensors(key);
  }
  val=hash_get(&I2Csensors, key, NULL);
  if (val) {
    SetResult(&result, R_STRING, val); 
  } else {
    SetResult(&result, R_STRING, "??"); 
  }
}


void my_i2c_sensors_path(char *method)
{
  struct dirent *dir;
  struct dirent *file;
  const char *base;
  char dname[64];
  DIR *fd1;
  DIR *fd2;
  int done;
	  
  if (!strcmp(method, "sysfs")) {
    base="/sys/bus/i2c/devices/";
  } else if (!strcmp(method, "procfs")) {
    base="/proc/sys/dev/sensors/";
    //base="/sensors_2.4/";		// fake dir to test without rebooting 2.4 ;)
  } else {
    return; 
  }
  
  fd1 = opendir(base);
  if (!fd1) {
    return;
  }
  
  while((dir = readdir(fd1)))   {
    // Skip non-directories and '.' and '..'
    if ((dir->d_type!=DT_DIR && dir->d_type!=DT_LNK) ||
       strcmp(dir->d_name, "." )==0 ||
       strcmp(dir->d_name, "..")==0) {
      continue;
    }

    // dname is the absolute path
    strcpy(dname, base);		
    strcat(dname, dir->d_name);
    strcat(dname, "/"); 
    
    fd2 = opendir(dname);
    done = 0;
    while((file = readdir(fd2))) {
      // FIXME : do all sensors have a temp_input1 ?
      if (!strcmp(file->d_name, "temp_input1") || !strcmp(file->d_name, "temp1_input") || !strcmp(file->d_name, "temp1")) {
	path = realloc(path, strlen(dname)+1);
	strcpy(path, dname);			  
	done=1;
	break;
      }
    }
    closedir(fd2);
    if (done) break;
  }
  closedir(fd1);
}


int plugin_init_i2c_sensors (void)
{
  char *path_cfg;

  hash_create(&I2Csensors);

  path_cfg = cfg_get(NULL, "i2c_sensors-path", "");
  if (path_cfg == NULL || *path_cfg == '\0') {
    // debug("No path to i2c sensors found in the conf, calling my_i2c_sensors_path()");
    my_i2c_sensors_path("sysfs");
    if (!path)
      my_i2c_sensors_path("procfs");
    
    if (!path) {
      error("i2c_sensors: unable to autodetect i2c sensors!");
    } else {
      debug("using i2c sensors at %s (autodetected)", path);
    }
  } else {
    if (path_cfg[strlen(path_cfg)-1] != '/') {
      // the headless user forgot the trailing slash :/
      error("i2c_sensors: please add a trailing slash to %s from %s", path_cfg, cfg_source());
      path_cfg = realloc(path_cfg, strlen(path_cfg)+2);
      strcat(path_cfg, "/");
    }
    debug("using i2c sensors at %s (from %s)", path, cfg_source());
    path = realloc(path, strlen(path_cfg)+1);
    strcpy(path, path_cfg);
  }
  if (path_cfg) free(path_cfg);
  
  // we activate the function only if there's a possibly path found
  if (path!=NULL) {
    if (strncmp(path, "/sys", 4)==0) {
      parse_i2c_sensors = parse_i2c_sensors_sysfs;
      AddFunction ("i2c_sensors", 1, my_i2c_sensors);
    } else if (strncmp(path, "/proc", 5)==0) {
      parse_i2c_sensors = parse_i2c_sensors_procfs;      
      AddFunction ("i2c_sensors", 1, my_i2c_sensors);
    } else {
      error("i2c_sensors: unknown path %s, should start with /sys or /proc");
    }
  }

  hash_create(&I2Csensors);
  
  return 0;
}

void plugin_exit_i2c_sensors(void) 
{
  hash_destroy(&I2Csensors);
}
