/* $Id: widget_bar.c,v 1.1 2004/01/18 21:25:16 reinelt Exp $
 *
 * bar widget handling
 *
 * Copyright 2003,2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: widget_bar.c,v $
 * Revision 1.1  2004/01/18 21:25:16  reinelt
 * Framework for bar widget opened
 *
 */

/* 
 * exported functions:
 *
 * WIDGET_CLASS Widget_Bar
 *   the bar widget
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cfg.h"
#include "evaluator.h"
#include "timer.h"
#include "widget.h"
#include "widget_bar.h"


void widget_bar_update (void *Self)
{
  WIDGET      *W = (WIDGET*)Self;
  WIDGET_BAR *T = W->data;
  RESULT result = {0, 0.0, NULL};
}


int widget_bar_init (WIDGET *Self) 
{
  char *section; char *c;
  WIDGET_BAR *B;
  
  // prepare config section
  // strlen("Widget:")=7
  section=malloc(strlen(Self->name)+8);
  strcpy(section, "Widget:");
  strcat(section, Self->name);
  
  B=malloc(sizeof(WIDGET_BAR));
  memset (B, 0, sizeof(WIDGET_BAR));

  // get raw expressions (we evaluate them ourselves)
  B->expression1 = cfg_get_raw (section, "expression",   "''");
  B->expression2 = cfg_get_raw (section, "expression2",  "''");
  
  // bar length, default 1
  cfg_number (section, "length", 1,  0, 99999, &(B->length));
  
  // direction: East (default), West, North, South
  c = cfg_get (section, "direction",  "E");
  switch (toupper(*c)) {
  case 'E':
    B->direction=DIR_EAST;
    break;
  case 'W':
    B->direction=DIR_WEST;
    break;
  case 'N':
    B->direction=DIR_NORTH;
    break;
  case 'S':
    B->direction=DIR_SOUTH;
    break;
  default:
    error ("widget %s has unknown direction '%s', using 'East'", section, c);
    B->direction=DIR_EAST;
  }
  free (c);
  
  // update interval (msec), default 1 sec
  cfg_number (section, "update", 1000, 10, 99999, &(B->update));
  
  // buffer
  // B->buffer=malloc(B->width+1);
  
  free (section);
  Self->data=B;
  
  timer_add (widget_bar_update, Self, B->update, 0);

  return 0;
}


int widget_bar_quit (WIDGET *Self) {

  if (Self->data) {
    free (Self->data);
    Self->data=NULL;
  }
  
  return 0;
  
}



WIDGET_CLASS Widget_Bar = {
  name:   "text",
  init:   widget_bar_init,
  draw:   NULL,
  quit:   widget_bar_quit,
};


