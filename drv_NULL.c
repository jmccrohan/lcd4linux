/* $Id: drv_NULL.c,v 1.2 2004/06/02 09:41:19 reinelt Exp $
 *
 * NULL driver (for testing)
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_NULL.c,v $
 * Revision 1.2  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.1  2004/05/31 16:39:06  reinelt
 *
 * added NULL display driver (for debugging/profiling purposes)
 * added backlight/contrast initialisation for matrixOrbital
 * added Backlight initialisation for Cwlinux
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_NULL
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"


static char Name[]="NULL";


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_NULL_write (int row, int col, unsigned char *data, int len)
{
  // empty
}


static void drv_NULL_defchar (int ascii, unsigned char *matrix)
{
  // empty
}


static int drv_NULL_start (char *section)
{
  char *s;
  
  s = cfg_get(section, "Size", "20x4");
  if (s == NULL || *s == '\0') {
    error ("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
    free(s);
    return -1;
  }
  if (sscanf(s, "%dx%d", &DCOLS, &DROWS) != 2 || DROWS < 1 || DCOLS < 1) {
    error ("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source);
    free(s);
    return -1;
  }
  free (s);
  
  return 0;
}


// ****************************************
// ***            plugins               ***
// ****************************************

// none at the moment...


// ****************************************
// ***        widget callbacks          ***
// ****************************************

// using drv_generic_text_draw(W)
// using drv_generic_text_bar_draw(W)


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_NULL_list (void)
{
  printf ("generic");
  return 0;
}


// initialize driver & display
int drv_NULL_init (char *section, int quiet)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // display preferences
  XRES  = 6;     // pixel width of one char 
  YRES  = 8;     // pixel height of one char 
  CHARS = 8;     // number of user-defineable characters
  CHAR0 = 0;     // ASCII of first user-defineable char
  GOTO_COST = 2; // number of bytes a goto command requires
  
  // real worker functions
  drv_generic_text_real_write   = drv_NULL_write;
  drv_generic_text_real_defchar = drv_NULL_defchar;

  // start display
  if ((ret = drv_NULL_start (section)) != 0)
    return ret;
  
  // initialize generic text driver
  if ((ret = drv_generic_text_init(section, Name)) != 0)
    return ret;

  // initialize generic bar driver
  if ((ret = drv_generic_text_bar_init(1)) != 0)
    return ret;
  
  // add fixed chars to the bar driver
  drv_generic_text_bar_add_segment (  0,  0,255, 32); // ASCII  32 = blank
  drv_generic_text_bar_add_segment (255,255,255,'*'); // asterisk
  
  // register text widget
  wc = Widget_Text;
  wc.draw = drv_generic_text_draw;
  widget_register(&wc);
  
  // register bar widget
  wc = Widget_Bar;
  wc.draw = drv_generic_text_bar_draw;
  widget_register(&wc);

  // register plugins
  // none at the moment...

  return 0;
}


// close driver & display
int drv_NULL_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_text_quit();
  
  return (0);
}


DRIVER drv_NULL = {
  name: Name,
  list: drv_NULL_list,
  init: drv_NULL_init,
  quit: drv_NULL_quit, 
};

