/* $Id: widget.c,v 1.6 2004/01/11 18:26:02 reinelt Exp $
 *
 * generic widget handling
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
 * $Log: widget.c,v $
 * Revision 1.6  2004/01/11 18:26:02  reinelt
 * further widget and layout processing
 *
 * Revision 1.5  2004/01/11 09:26:15  reinelt
 * layout starts to exist...
 *
 * Revision 1.4  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.3  2004/01/10 17:34:40  reinelt
 * further matrixOrbital changes
 * widgets initialized
 *
 * Revision 1.2  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.1  2003/09/19 03:51:29  reinelt
 * minor fixes, widget.c added
 *
 */

/* 
 * exported functions:
 *
 * int widget_junk(void)
 *   does something
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "widget.h"


static WIDGET_CLASS *Classes=NULL;
static int          nClasses=0;

static WIDGET       *Widgets=NULL;
static int          nWidgets=0;


int widget_register (WIDGET_CLASS *widget)
{
  int i;

  for (i=0; i<nClasses; i++) {
    if (strcasecmp(widget->name, Classes[i].name)==0) {
      error ("internal error: widget '%s' already exists!");
      return -1;
    }
  }

  nClasses++;
  Classes=realloc(Classes, nClasses*sizeof(WIDGET_CLASS));
  Classes[nClasses-1] = *widget;
  
  return 0;
}


int widget_add (char *name)
{
  int i;
  char *section;
  char *class;
  
  WIDGET_CLASS *Class;
  WIDGET       *Widget;
  
  // prepare config section
  // strlen("Widget:")=7
  section=malloc(strlen(name)+8);
  strcpy(section, "Widget:");
  strcat(section, name);
  
  // get widget class
  class=cfg_get(section, "class", NULL);
  if (class==NULL || *class=='\0') {
    error ("error: widget '%s' has no class!", name);
    return -1;
  }
  
  // lookup widget class
  for (i=0; i<nClasses; i++) {
    if (strcasecmp(class, Classes[i].name)==0) {
      Class=&(Classes[i]);
      break;
    }
  }
  if (i==nClasses) {
    error ("widget '%s': class '%s' not supported");
    return -1;
  }
  
  nWidgets++;
  Widgets=realloc(Widgets, nWidgets*sizeof(WIDGET));
  Widget=&(Widgets[nWidgets-1]);
  
  Widget->name  = name;
  Widget->class = Class;
  
  if (Class->init!=0) {
    Class->init(Widget);
  }

  return 0;
}
