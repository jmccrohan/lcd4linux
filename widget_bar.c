/* $Id: widget_bar.c,v 1.3 2004/01/20 12:45:47 reinelt Exp $
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

  double val1, val2;
  double min, max;
  
  // evaluate expressions
  val1 = 0.0;
  if (T->expression1!=NULL && *T->expression1!='\0') {
    Eval(T->expression1, &result); 
    val1 = R2N(&result); 
    DelResult(&result);
  }
  
  val2 = val1;
  if (T->expression2!=NULL && *T->expression2!='\0') {
    Eval(T->expression2, &result); 
    val2 = R2N(&result); 
    DelResult(&result);
  }
  
  // minimum: if expression is empty, do auto-scaling
  if (T->expr_min!=NULL && *T->expr_min!='\0') {
    Eval(T->expr_min, &result); 
    min = R2N(&result); 
    DelResult(&result);
  } else {
    min = T->min;
    if (val1 < min) min = val1;
    if (val2 < min) min = val2;
  }
  
  // maximum: if expression is empty, do auto-scaling
  if (T->expr_max!=NULL && *T->expr_max!='\0') {
    Eval(T->expr_max, &result); 
    max = R2N(&result); 
    DelResult(&result);
  } else {
    max = T->max;
    if (val1 > max) max = val1;
    if (val2 > max) max = val2;
  }
  
  // calculate bar values
  T->min=min;
  T->max=max;
  if (max>min) {
    T->val1=(val1-min)/(max-min);
    T->val2=(val2-min)/(max-min);
  } else {
    T->val1=0.0;
    T->val2=0.0;
  }
  
  // finally, draw it!
  if (W->class->draw)
    W->class->draw(W);
  
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
  B->expression1 = cfg_get_raw (section, "expression",  NULL);
  B->expression2 = cfg_get_raw (section, "expression2", NULL);
  
  // sanity check
  if (B->expression1==NULL || *B->expression1=='\0') {
    error ("widget %s has no expression, using '0.0'", Self->name, c);
    B->expression1="0";
  }
  
  // minimum and maximum value
  B->expr_min = cfg_get_raw (section, "min", NULL);
  B->expr_max = cfg_get_raw (section, "max", NULL);

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
    error ("widget %s has unknown direction '%s', using 'East'", Self->name, c);
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
  name:   "bar",
  init:   widget_bar_init,
  draw:   NULL,
  quit:   widget_bar_quit,
};


