/* $Id: drv_Crystalfontz.c,v 1.1 2004/01/21 12:36:19 reinelt Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_serial.h"


static char Name[]="Crystalfontz";

static int Model;

// Fixme: do we need PROTOCOL?
static int GPOS, ICONS, PROTOCOL;

// Fixme:
// static int GPO[8];


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
  { 0x32, "632",      2, 16, 0, 1 },
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


static void drv_CF_define_char (int ascii, char *buffer)
{
  char cmd[2]="\031n"; // set custom char bitmap

  cmd[1]=(char)ascii;
  drv_generic_serial_write (cmd, 2);
  drv_generic_serial_write (buffer, 8);
}


static int drv_CF_start (char *section)
{
  int i;  
  char *model;
  
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
    error ("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
    return -1;
  }
  
  // initialize global variables
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  GPOS     = Models[Model].gpos;
  PROTOCOL = Models[Model].protocol;

  // open serial port
  if (drv_generic_serial_open(section, Name)<0) return -1;

  // MR: why such a large delay?
  usleep(350*1000);

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


int drv_CF_draw_text (WIDGET *W)
{
  return drv_generic_text_draw_text(W, 4, drv_CF_goto, drv_generic_serial_write);
}


int drv_CF_draw_bar (WIDGET *W)
{
  return drv_generic_text_draw_bar(W, 4, drv_CF_define_char, drv_CF_goto, drv_generic_serial_write);
}


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
  
  XRES=5;
  YRES=8;
  CHARS=8;
  
  // start display
  if ((ret=drv_CF_start (section))!=0)
    return ret;
  
  // initialize generic text driver
  if ((ret=drv_generic_text_init(Name))!=0)
    return ret;

  // initialize generic bar driver
  if ((ret=drv_generic_text_bar_init())!=0)
    return ret;
  
  // add fixed chars to the bar driver
  drv_generic_text_bar_add_segment (  0,  0,255, 32); // ASCII  32 = blank
  drv_generic_text_bar_add_segment (255,255,255,255); // ASCII 255 = block
  
  // register text widget
  wc=Widget_Text;
  wc.draw=drv_CF_draw_text;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_CF_draw_bar;
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

