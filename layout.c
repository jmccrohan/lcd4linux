/* $Id: layout.c,v 1.1 2004/01/10 20:22:33 reinelt Exp $
 *
 * new layouter framework
 *
 * Copyright 1999-2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: layout.c,v $
 * Revision 1.1  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 */

/* 
 * exported functions:
 *
 * layout_init (char *section)
 *    initializes the layouter
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "layout.h"


#define MAX_ROWS 32
#define MAX_COLS 80

int layout_init (char *layout)
{
  char *section, *widget;
  char  buffer[15];
  int row, col;
  
  info ("initializing layout '%s'", layout);
  
  // prepare config section
  // strlen("Layout:")=7, +1=8
  section=malloc(strlen(layout)+8);
  strcpy(section, "Layout:");
  strcat(section, layout);

  cfg_list(section);

  
  return 0;
}

