/* $ID$
 *
 * seti@home specific functions
 *
 * Copyright 2001 by Axel Ehnert <Axel@Ehnert.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: seti.c,v $
 * Revision 1.1  2001/02/18 21:15:15  reinelt
 *
 * added setiathome client
 *
 */

/* 
 * exported functions:
 *
 * Seti (int *perc, int *cput)
 *   returns 0 if ok, -1 if error
 *   sets *perc to the percentage completed by seti@home client
 *   sets *perc to the cpu time used
 *
 */

#define FALSE 0
#define TRUE 1

#define STATEFILE "state.sah"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include "cfg.h"
#include "debug.h"
#include "seti.h"

int Seti (int *perc, int *cput)
{
  FILE *fstr;
  static int err_marker=0; // Was there an erro before -> -2
  static time_t cnt;       // Time of last calculation
  static int retry_cnt=0;  // Retry 10 times to find prog=
  char *dirname;           // Directory of Seti@HOME
  char *fn;
  char fn1[200];
  char *txt;
  char txt1[100];
  int  v1=0;
  int  i;
  int  l;
  int  found_perc;         // Flag to show, if we allready found
  int  found_cpu;          // Flag to show, if we allready found
  int  interv=-1;
  char *cinterv;

  /*
   * Was there an error before? Return any way
   */
  if (err_marker == -2) {
    return (-1);
  }
  /*
    Interval set?
  */
  if (interv < 0) {
    cinterv = cfg_get("pollintseti");
    if ( cinterv == NULL ) {
      interv=DEFSETIPOLLEXT;
    }
    else {
      interv = atoi(cinterv);
    }
  }
  /*
    Is it time to look into the file?
  */
  if (time(NULL)>cnt+interv-1) {
    cnt=time(NULL);
  }
  else {
    return 0;
  }
  /*
    Reread pollext, because it could be changed due to reading a new conf file
  */
  cinterv = cfg_get("pollintseti");
  if ( cinterv == NULL ) {
    interv=DEFSETIPOLLEXT;
  }
  else {
    interv = atoi(cinterv);
  }
  /*
    Build the filename from the config
  */
  dirname=cfg_get("SetiDir");
  if (dirname==NULL || *dirname=='\0') {
    error ("%s: missing 'SetiDir' entry!\n", cfg_file());
    err_marker = -2;
    return (-1);
  }
  
  fn=&fn1[0];
  strcpy(fn, dirname);
  strcat(fn, "/state.sah");
  /*
    Open the file
  */
  fstr=fopen(fn,"r");

  if (fstr == NULL) {
    error ("File %s could not be opened!\n", fn);
    err_marker = -2;
    return (-1);
  }
  /* 
     Read the file. Break the loop after we found all strings.
  */
  found_perc=FALSE;
  found_cpu=FALSE;
  txt=&txt1[0];

  while ( ( fgets ( txt1, 100, fstr ) ) != NULL ) {
    if ( strncmp (txt1, "prog=", 5 ) == 0 ) {
      txt=strncpy(txt,txt+7,4);
      txt[4]='\0';
      debug ("Seti in text: %s", txt);
      i=sscanf(txt, "%d", &v1);
      debug ("Seti in numb: %d", v1);
      *perc=v1;
      found_perc=TRUE;
    }
    if ( strncmp (txt1, "cpu=", 4 ) == 0 ) {
      l=strstr(txt+4,".")-txt-4;
      txt=strncpy(txt,txt+4,l);
      txt[l]='\0';
      i=sscanf(txt, "%d", &v1);
      *cput=v1;
      found_cpu=TRUE;
    }
    if (found_perc && found_cpu) {
      retry_cnt = 0;               // Reset retry counter. WE FOUND!
      fclose(fstr);
      return (0);
    }
  }

  retry_cnt++;
  if ( retry_cnt < 10 ) {
    error ("%s: prog= or cpu= not found in file! Retrying ...\n", fn);
    return 0;
  } 
  else {
    error ("%s: prog= or cpu= not found in file!\n", fn);
    err_marker = -2;
    return (-1);
  }
}


int newSeti (double *perc, double *cput)
{
  static char fn[256];
  static time_t now=0;
  static int fd=-2;
  static double v1=0;
  static double v2=0;
  char buffer[8192], *p;
  
  *perc=v1;
  *cput=v2;

  if (fd==-1) return -1;
  
  if (time(NULL)==now) return 0;
  time(&now);
  
  if (fd==-2) {
    char *dir=cfg_get("SetiDir");
    if (dir==NULL || *dir=='\0') {
      error ("%s: missing 'SetiDir' entry!\n", cfg_file());
      fd=-1;
      return -1;
    }
    if (strlen(dir)>sizeof(fn)-sizeof(STATEFILE)-2) {
      error ("%s: 'SetiDir' too long!\n", cfg_file());
      fd=-1;
      return -1;
    }
    strcpy(fn, dir);
    strcat(fn, "/");
    strcat(fn, STATEFILE);
  }

  fd = open(fn, O_RDONLY);
  if (fd==-1) {
    error ("open(%s) failed: %s", fn, strerror(errno));
    return -1;
  }

  if (read (fd, &buffer, sizeof(buffer)-1)==-1) {
    error ("read(%s) failed: %s", fn, strerror(errno));
    fd=-1;
    return -1;
  }
  
  p=strstr(buffer, "prog=");
  if (p==NULL) {
    error ("parse(%s) failed: no 'prog=' line", fn);
    fd=-1;
    return -1;
  }
  if (sscanf(p+5, "%lf", &v1)!=1) {
    error ("parse(%s) failed: unknown 'prog=' format", fn);
    fd=-1;
    return -1;
  }

  p=strstr(buffer, "cpu=");
  if (p==NULL) {
    error ("parse(%s) failed: no 'cpu=' line", fn);
    fd=-1;
    return -1;
  }
  if (sscanf(p+4, "%lf", &v2)!=1) {
    error ("parse(%s) failed: unknown 'cpu=' format", fn);
    fd=-1;
    return -1;
  }

  *perc=v1;
  *cput=v2;

  return 0;

}
