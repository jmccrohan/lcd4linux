/* $Id: drv_Crystalfontz.c,v 1.6 2004/01/29 04:40:02 reinelt Exp $
 *
 * new style driver for Crystalfontz display modules
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_Crystalfontz.c,v $
 * Revision 1.6  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.5  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
 * Revision 1.4  2004/01/23 07:04:03  reinelt
 * icons finished!
 *
 * Revision 1.3  2004/01/23 04:53:34  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.2  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.1  2004/01/21 12:36:19  reinelt
 * Crystalfontz NextGeneration driver added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Crystalfontz
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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


static char Name[]="Crystalfontz";

static int Model;
static int Protocol;

// Fixme:
// static int GPO[8];
static int GPOS;


typedef struct {
  int type;
  char *name;
  int rows;
  int cols;
  int gpos;
  int protocol;
} MODEL;

// Fixme #1: number of gpo's should be verified
// Fixme #2: protocol should be verified
// Fixme #3: do CF displays have type numbers?

static MODEL Models[] = {
  { 0x26, "626",      2, 16, 0, 1 },
  { 0x31, "631",      2, 16, 0, 2 },
  { 0x32, "632",      2, 16, 0, 1 },
  { 0x33, "633",      2, 16, 0, 2 },
  { 0x34, "634",      4, 20, 0, 1 },
  { 0x36, "636",      2, 16, 0, 1 },
  { 0xff, "Unknown", -1, -1, 0, 0 }
};


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_CF_goto (int row, int col)
{
  char cmd[3]="\021xy"; // set cursor position

  if (row==0 && col==0) {
    drv_generic_serial_write("\001", 1); // cursor home
  } else {
    cmd[1]=(char)col;
    cmd[2]=(char)row;
    drv_generic_serial_write(cmd, 3);
  }
}


static void drv_CF_defchar (int ascii, char *buffer)
{
  char cmd[2]="\031n"; // set custom char bitmap

  // user-defineable chars start at 128, but are defined at 0
  cmd[1]=(char)(ascii-CHAR0); 
  drv_generic_serial_write (cmd, 2);
  drv_generic_serial_write (buffer, 8);
}


static int drv_CF_start (char *section)
{
  int i;  
  char *model;
  char buffer[17];
  
  model=cfg_get(section, "Model", NULL);
  if (model!=NULL && *model!='\0') {
    for (i=0; Models[i].type!=0xff; i++) {
      if (strcasecmp(Models[i].name, model)==0) break;
    }
    if (Models[i].type==0xff) {
      error ("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
      return -1;
    }
    Model=i;
    info ("%s: using model '%s'", Name, Models[Model].name);
  } else {
    info ("%s: no '%s.Model' entry from %s, auto-dedecting", Name, section, cfg_source());
    Model=-1;
  }
  
  // open serial port
  if (drv_generic_serial_open(section, Name)<0) return -1;

  // MR: why such a large delay?
  usleep(350*1000);

  // read display type
  memset(buffer, 0, sizeof(buffer));
  drv_generic_serial_write ("\1", 2);
  usleep(250*1000);
#if 1
  {
    int len=drv_generic_serial_read (buffer, 16);
    debug ("Michi: len=<%d> buffer=<%s>", len, buffer);
  }
#else
  if (drv_generic_serial_read (buffer, 16)==16) {
    info ("%s: display reports serial number 0x%x", Name, *(short*)buffer);
  }
#endif

  // initialize global variables
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  GPOS     = Models[Model].gpos;
  Protocol = Models[Model].protocol;

  drv_generic_serial_write ("\014", 1);  // Form Feed (Clear Display)
  drv_generic_serial_write ("\004", 1);  // hide cursor
  drv_generic_serial_write ("\024", 1);  // scroll off
  drv_generic_serial_write ("\030", 1);  // wrap off

  return 0;
}


// ****************************************
// ***            plugins               ***
// ****************************************


static void plugin_contrast (RESULT *result, RESULT *arg1)
{
  char buffer[3];
  double contrast;
  
  contrast=R2N(arg1);
  if (contrast<0  ) contrast=0;
  if (contrast>100) contrast=100;

  snprintf (buffer, 3, "\017%c", (int)contrast);
  drv_generic_serial_write (buffer, 2);
  
  SetResult(&result, R_NUMBER, &contrast); 
}


static void plugin_backlight (RESULT *result, RESULT *arg1)
{
  char buffer[3];
  double backlight;

  backlight=R2N(arg1);
  if (backlight<0  ) backlight=0;
  if (backlight>100) backlight=100;

  snprintf (buffer, 4, "\016%c", (int)backlight);
  drv_generic_serial_write (buffer, 3);

  SetResult(&result, R_NUMBER, &backlight); 
}

// Fixme: other plugins for Fans, Temmperature sensors, ...


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
int drv_CF_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_CF_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // display preferences
  XRES  = 6;      // pixel width of one char 
  YRES  = 8;      // pixel height of one char 
  CHARS = 8;      // number of user-defineable characters
  CHAR0 = 128;    // ASCII of first user-defineable char
  GOTO_COST = 3;  // number of bytes a goto command requires

  // real worker functions
  drv_generic_text_real_write   = drv_generic_serial_write;
  drv_generic_text_real_goto    = drv_CF_goto;
  drv_generic_text_real_defchar = drv_CF_defchar;


  // start display
  if ((ret=drv_CF_start (section))!=0)
    return ret;
  
  // initialize generic text driver
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  // initialize generic icon driver
  if ((ret=drv_generic_text_icon_init())!=0)
    return ret;
  
  // initialize generic bar driver
  if ((ret=drv_generic_text_bar_init())!=0)
    return ret;
  
  // add fixed chars to the bar driver
  drv_generic_text_bar_add_segment (0, 0, 255, 32); // ASCII 32 = blank
  
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
  AddFunction ("contrast",  1, plugin_contrast);
  AddFunction ("backlight", 1, plugin_backlight);
  
  return 0;
}


// close driver & display
int drv_CF_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_serial_close();
  drv_generic_text_quit();
  
  return (0);
}


DRIVER drv_Crystalfontz = {
  name: Name,
  list: drv_CF_list,
  init: drv_CF_init,
  quit: drv_CF_quit, 
};

