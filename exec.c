/* $Id: exec.c,v 1.13 2004/03/03 03:47:04 reinelt Exp $
 *
 * exec ('x*') functions
 *
 * Copyright 2001 Leopold Tötsch <lt@toetsch.at>
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
 * $Log: exec.c,v $
 * Revision 1.13  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.12  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.11  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.10  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.9  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.8  2003/02/05 04:31:38  reinelt
 *
 * T_EXEC: remove trailing CR/LF
 * T_EXEC: deactivated maxlen calculation (for I don't understand what it is for :-)
 *
 * Revision 1.7  2002/04/29 11:00:28  reinelt
 *
 * added Toshiba T6963 driver
 * added ndelay() with nanosecond resolution
 *
 * Revision 1.6  2001/03/15 09:13:22  ltoetsch
 * delay first exec for faster start
 *
 * Revision 1.5  2001/03/13 08:34:15  reinelt
 *
 * corrected a off-by-one bug with sensors
 *
 * Revision 1.4  2001/03/09 14:24:49  ltoetsch
 * exec: Scale_x ->Min/Max_x
 *
 * Revision 1.3  2001/03/08 15:25:38  ltoetsch
 * improved exec
 *
 * Revision 1.2  2001/03/08 08:39:54  reinelt
 *
 * fixed two typos
 *
 * Revision 1.1  2001/03/07 18:10:21  ltoetsch
 * added e(x)ec commands
 *
 *
 * This implements the x1 .. x9 commands
 * config options:
 *   x1 .. x9      command to execute
 *   Tick_x1 ... 9 delay in ticks
 *   Delay_x1 .. 9 delay in seconds
 *   Max_x1 .. 9   max for scaling bars (100)
 *   Min_x1 .. 9   min for scaling bars (0)
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#define IN_EXEC
#include "exec.h"
#include "debug.h"
#include "cfg.h"


int Exec(int index, char buff[EXEC_TXT_LEN], double *val)
{
  static time_t now[EXECS+1];
  static int errs[EXECS+1];
  static int ticks[EXECS+1];
  char *command, *strticks;
  char xn[20];
  char env[EXEC_TXT_LEN];
  FILE *pipe;
  size_t len;
  int i;

  if (index < 0 || index > EXECS)
    return -1; 
  if (errs[index])
    return -1;
  
  /* first time ? */
  if (now[index] == 0) { /* not first time, to give faster a chance */
    now[index] = -1;
    return 0;
  }
  if (now[index] > 0) {
    /* delay in Ticks ? */
    sprintf(xn, "Tick_x%d", index);
    strticks = cfg_get(NULL, xn, NULL);
    if (p && *p) {
      if (ticks[index]++ % atoi(strticks) != 0) {
	    free(strticks);
        return 0;
      }
	  free(strticks);
    }
    else {
	  free(strticks);	
      sprintf(xn, "Delay_x%d", index);
      /* delay in Delay_x* sec ? */
      if (time(NULL) <= now[index] + atoi(cfg_get(NULL, xn, "1"))) {
        return 0;
      }
    }
  }
  time(&now[index]); 
  *val = -1;
  
  sprintf(xn, "x%d", index);
  command = cfg_get(NULL, xn, NULL);
					    
  if (!command || !*command) {
    error("Empty command for 'x%d'", index);
    errs[index]++;
	if (command) free(command);
    return -1;
  }
  for (i = 1; i < index; i++) {
    sprintf(env, "X%d=%.*s", i, EXEC_TXT_LEN-10, exec[i].s);
    putenv(env);
  }
  putenv("PATH=/usr/local/bin:/usr/bin:/bin");
  pipe = popen(command, "r");
  if (pipe == NULL) {
    error("Couldn't run pipe '%s':\n%s", command, strerror(errno));
    errs[index]++;
	free(command);
    return -1;
  }
  len = fread(buff, 1, EXEC_TXT_LEN-1,  pipe);
  if (len <= 0) {
    pclose(pipe);
    error("Couldn't fread from pipe '%s', len=%d", command, len);
    errs[index]++;
    *buff = '\0';
	free(command);
    return -1;
  }
  pclose(pipe);
  buff[len] = '\0';

  // remove trailing CR/LF
  while (buff[len-1]=='\n' || buff[len-1]=='\r') {
    buff[--len]='\0';
  }

  debug("%s: <%s> = '%s'",xn,command,buff);

  if (isdigit(*buff)) {
    double max, min;
    *val = atof(buff);
    sprintf(xn, "Max_x%d", index);
    max = atof(cfg_get(NULL, xn, "100"));
    sprintf(xn, "Min_x%d", index);
    min = atof(cfg_get(NULL, xn, "0"));
    if (max != min)
      *val = (*val - min)/(max - min);
  }
  free(command);
  return 0;
}

