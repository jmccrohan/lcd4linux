/* $Id: drv_MatrixOrbital.c,v 1.1 2004/01/09 17:03:07 reinelt Exp $
 *
 * new style driver for Matrix Orbital serial display modules
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_MatrixOrbital.c,v $
 * Revision 1.1  2004/01/09 17:03:07  reinelt
 * initiated transfer to new driver architecture
 * new file 'drv.c' will someday replace 'display.c'
 * new file 'drv_MatrixOrbital.c' will replace 'MatrixOrbital.c'
 * due to this 'soft' transfer lcd4linux should stay usable during the switch
 * (at least I hope so)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_MatrixOrbital
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "lock.h"
#include "drv.h"
#include "bar.h"
#include "icon.h"

int drv_MO_list (void)
{
  printf ("this and that");
  return 0;
}

int drv_MO_init (struct DRIVER *Self)
{
  debug ("consider me initialized");
  return 0;
}

int drv_MO_quit (void) {
  debug ("consider me gone");
  return 0;
}


DRIVER drv_MatrixOrbital = {
  name: "MatrixOrbital",
  list:  drv_MO_list,
  init:  drv_MO_init,
  quit:  drv_MO_quit 
};
