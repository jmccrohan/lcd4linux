/* $Id: widget_text.c,v 1.6 2004/01/15 07:47:02 reinelt Exp $
 *
 * simple text widget handling
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
 * $Log: widget_text.c,v $
 * Revision 1.6  2004/01/15 07:47:02  reinelt
 * debian/ postinst and watch added (did CVS forget about them?)
 * evaluator: conditional expressions (a?b:c) added
 * text widget nearly finished
 *
 * Revision 1.5  2004/01/15 04:29:45  reinelt
 * moved lcd4linux.conf.sample to *.old
 * lcd4linux.conf.sample with new layout
 * new plugins 'loadavg' and 'meminfo'
 * text widget have pre- and postfix
 *
 * Revision 1.4  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.3  2004/01/13 08:18:20  reinelt
 * timer queues added
 * liblcd4linux deactivated turing transformation to new layout
 *
 * Revision 1.2  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
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
 * WIDGET_CLASS Widget_Text
 *   a simple text widget which 
 *   must be supported by all displays
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
#include "widget_text.h"


void widget_text_scroll (void *Self)
{
  WIDGET      *W = (WIDGET*)Self;
  WIDGET_TEXT *T = W->data;
  
  int num, len, width, pad;
  char *src, *dst;

  num   = 0;
  len   = strlen(T->value);
  width = T->width-strlen(T->preval)-strlen(T->postval);
  if (width<0) width=0;
  
  switch (T->align) {
  case ALIGN_LEFT:
    pad=0;
    break;
  case ALIGN_CENTER:
    pad=(width - len)/2;  
    if (pad<0) pad=0;
    break;
  case ALIGN_RIGHT:
    pad=width - len; 
    if (pad<0) pad=0;
    break;
  case ALIGN_MARQUEE:
    pad=width - T->scroll;
    T->scroll++;
    if (T->scroll >= width+len) T->scroll=0;
    break;
  default: // not reached 
    pad=0;
  }
  
  dst=T->buffer;

  // process prefix
  src=T->preval;
  while (num < T->width) {
    if (*src=='\0') break;
    *(dst++)=*(src++);
    num++;
  }
  
  src=T->value;

  // pad blanks on the beginning
  while (pad > 0 && num < T->width) {
    *(dst++)=' ';
    num++;
    pad--;
  }
  
  // skip src chars (marquee)
  while (pad<0) {
    src++;
    pad++;
  }
  
  // copy content
  while (num < T->width) {
    if (*src=='\0') break;
    *(dst++)=*(src++);
    num++;
  }
  
  // pad blanks on the end
  src=T->postval;
  len=strlen(src);
  while (num < T->width-len) {
    *(dst++)=' ';
    num++;
  }
  
  // process postfix
  while (num < T->width) {
    if (*src=='\0') break;
    *(dst++)=*(src++);
    num++;
  }
  
  *dst='\0';
  
  // finally, draw it!
  if (W->class->draw)
    W->class->draw(W);
}


void widget_text_update (void *Self)
{
  WIDGET      *W = (WIDGET*)Self;
  WIDGET_TEXT *T = W->data;
  RESULT result = {0, 0.0, NULL};
  char *preval, *postval, *value;
  int update;
  
  // evaluate prefix
  if (T->prefix!=NULL && strlen(T->prefix)>0) {
    Eval(T->prefix, &result);
    preval=strdup(R2S(&result));
    DelResult (&result);
  } else {
    preval=strdup("");
  }
  
  // evaluate postfix
  if (T->postfix!=NULL && *(T->postfix)!='\0') {
    Eval(T->postfix, &result);
    postval=strdup(R2S(&result));
    DelResult (&result);
  } else {
    postval=strdup("");
  }
  
  // evaluate expression
  Eval(T->expression, &result);
  
  // string or number?
  if (T->precision==0xC0DE) {
    value=strdup(R2S(&result));
  } else {
    double number=R2N(&result);
    int width=T->width-strlen(preval)-strlen(postval);
    if (width<0) width=0;
    int precision=T->precision;
    // print zero bytes so we can specify NULL as target 
    // and get the length of the resulting string
    int size=snprintf (NULL, 0, "%.*f", precision, number);
    // number does not fit into field width: try to reduce precision
    if (size>width && precision>0) {
      int delta=size-width;
      if (delta>precision) delta=precision;
      precision-=delta;
      size-=delta;
      // zero precision: omit decimal point, too
      if (precision==0) size--;
    }
    // number still doesn't fit: display '*****' 
    if (size>width) {
      value=malloc(width+1);
      memset (value, '*', width);
      *(value+width)='\0';
    } else {
      value=malloc(size+1);
      snprintf (value, size+1, "%.*f", precision, number);
    }
  }
  
  DelResult (&result);
  
  update=0;
  
  // prefix changed?
  if (T->preval == NULL || strcmp(T->preval, preval)!=0) {
    update=1;
    if (T->preval) free (T->preval); 
    T->preval=preval;   
    T->scroll=0; // reset marquee counter
  } else {              
    free (preval);      
  }
  
  // postfix changed?
  if (T->postval == NULL || strcmp(T->postval, postval)!=0) {
    update=1;
    if (T->postval) free (T->postval);
    T->postval=postval;
    T->scroll=0; // reset marquee counter
  } else {
    free (postval);
  }
  
  // value changed?
  if (T->value == NULL || strcmp(T->value, value)!=0) {
    update=1;
    if (T->value) free (T->value);
    T->value=value;
    T->scroll=0; // reset marquee counter
  } else {
    free (value);
  }

  // something has changed and should be updated
  if (update) {
    // if there's a marquee scroller active, it has its own
    // update callback timer, so we do nothing here; otherwise
    // we simply call this scroll callback directly
    if (T->align!=ALIGN_MARQUEE) {
      widget_text_scroll (Self);
    }
  }

}


int widget_text_init (WIDGET *Self) 
{
  char *section; char *c;
  WIDGET_TEXT *T;
  
  // prepare config section
  // strlen("Widget:")=7
  section=malloc(strlen(Self->name)+8);
  strcpy(section, "Widget:");
  strcat(section, Self->name);
  
  T=malloc(sizeof(WIDGET_TEXT));
  memset (T, 0, sizeof(WIDGET_TEXT));

  // get raw pre- and postfix (we evaluate it ourselves)
  T->prefix  = cfg_get_raw (section, "prefix",  NULL);
  T->postfix = cfg_get_raw (section, "postfix", NULL);

  // get raw expression (we evaluate it ourselves)
  T->expression = cfg_get_raw (section, "expression",  "''");
  
  // field width, default 10
  cfg_number (section, "width", 10,  0, 99999, &(T->width));
  
  // precision: number of digits after the decimal point (default: none)
  // Note: this is the *maximum* precision on small values,
  // for larger values the precision may be reduced to fit into the field width.
  // The default value 0xC0DE is used to distinguish between numbers and strings:
  // if no precision is given, the result is always treated as a string. If a
  // precision is specified, the result is treated as a number.
  cfg_number (section, "precision", 0xC0DE, 0, 80, &(T->precision));
  
  // field alignment: Left (default), Center, Right or Marquee
  c = cfg_get (section, "align",  "L");
  switch (toupper(*c)) {
  case 'L':
    T->align=ALIGN_LEFT;
    break;
  case 'C':
    T->align=ALIGN_CENTER;
    break;
  case 'R':
    T->align=ALIGN_RIGHT;
    break;
  case 'M':
    T->align=ALIGN_MARQUEE;
    break;
  default:
    error ("widget %s has unknown alignment '%s', using 'Left'", section, c);
    T->align=ALIGN_LEFT;
  }
  free (c);
  
  // update interval (msec), default 1 sec
  cfg_number (section, "update", 1000, 10, 99999, &(T->update));
  
  // marquee scroller speed: interval (msec), default 500msec
  if (T->align==ALIGN_MARQUEE) {
    cfg_number (section, "speed", 500, 10, 99999, &(T->speed));
  }
  
  // buffer
  T->buffer=malloc(T->width+1);
  
  free (section);
  Self->data=T;
  
  timer_add (widget_text_update, Self, T->update, 0);

  // a marquee scroller has its own timer and callback
  if (T->align==ALIGN_MARQUEE) {
    timer_add (widget_text_scroll, Self, T->speed, 0);
  }

  return 0;
}


int widget_text_quit (WIDGET *Self) {

  if (Self->data) {
    free (Self->data);
    Self->data=NULL;
  }
  
  return 0;
  
}



WIDGET_CLASS Widget_Text = {
  name:   "text",
  init:   widget_text_init,
  draw:   NULL,
  quit:   widget_text_quit,
};


