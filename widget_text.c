/* $Id: widget_text.c,v 1.1 2004/01/10 20:22:33 reinelt Exp $
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

#include "debug.h"
#include "cfg.h"
#include "widget.h"


WIDGET_CLASS Widget_Text = {
  name:   "text",
  init:   NULL,
  update: NULL,
  render: NULL,
  quit:   NULL,
};


