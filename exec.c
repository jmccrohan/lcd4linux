/* $Id: exec.c,v 1.2 2001/03/08 08:39:54 reinelt Exp $
 *
 * exec ('x*') functions
 *
 * Copyright 2001 by Leopold Tötsch (lt@toetsch.at)
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
 * $Log: exec.c,v $
 * Revision 1.2  2001/03/08 08:39:54  reinelt
 *
 * fixed two typos
 *
 * Revision 1.1  2001/03/07 18:10:21  ltoetsch
 * added e(x)ec commands
 *
 *
 */

#include <stdio.h>
#include <time.h>
#include "exec.h"
#include "debug.h"
#include "cfg.h"


int Exec(int index, char buff[EXEC_TXT_LEN])
{
  static time_t now = 0;
  char *command;
  char xn[4];
  FILE *pipe;
  
  if (time(NULL) <= now+EXEC_WAIT) 
    return 0;
  time(&now); 
  
  sprintf(xn, "x%d", index);
  command = cfg_get(xn);
  debug("command%d = %s:%s",index,xn,command);
					    
  if (!*command) {
    error("Empty command for 'x%d'", index);
    return -1;
  }
  
  pipe = popen(command, "r");
  if (pipe == NULL) {
    error("Couln't run pipe '%s'", command);
    return -1;
  }
  fread(buff, EXEC_TXT_LEN, 1, pipe);
  fclose(pipe);
  buff[EXEC_TXT_LEN-1] = '\0';
  return 0;
}

