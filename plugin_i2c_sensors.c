/* $Id: plugin_i2c_sensors.c,v 1.9 2004/02/14 11:56:17 reinelt Exp $
 *
 * I2C sensors plugin
 *
 * Copyright 2003,2004 Xavier Vello <xavier66@free.fr>
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
 * [
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
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_i2c_sensors (void)
 *  adds function i2c_sensors() to retrieve informations from
 *  the i2c sensors via the sysfs interface of 2.6.x kernels
 *
 * -- WARNING --
 * This plugin is only for kernel series 2.5/2.6 and higher !
 * It uses the new sysfs pseudo-filesystem that you can mount with:
 *       mount -t sysfs none /sys
 *
 * -- WARNING #2 --
 * This plugin should detect where your sensors are at startup.
 * If you can't get any token to work, ensure you don't get
 * an error message with "lcd4linux -Fvvv".
 *
 * If so, try to force the path to your sensors in the conf like this :
 *   i2c_sensors-path '/sys/bus/i2c/devices/0-6000/'
 *   (replace 0-6000 with the appropriate dir)
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"
#include "hash.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char *path=NULL;
static HASH I2Csensors = { 0, };

	/***********************************************\
	* Parsing for new 2.6 kernels 'sysfs' interface *
	\***********************************************/
static int parse_i2c_sensors_sysfs(RESULT *arg)
{
  double value;
  char val[32];
  char buffer[32];
  char *key=R2S(arg);
  char file[64];
  FILE *stream;

  // construct absolute path to the file to read
  strcpy(file, path);
  strcat(file, key);

  // read of file to buffer
  stream=fopen(file, "r");
  if (stream==NULL) {
    error ("fopen(%s) failed",file);
    return -1;
  }
  fgets (buffer, sizeof(buffer), stream);
  fclose (stream);
  
  if (!buffer) {
    error ("%s empty ?!",file);	  
    return -1;
  }

  // now the formating stuff, depending on the file :
  // Some values must be divided by 1000, the others
  // are parsed directly (we just remove the \n).
  if (!strncmp(key, "temp_", 5)  ||
      !strncmp(key, "curr_", 5)  ||
      !strncmp(key, "in_", 3)    ||
      !strncmp(key, "vid", 3)) {
    value = strtod(buffer, NULL);
    // FIXME: any way to do this without converting to double ?		  
    value /= 1000.0;
    sprintf(val, "%f", value);   
		  
  } else {
    sprintf(val, "%s", buffer); 
    // we supress this nasty \n at the end
    val[strlen(val)-1]='\0';
  } 
  
  hash_set (&I2Csensors, key, val);
  return 0; 

}  

void my_i2c_sensors_sysfs(RESULT *result, RESULT *arg)
{
  int age;
  char *val;
  char *key=R2S(arg);  
  
  age=hash_age(&I2Csensors, key, &val);
  // refresh every 100msec
  if (age<0 || age>100) {
    parse_i2c_sensors_sysfs(arg);
    val=hash_get(&I2Csensors, key);
  }
  if (val) {
    SetResult(&result, R_STRING, val); 
  } else {
    SetResult(&result, R_STRING, "??"); 
  }
}


	/************************************************\
	* Parsing for old 2.4 kernels 'procfs' interface *
	\************************************************/

static int parse_i2c_sensors_procfs(RESULT *arg)
{
  char *key=R2S(arg);
  char file[64];    
  FILE *stream;
  char buffer[32];

  char *value;
  char *running;
  int pos=0;
  const char delim[3]= " \n";
  char final_key[32];

  strcpy(file, path);
  strcat(file, key);

  stream=fopen(file, "r");
  if (stream==NULL) {
    error ("fopen(%s) failed",file);
    return -1;
  }
  fgets (buffer, sizeof(buffer), stream);
  fclose (stream);
  
  if (!buffer) {
    error ("%s empty ?!",file);	  
    return -1;
  }
  
  running=strdupa(buffer);
  while(1) {
    value = strsep (&running, delim);
    //    debug("%s pos %i -> %s", key, pos , value);
    if (!value) {
      break;
    } else {
      sprintf (final_key, "%s.%i", key, pos);
      hash_set (&I2Csensors, final_key, value);
      pos++;
    }
  }
  return 0;
}  

void my_i2c_sensors_procfs(RESULT *result, int argc, RESULT *argv[])
{
  int age;
  char *val;
  char *key;  
  
  switch (argc) {
  case 1:
    sprintf(key, "%s.0", R2S(argv[0]));
    break;
  case 2:
    sprintf(key, "%s.%s", R2S(argv[0]), R2S(argv[1]));
    break;
  default:
    return;  
    break;
  }

  age=hash_age(&I2Csensors, key, &val);

  // refresh every 100msec
  if (age<0 || age>100) {
    parse_i2c_sensors_procfs(argv[0]);
    val=hash_get(&I2Csensors, key);
  }
  if (val) {
    SetResult(&result, R_STRING, val); 
  } else {
    SetResult(&result, R_STRING, "??"); 
  }
}

	/*****************************************\
	* Common functions (path search and init) *
	\*****************************************/

void my_i2c_sensors_path(char *method)
{
  struct dirent *dir;
  struct dirent *file;
  const char *base;
	  
  if (!strcmp(method, "sysfs")) {
    base="/sys/bus/i2c/devices/";
  } else if (!strcmp(method, "procfs")) {
    base="/proc/sys/dev/sensors/";
    //base="/sensors_2.4/";			// fake dir to test without rebooting 2.4 ;)
  } else {
    return; 
  }

  char dname[64];
  DIR *fd1;
  DIR *fd2;
  int done;
  
  fd1 = opendir(base);
  if (!fd1) {
    if (!strcmp(method, "sysfs")) {
      error("[i2c_sensors] Impossible to open %s! Is /sys mounted?", base);
    } else if (!strcmp(method, "procfs")) {
      error("[i2c_sensors] Impossible to open %s! Is i2c_proc loaded ?", base);
    }
    return;
  }
  
  while((dir = readdir(fd1)))   {
    // Skip non-directories and '.' and '..'
    if (dir->d_type!=DT_DIR || 
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
      if (!strcmp(file->d_name, "temp_input1") || !strcmp(file->d_name, "temp1")) { // FIXME : do all sensors have a temp_input1 ?
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
  char *path_cfg=cfg_get(NULL, "i2c_sensors-path", "");

  if (strncmp(path_cfg, "/", 1)) {
    debug("No path to i2c sensors found in the conf, calling my_i2c_sensors_path()");
    my_i2c_sensors_path("sysfs");
    if (!path)
      my_i2c_sensors_path("procfs");

    if (!path) {
      error("[i2c_sensors] No i2c sensors found via the i2c interface !");
      error("[i2c_sensors] Try to specify the path to the sensors !");
    } else {
      debug("Your i2c sensors are probably in %s", path);
      debug("if i2c_sensors doesn't work, try to specify the path in your conf");
    }
	
  } else {
    if (path_cfg[strlen(path_cfg)-1] != '/') {
      // the headless user forgot the trailing slash :/
      debug("adding a trailing slash at the end of the path");
      path_cfg = realloc(path_cfg, strlen(path_cfg)+2);
      strcat(path_cfg, "/");
    }
    debug("Path to i2c sensors from the conf : %s", path_cfg);
    debug("if i2c_sensors doesn't work, double check this value !");
    path = realloc(path, strlen(path_cfg)+1);
    strcpy(path, path_cfg);
    free(path_cfg);
  }

  // we activate the function only if there's a possibly path found
  if (!path) {
    free(path);
  } else {
    if (!strncmp(path, "/sys", 4)) {
      AddFunction ("i2c_sensors", 1, my_i2c_sensors_sysfs);
    } else if (!strncmp(path, "/proc", 5)) {
      AddFunction ("i2c_sensors", -1, my_i2c_sensors_procfs);
    }
  }
 
  return 0;
}
