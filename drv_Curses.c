/* $Id: drv_Curses.c,v 1.3 2004/06/05 06:13:11 reinelt Exp $
 *
 * pure ncurses based text driver
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old Curses/Text driver which is
 * Copyright 2001 Leopold Toetsch <lt@toetsch.at>
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
 * $Log: drv_Curses.c,v $
 * Revision 1.3  2004/06/05 06:13:11  reinelt
 *
 * splash screen for all text-based display drivers
 *
 * Revision 1.2  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.1  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Curses
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"


static char Name[]="Curses";

static WINDOW *w = NULL;
static WINDOW *e = NULL;

static int EROWS;


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_Curs_clear (void)
{
  werase (w);
  box (w, 0, 0);
  wrefresh (w);
}


static void drv_Curs_write (int row, int col, unsigned char *data, int len)
{
  char *p;
  
  while ((p = strpbrk(data, "\r\n")) != NULL) {
    *p = '\0';
  }
  
  if (col < DCOLS) {
    if (DCOLS-col < len ) len = DCOLS-col;
    mvwprintw(w, row+1 , col+1, "%.*s", DCOLS-col, data);
    wmove(w, DROWS+1, 0);
    wrefresh(w);  
  }
}


static void drv_Curs_defchar (int ascii, unsigned char *buffer)
{
  // empty
}


/* ncures scroll SIGSEGVs on my system, so this is a workaroud */

int curses_error(char *buffer) 
{
  static int lines = 0;
  static char *lb[100];
  int start, i;
  char *p;
  
  if (e == NULL) return 0;
  
  // replace \r, \n with underscores
  while ((p = strpbrk(buffer, "\r\n")) != NULL) {
    *p='_';
  }
  
  if (lines >= EROWS) {
    free(lb[0]);
    for (i = 1; i <= EROWS; i++) {
      lb[i-1] = lb[i];
    }
    start = 0;
  } else {
    start = lines;
  }
  
  lb[lines] = strdup(buffer);
  for (i = start; i <= lines; i++) {
    mvwprintw(e, i+1, 1, "%s", lb[i]);
    wclrtoeol(e);
  }

  box(e, 0, 0);
  mvwprintw(e, 0, 3, "Stderr:");
  wrefresh(e);

  if (lines < EROWS) lines++;
  
  return 1;
}


static int drv_Curs_start (char *section, int quiet)
{
  char *s;
  
  if (!running_foreground) {
    error("%s: You want me to display on /dev/null? Sorry, I can't ...", Name);
    error("%s: Maybe you want me to run in foreground? Try '-F'", Name);
    return -1;
  }
  
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
  
  initscr();
  debug ("%s: curses thinks that COLS=%d LINES=%d", Name, COLS, LINES);
  w = newwin(DROWS+2, DCOLS+2, 0, 0);
  
  EROWS = LINES-DROWS-3;
  if (EROWS > 99) EROWS = 99;
  debug ("EROWS=%d", EROWS);
  
  if (EROWS >= 4) {
    e = newwin (EROWS, COLS, DROWS+3, 0);
    EROWS -= 3;
    box(e, 0, 0);
    mvwprintw (e, 0, 3, "Stderr:");
    wmove(e, 1 , 0);
    wrefresh(e);
  }

  drv_Curs_clear();

  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer)) {
      sleep (3);
      drv_Curs_clear();
    }
  }
  
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
int drv_Curs_list (void)
{
  printf ("any");
  return 0;
}


// initialize driver & display
int drv_Curs_init (char *section, int quiet)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // display preferences
  XRES  = 1;     // pixel width of one char 
  YRES  = 1;     // pixel height of one char 
  CHARS = 0;     // number of user-defineable characters
  CHAR0 = 0;     // ASCII of first user-defineable char
  GOTO_COST = 0; // number of bytes a goto command requires
  
  // real worker functions
  drv_generic_text_real_write   = drv_Curs_write;
  drv_generic_text_real_defchar = drv_Curs_defchar;

  // start display
  if ((ret = drv_Curs_start (section, quiet)) != 0) {
    return ret;
  }
  
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
int drv_Curs_quit (void) {

  info("%s: shutting down.", Name);

  drv_generic_text_quit();
  
  // clear display
  drv_Curs_clear();

  // say goodbye...
  drv_generic_text_greet ("goodbye!");

  endwin();
  
  return (0);
}


DRIVER drv_Curses = {
  name: Name,
  list: drv_Curs_list,
  init: drv_Curs_init,
  quit: drv_Curs_quit, 
};

