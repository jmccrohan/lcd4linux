/* $Id: drv_MatrixOrbital.c,v 1.14 2004/01/20 15:32:49 reinelt Exp $
 *
 * new style driver for Matrix Orbital serial display modules
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
 * $Log: drv_MatrixOrbital.c,v $
 * Revision 1.14  2004/01/20 15:32:49  reinelt
 * first version of Next Generation HD44780 (untested! but it compiles...)
 * some cleanup in the other drivers
 *
 * Revision 1.13  2004/01/20 14:25:12  reinelt
 * some reorganization
 * moved drv_generic to drv_generic_serial
 * moved port locking stuff to drv_generic_serial
 *
 * Revision 1.12  2004/01/20 12:45:47  reinelt
 * "Default screen" working with MatrixOrbital
 *
 * Revision 1.11  2004/01/20 05:36:59  reinelt
 * moved text-display-specific stuff to drv_generic_text
 * moved all the bar stuff from drv_generic_bar to generic_text
 *
 * Revision 1.10  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 * Revision 1.9  2004/01/18 21:25:16  reinelt
 * Framework for bar widget opened
 *
 * Revision 1.8  2004/01/15 07:47:02  reinelt
 * debian/ postinst and watch added (did CVS forget about them?)
 * evaluator: conditional expressions (a?b:c) added
 * text widget nearly finished
 *
 * Revision 1.7  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
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
 * Revision 1.2  2004/01/10 10:20:22  reinelt
 * new MatrixOrbital changes
 *
 * Revision 1.1  2004/01/09 17:03:07  reinelt
 * initiated transfer to new driver architecture
 * new file 'drv.c' will someday replace 'display.c'
 * new file 'drv_MatrixOrbital.c' will replace 'MatrixOrbital.c'
 * due to this 'soft' transfer lcd4linux should stay usable during the switch
 * (at least I hope so)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_MatrixOrbital
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


static char Name[]="MatrixOrbital";

static int Model;

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

static MODEL Models[] = {
  { 0x01, "LCD0821",      2,  8, 0, 1 },
  { 0x03, "LCD2021",      2, 20, 0, 1 },
  { 0x04, "LCD1641",      4, 16, 0, 1 },
  { 0x05, "LCD2041",      4, 20, 0, 1 },
  { 0x06, "LCD4021",      2, 40, 0, 1 },
  { 0x07, "LCD4041",      4, 40, 0, 1 },
  { 0x08, "LK202-25",     2, 20, 0, 2 },
  { 0x09, "LK204-25",     4, 20, 0, 2 },
  { 0x0a, "LK404-55",     4, 40, 0, 2 },
  { 0x0b, "VFD2021",      2, 20, 0, 1 },
  { 0x0c, "VFD2041",      4, 20, 0, 1 },
  { 0x0d, "VFD4021",      2, 40, 0, 1 },
  { 0x0e, "VK202-25",     2, 20, 0, 1 },
  { 0x0f, "VK204-25",     4, 20, 0, 1 },
  { 0x10, "GLC12232",    -1, -1, 0, 1 },
  { 0x13, "GLC24064",    -1, -1, 0, 1 },
  { 0x15, "GLK24064-25", -1, -1, 0, 1 },
  { 0x22, "GLK12232-25", -1, -1, 0, 1 },
  { 0x31, "LK404-AT",     4, 40, 0, 2 },
  { 0x32, "VFD1621",      2, 16, 0, 1 },
  { 0x33, "LK402-12",     2, 40, 0, 2 },
  { 0x34, "LK162-12",     2, 16, 0, 2 },
  { 0x35, "LK204-25PC",   4, 20, 0, 2 },
  { 0x36, "LK202-24-USB", 2, 20, 0, 2 },
  { 0x38, "LK204-24-USB", 4, 20, 0, 2 },
  { 0xff, "Unknown",     -1, -1, 0, 0 }
};


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_MO_define_char (int ascii, char *buffer)
{
  char cmd[3]="\376N";

  cmd[2]=(char)ascii;
  drv_generic_serial_write (cmd, 3);
  drv_generic_serial_write (buffer, 8);
}


static void drv_MO_goto (int row, int col)
{
  char cmd[5]="\376Gyx";
  cmd[2]=(char)col+1;
  cmd[3]=(char)row+1;
  drv_generic_serial_write(cmd,4);
}


static int drv_MO_start (char *section)
{
  int i;  
  char *model;
  char buffer[256];
  
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
  
  
  if (drv_generic_serial_open(section, Name)<0) return -1;

  // read module type
  drv_generic_serial_write ("\3767", 2);
  usleep(1000);
  if (drv_generic_serial_read (buffer, 1)==1) {
    for (i=0; Models[i].type!=0xff; i++) {
      if (Models[i].type == (int)*buffer) break;
    }
    info ("%s: display reports model '%s' (type 0x%02x)", 
	  Name, Models[i].name, Models[i].type);

    // auto-dedection
    if (Model==-1) Model=i;

    // auto-dedection matches specified model?
    if (Models[i].type!=0xff && Model!=i) {
      error ("%s: %s.Model '%s' from %s does not match dedected Model '%s'", 
	     Name, section, model, cfg_source(), Models[i].name);
      return -1;
    }

  } else {
    info ("%s: display detection failed.", Name);
  }
  
  // read serial number
  drv_generic_serial_write ("\3765", 2);
  usleep(100000);
  if (drv_generic_serial_read (buffer, 2)==2) {
    info ("%s: display reports serial number 0x%x", Name, *(short*)buffer);
  }

  // read version number
  drv_generic_serial_write ("\3766", 2);
  usleep(100000);
  if (drv_generic_serial_read (buffer, 1)==1) {
    info ("%s: display reports firmware version 0x%x", Name, *buffer);
  }
  
  // initialize global variables
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  GPOS     = Models[Model].gpos;
  PROTOCOL = Models[Model].protocol;

  if (PROTOCOL==2) 
    drv_generic_serial_write ("\376\130", 2);  // Clear Screen
  else 
    drv_generic_serial_write ("\014", 1);  // Clear Screen
  
  drv_generic_serial_write ("\376B", 3);  // backlight on
  drv_generic_serial_write ("\376K", 2);  // cursor off
  drv_generic_serial_write ("\376T", 2);  // blink off
  drv_generic_serial_write ("\376D", 2);  // line wrapping off
  drv_generic_serial_write ("\376R", 2);  // auto scroll off

  return 0;
}


// ****************************************
// ***            plugins               ***
// ****************************************


static void plugin_contrast (RESULT *result, RESULT *arg1)
{
  char buffer[4];
  double contrast;
  
  contrast=R2N(arg1);
  if (contrast<0  ) contrast=0;
  if (contrast>255) contrast=255;
  snprintf (buffer, 4, "\376P%c", (int)contrast);
  drv_generic_serial_write (buffer, 3);
  
  SetResult(&result, R_NUMBER, &contrast); 
}


static void plugin_backlight (RESULT *result, RESULT *arg1)
{
  char buffer[4];
  double backlight;

  backlight=R2N(arg1);
  if (backlight<-1  ) backlight=-1;
  if (backlight>255) backlight=255;
  if (backlight<0) {
    // backlight off
    snprintf (buffer, 3, "\376F");
    drv_generic_serial_write (buffer, 2);
  } else {
    // backlight on for n minutes
    snprintf (buffer, 4, "\376B%c", (int)backlight);
    drv_generic_serial_write (buffer, 3);
  }
  SetResult(&result, R_NUMBER, &backlight); 
}


static void plugin_gpo (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  int num;
  double val;
  char cmd[3]="\376";
  
  num=R2N(arg1);
  val=R2N(arg2);
  
  if (num<1) num=1;
  if (num>6) num=6;
  
  if (val>=1.0) {
    val=1.0;
  } else {
    val=0.0;
  }
  
  switch (PROTOCOL) {
  case 1:
    if (num==0) {
      if (val>=1.0) {
	drv_generic_serial_write ("\376W", 2);  // GPO on
      } else {
	drv_generic_serial_write ("\376V", 2);  // GPO off
      }
    } else {
      error("Fixme");
      val=-1.0;
    }
    break;
    
  case 2:
    if (val>=1.0) {
      cmd[1]='W';  // GPO on
    } else {
      cmd[1]='V';  // GPO off
    }
    cmd[2]=(char)num;
    drv_generic_serial_write (cmd, 3);
    break;
  }
  
  SetResult(&result, R_NUMBER, &val); 
}


static void plugin_pwm (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  int    num;
  double val;
  char   cmd[4]="\376\300";
  
  num=R2N(arg1);
  if (num<1) num=1;
  if (num>6) num=6;
  cmd[2]=(char)num;
  
  val=R2N(arg2);
  if (val<  0.0) val=  0.0;
  if (val>255.0) val=255.0;
  cmd[3]=(char)val;

  drv_generic_serial_write (cmd, 4);

  SetResult(&result, R_NUMBER, &val); 
}


static void plugin_rpm (RESULT *result, RESULT *arg1)
{
  int    num;
  double val;
  char   cmd[3]="\376\301";
  char   buffer[7];
  
  num=R2N(arg1);
  if (num<1) num=1;
  if (num>6) num=6;
  cmd[2]=(char)num;
  
  drv_generic_serial_write (cmd, 3);
  usleep(100000);
  drv_generic_serial_read (buffer, 7);
  
  debug ("rpm: buffer[0]=0x%01x", buffer[0]);
  debug ("rpm: buffer[1]=0x%01x", buffer[1]);
  debug ("rpm: buffer[2]=0x%01x", buffer[2]);
  debug ("rpm: buffer[3]=0x%01x", buffer[3]);
  debug ("rpm: buffer[4]=0x%01x", buffer[4]);
  debug ("rpm: buffer[5]=0x%01x", buffer[5]);
  debug ("rpm: buffer[6]=0x%01x", buffer[6]);

  SetResult(&result, R_NUMBER, &val); 
}


// ****************************************
// ***        widget callbacks          ***
// ****************************************


int drv_MO_draw_text (WIDGET *W)
{
  return drv_generic_text_draw_text(W, 5, drv_MO_goto, drv_generic_serial_write);
}


int drv_MO_draw_bar (WIDGET *W)
{
  return drv_generic_text_draw_bar(W, 5, drv_MO_define_char, drv_MO_goto, drv_generic_serial_write);
}


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_MO_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_MO_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  XRES=5;
  YRES=8;
  CHARS=8;
  
  // start display
  if ((ret=drv_MO_start (section))!=0)
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
  wc.draw=drv_MO_draw_text;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_MO_draw_bar;
  widget_register(&wc);
  
  // register plugins
  AddFunction ("contrast",  1, plugin_contrast);
  AddFunction ("backlight", 1, plugin_backlight);
  AddFunction ("gpo",       2, plugin_gpo);
  AddFunction ("pwm",       2, plugin_pwm);
  AddFunction ("rpm",       1, plugin_rpm);
  
  return 0;
}


// close driver & display
int drv_MO_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_serial_close();
  drv_generic_text_quit();
  
  return (0);
}


DRIVER drv_MatrixOrbital = {
  name: Name,
  list: drv_MO_list,
  init: drv_MO_init,
  quit: drv_MO_quit, 
};

