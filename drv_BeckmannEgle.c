/* $Id: drv_BeckmannEgle.c,v 1.4 2004/06/02 09:41:19 reinelt Exp $
 *
 * driver for Beckmann+Egle mini terminals
 * Copyright 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_BeckmannEgle.c,v $
 * Revision 1.4  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.3  2004/06/02 05:14:16  reinelt
 *
 * fixed models listing for Beckmann+Egle driver
 * some cosmetic changes
 *
 * Revision 1.2  2004/05/31 05:38:02  reinelt
 *
 * fixed possible bugs with user-defined chars (clear high bits)
 * thanks to Andy Baxter for debugging the MilfordInstruments driver!
 *
 * Revision 1.1  2004/05/28 14:36:10  reinelt
 *
 * added drv_BeckmannEgle.c (forgotten at first check in :-)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_BeckmannEgle
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_serial.h"


static char Name[]="BeckmannEgle";

typedef struct {
  int type;
  char *name;
  int rows;
  int cols;
} MODEL;

static MODEL Models[] = {

  {  0, "MT16x1",   1, 16,},
  {  1, "MT16x2",   2, 16,},
  {  2, "MT16x4",   4, 16,},
  {  3, "MT20x1",   1, 20,},
  {  4, "MT20x2",   2, 20,},
  {  5, "MT20x4",   4, 20,},
  {  6, "MT24x1",   1, 24,},
  {  7, "MT24x2",   2, 24,},
  {  8, "MT32x1",   1, 32,},
  {  9, "MT32x2",   2, 32,},
  { 10, "MT40x1",   1, 40,},
  { 11, "MT40x2",   2, 40,},
  { 12, "MT40x4",   4, 40,},
  { -1, "unknown", -1, -1 },
};

static int  Model;


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_BE_write (int row, int col, unsigned char *data, int len)
{
  char cmd[] = "\033[y;xH";

  cmd[2] = (char)row;
  cmd[4] = (char)col;

  drv_generic_serial_write(cmd,6);
  drv_generic_serial_write (data, len);
}


static void drv_BE_defchar (int ascii, unsigned char *matrix)
{
  int  i;
  char cmd[32];
  char *p;
  
  p = cmd;
  *p++ = '\033';
  *p++ = '&';
  *p++ = 'T';                  // enter transparent mode
  *p++ = '\0';                 // write cmd
  *p++ = 0x40|8*ascii;         // write CGRAM
  for (i = 0; i < YRES; i++) {
    *p++ = '\1';               // write data
    *p++ = matrix[i] & 0x1f;   // character bitmap
  }
  *p++ = '\377';               // leave transparent mode

  drv_generic_serial_write (cmd, p-cmd);
}


static int drv_BE_start (char *section)
{
  int i;  
  char *model;
  char buffer[] = "\033&sX";
  
  model = cfg_get(section, "Model", NULL);
  if (model == NULL && *model == '\0') {
    error ("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
    return -1;
  }
  
  for (i=0; Models[i].type!=0xff; i++) {
    if (strcasecmp(Models[i].name, model)==0) break;
  }
  if (Models[i].type==0xff) {
    error ("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
    return -1;
  }
  Model=i;
  info ("%s: using model '%s'", Name, Models[Model].name);
  
  // CSTOPB: 2 stop bits
  if (drv_generic_serial_open(section, Name, CSTOPB)<0) return -1;
  
  // initialize global variables
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  
  buffer[4] = Models[Model].type;
  drv_generic_serial_write (buffer,   4); // select display type
  drv_generic_serial_write ("\033&D", 3); // cursor off
  drv_generic_serial_write ("\033&#", 3); // clear

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
// using drv_generic_text_icon_draw(W)
// using drv_generic_text_bar_draw(W)


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_BE_list (void)
{
  int i;
  
  for (i = 0; Models[i].type > -1; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_BE_init (char *section, int quiet)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // display preferences
  XRES  = 5;     // pixel width of one char 
  YRES  = 8;     // pixel height of one char 
  CHARS = 8;     // number of user-defineable characters
  CHAR0 = 0;     // ASCII of first user-defineable char
  GOTO_COST = 6; // number of bytes a goto command requires
  
  // real worker functions
  drv_generic_text_real_write   = drv_BE_write;
  drv_generic_text_real_defchar = drv_BE_defchar;


  // start display
  if ((ret=drv_BE_start (section))!=0)
    return ret;
  
  // initialize generic text driver
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  // initialize generic icon driver
  if ((ret=drv_generic_text_icon_init())!=0)
    return ret;
  
  // initialize generic bar driver
  if ((ret=drv_generic_text_bar_init(0))!=0)
    return ret;
  
  // add fixed chars to the bar driver
  drv_generic_text_bar_add_segment (  0,  0,255, 32); // ASCII  32 = blank
  drv_generic_text_bar_add_segment (255,255,255,255); // ASCII 255 = block
  
  // register text widget
  wc=Widget_Text;
  wc.draw=drv_generic_text_draw;
  widget_register(&wc);
  
  // register icon widget
  wc=Widget_Icon;
  wc.draw=drv_generic_text_icon_draw;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_generic_text_bar_draw;
  widget_register(&wc);
  
  // register plugins
  // none at the moment...

  return 0;
}


// close driver & display
int drv_BE_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_serial_close();
  drv_generic_text_quit();
  
  return (0);
}


DRIVER drv_BeckmannEgle = {
  name: Name,
  list: drv_BE_list,
  init: drv_BE_init,
  quit: drv_BE_quit, 
};

