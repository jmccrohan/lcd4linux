/* $Id: widget_bar.c,v 1.6 2004/01/29 04:40:03 reinelt Exp $
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
 * Revision 1.6  2004/01/29 04:40:03  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.5  2004/01/23 04:54:00  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.4  2004/01/20 14:25:12  reinelt
 * some reorganization
 * moved drv_generic to drv_generic_serial
 * moved port locking stuff to drv_generic_serial
 *
 * Revision 1.3  2004/01/20 12:45:47  reinelt
 * "Default screen" working with MatrixOrbital
 *
 * Revision 1.2  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
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


#include "config.h"

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
  WIDGET_BAR *Bar = W->data;
  RESULT result = {0, 0.0, NULL};

  double val1, val2;
  double min, max;
  
  // evaluate expressions
  val1 = 0.0;
  if (Bar->expression1!=NULL && *Bar->expression1!='\0') {
    Eval(Bar->expression1, &result); 
    val1 = R2N(&result); 
    DelResult(&result);
  }
  
  val2 = val1;
  if (Bar->expression2!=NULL && *Bar->expression2!='\0') {
    Eval(Bar->expression2, &result); 
    val2 = R2N(&result); 
    DelResult(&result);
  }
  
  // minimum: if expression is empty, do auto-scaling
  if (Bar->expr_min!=NULL && *Bar->expr_min!='\0') {
    Eval(Bar->expr_min, &result); 
    min = R2N(&result); 
    DelResult(&result);
  } else {
    min = Bar->min;
    if (val1 < min) min = val1;
    if (val2 < min) min = val2;
  }
  
  // maximum: if expression is empty, do auto-scaling
  if (Bar->expr_max!=NULL && *Bar->expr_max!='\0') {
    Eval(Bar->expr_max, &result); 
    max = R2N(&result); 
    DelResult(&result);
  } else {
    max = Bar->max;
    if (val1 > max) max = val1;
    if (val2 > max) max = val2;
  }
  
  // calculate bar values
  Bar->min=min;
  Bar->max=max;
  if (max>min) {
    Bar->val1=(val1-min)/(max-min);
    Bar->val2=(val2-min)/(max-min);
  } else {
    Bar->val1=0.0;
    Bar->val2=0.0;
  }
  
  // finally, draw it!
  if (W->class->draw)
    W->class->draw(W);
  
}



int widget_bar_init (WIDGET *Self) 
{
  char *section; char *c;
  WIDGET_BAR *Bar;
  
  // prepare config section
  // strlen("Widget:")=7
  section=malloc(strlen(Self->name)+8);
  strcpy(section, "Widget:");
  strcat(section, Self->name);
  
  Bar=malloc(sizeof(WIDGET_BAR));
  memset (Bar, 0, sizeof(WIDGET_BAR));

  // get raw expressions (we evaluate them ourselves)
  Bar->expression1 = cfg_get_raw (section, "expression",  NULL);
  Bar->expression2 = cfg_get_raw (section, "expression2", NULL);
  
  // sanity check
  if (Bar->expression1==NULL || *Bar->expression1=='\0') {
    error ("widget %s has no expression, using '0.0'", Self->name);
    Bar->expression1="0";
  }
  
  // minimum and maximum value
  Bar->expr_min = cfg_get_raw (section, "min", NULL);
  Bar->expr_max = cfg_get_raw (section, "max", NULL);

  // bar length, default 1
  cfg_number (section, "length", 1,  0, 99999, &(Bar->length));
  
  // direction: East (default), West, North, South
  c = cfg_get (section, "direction",  "E");
  switch (toupper(*c)) {
  case 'E':
    Bar->direction=DIR_EAST;
    break;
  case 'W':
    Bar->direction=DIR_WEST;
    break;
  case 'N':
    Bar->direction=DIR_NORTH;
    break;
  case 'S':
    Bar->direction=DIR_SOUTH;
    break;
  default:
    error ("widget %s has unknown direction '%s', using 'East'", Self->name, c);
    Bar->direction=DIR_EAST;
  }
  free (c);
  
  // update interval (msec), default 1 sec
  cfg_number (section, "update", 1000, 10, 99999, &(Bar->update));
  
  // buffer
  // Bar->buffer=malloc(Bar->width+1);
  
  free (section);
  Self->data=Bar;
  
  timer_add (widget_bar_update, Self, Bar->update, 0);
  
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
  name:   "bar",
  init:   widget_bar_init,
  draw:   NULL,
  quit:   widget_bar_quit,
};


