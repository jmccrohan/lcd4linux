/* $Id: plugin_i2c_sensors.c,v 1.1 2004/01/10 17:36:56 reinelt Exp $
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
 * an error message with "lcd4linux -F".
 *
 * If so, try to force the path to your sensors in the conf like this :
 *   i2c_sensors.path /sys/bus/i2c/devices/0-6000/
 *
 *   - replace 0-6000 with the appropriate dir
 *   - DON'T forget the trailing slash or it won't work !
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"

static char *path=NULL;

static void my_i2c_sensors (RESULT *result, RESULT *arg)
{
  int fd=-2;
  double value;
  char buffer[32];
  char *key=R2S(arg);
  char *file;

  // construct absolute path to the file to read
  // Fixme: MR: free the path again??
  file = strdup(path);
  file = realloc(file, strlen(path)+strlen(key)+1);
  file = strcat(file, key);
  
  // read of file to buffer
  fd = open(file, O_RDONLY);
  read (fd, &buffer, 31);
  close(fd);

  if (!buffer) {
    SetResult(&result, R_STRING, "??"); 
    return;
  }
  
  // now the formating stuff, depending on the file wanted
  if (!strncmp(key, "temp_", 5)  ||
      !strncmp(key, "curr_", 5)  ||
      !strncmp(key, "in_", 3)    ||
      !strncmp(key, "vid", 3)) {
    value = strtod(buffer, NULL);
    value /= 1000.0;
    if (value) {
      SetResult(&result, R_NUMBER, &value); 
      return;
    }    

  } else if (!strncmp(key, "fan_", 4) ||
	     !strncmp(key, "pwn_", 4) ||
	     !strncmp(key, "vrm", 5)) {
    value = strtod(buffer, NULL);
    if (value) {
      SetResult(&result, R_NUMBER, &buffer);
      return;
    } 
 
  } else {
    SetResult(&result, R_STRING, &buffer);
    return; 
  } 
  
  // fallback is there's a problem  
  SetResult(&result, R_STRING, "??");
}


void my_i2c_sensors_path(void)
{
  struct dirent *dir;
  struct dirent *file;
  char *base="/sys/bus/i2c/devices/";
  char dname[64];
  DIR *fd1;
  DIR *fd2;
  int done;
  
  fd1 = opendir(base);
  while((dir = readdir(fd1)))   {
    // Skip '.' and '..'
    if (strcmp(dir->d_name, "." )==0 ||
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
      if (!strcmp(file->d_name, "temp_input1")) { // FIXME : do all sensors have a temp_input1 ?
	path = realloc(path, strlen(dname));
	strcpy(path, dname);			  // we've got it ;)
	done=1;
	break;
      }
    }
    closedir(fd2);
    if (done) break;
  }
  closedir(fd1);
  
  // fallback is path undefined
  if (*path != '/') {	
    error("[i2c_sensors] No i2c sensors found via the i2c interface !");
    error("[i2c_sensors] Try to specify the path to the sensors !");
  }
}


int plugin_init_i2c_sensors (void)
{
  //  char *path_cfg=cfg_get(NULL, "i", "");
  //  printf("%s\n", path_cfg);
  //  if (strncmp(path_cfg, "/", 1)) {
  //    printf("%s\n", "Calling my_i2c_sensors_path()");
  my_i2c_sensors_path();
  //  } else {
  //    path = realloc(path, strlen(path_cfg)+1);
  //    strcpy(path, path_cfg);
  //  }
  
  AddFunction ("i2c_sensors", 1, my_i2c_sensors);

  printf("%s\n", path);
  return 0;
}

