/* $Id: drv_MatrixOrbital.c,v 1.28 2004/06/01 06:45:29 reinelt Exp $
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
 * Revision 1.28  2004/06/01 06:45:29  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.27  2004/05/31 21:23:16  reinelt
 *
 * some cleanups in the MatrixOrbital driver
 *
 * Revision 1.26  2004/05/31 16:39:06  reinelt
 *
 * added NULL display driver (for debugging/profiling purposes)
 * added backlight/contrast initialisation for matrixOrbital
 * added Backlight initialisation for Cwlinux
 *
 * Revision 1.25  2004/05/31 05:38:02  reinelt
 *
 * fixed possible bugs with user-defined chars (clear high bits)
 * thanks to Andy Baxter for debugging the MilfordInstruments driver!
 *
 * Revision 1.24  2004/05/28 13:51:42  reinelt
 *
 * ported driver for Beckmann+Egle Mini-Terminals
 * added 'flags' parameter to serial_init()
 *
 * Revision 1.23  2004/05/27 03:39:47  reinelt
 *
 * changed function naming scheme to plugin::function
 *
 * Revision 1.22  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.21  2004/03/19 09:17:46  reinelt
 *
 * removed the extra 'goto' function, row and col are additional parameters
 * of the write() function now.
 *
 * Revision 1.20  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.19  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.18  2004/01/23 07:04:22  reinelt
 * icons finished!
 *
 * Revision 1.17  2004/01/23 04:53:50  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.16  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.15  2004/01/21 12:36:19  reinelt
 * Crystalfontz NextGeneration driver added
 *
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


static char Name[]="MatrixOrbital";

static int Model;
static int Protocol;

// Fixme: GPO's not yet implemented
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

static void drv_MO_write (int row, int col, unsigned char *data, int len)
{
  char cmd[5]="\376Gyx";

  cmd[2]=(char)col+1;
  cmd[3]=(char)row+1;
  drv_generic_serial_write(cmd,4);

  drv_generic_serial_write (data, len);
}


static void drv_MO_defchar (int ascii, unsigned char *matrix)
{
  int i;
  char cmd[11]="\376N";

  cmd[2]=(char)ascii;
  for (i = 0; i < 8; i++) {
    cmd[i+3] = matrix[i] & 0x1f;
  }
  drv_generic_serial_write (cmd, 11);
}


static int drv_MO_contrast (int contrast)
{
  static unsigned char Contrast=0;
  char buffer[4];

  // -1 is used to query the current contrast
  if (contrast == -1) return Contrast;

  if (contrast < 0  ) contrast = 0;
  if (contrast > 255) contrast = 255;
  Contrast = contrast;

  snprintf (buffer, 4, "\376P%c", Contrast);
  drv_generic_serial_write (buffer, 3);
  
  return Contrast;
}


static int drv_MO_backlight (int backlight)
{
  static unsigned char Backlight=0;
  char buffer[4];

  // -1 is used to query the current backlight
  if (backlight == -1) return Backlight;

  if (backlight < 0  ) backlight = 0;
  if (backlight > 255) backlight = 255;
  Backlight = backlight;

  if (backlight<0) {
    // backlight off
    snprintf (buffer, 3, "\376F");
    drv_generic_serial_write (buffer, 2);
  } else {
    // backlight on for n minutes
    snprintf (buffer, 4, "\376B%c", (int)backlight);
    drv_generic_serial_write (buffer, 3);
  }

  return Backlight;
}


static int drv_MO_gpo (int num, int val)
{
  static int GPO[6] = { -1, -1, -1, -1, -1, -1 };
  char cmd[3]="\376";
  
  if (num < 1) num = 1;
  if (num > 6) num = 6;

  // -1 is used to query the current PWM
  if (val == -1) return GPO[num-1];

  if (val < 0) val = 0;
  if (val > 1) val = 1;
  GPO[num-1] = val;

  switch (Protocol) {
  case 1:
    if (num == 1) {
      if (val > 0) {
	drv_generic_serial_write ("\376W", 2);  // GPO on
      } else {
	drv_generic_serial_write ("\376V", 2);  // GPO off
      }
    } else {
      GPO[num-1] = -1;
    }
    break;
    
  case 2:
    if (val > 0) {
      cmd[1] = 'W';  // GPO on
    } else {
      cmd[1] = 'V';  // GPO off
    }
    cmd[2] = (char)num;
    drv_generic_serial_write (cmd, 3);
    break;
  }
  
  return GPO[num-1];
}


static int drv_MO_pwm (int num, int val)
{
  static int PWM[6] = { -1, -1, -1, -1, -1, -1 };
  char   cmd[4]="\376\300";
  
  if (num < 1) num = 1;
  if (num > 6) num = 6;
  
  // -1 is used to query the current PWM
  if (val == -1) return PWM[num-1];

  if (val <   0) val =   0;
  if (val > 255) val = 255;
  PWM[num-1] = val;

  cmd[2] = (char)num;
  cmd[3] = (char)val;
  drv_generic_serial_write (cmd, 4);

  return val;
}


static int drv_MO_rpm (int num)
{
  static int RPM[6] = { -1, -1, -1, -1, -1, -1 };
  char   cmd[3] = "\376\301";
  char   buffer[7];
  
  if (num < 1) num = 1;
  if (num > 6) num = 6;
  
  cmd[2] = (char)num;
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

  // Fixme: RPM calculations???
  RPM[num-1] = 42;

  return RPM[num-1];
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
  
  
  if (drv_generic_serial_open(section, Name, 0)<0) return -1;

  if (Model == -1 || Models[Model].protocol > 1) {
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
  }

  // initialize global variables
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  GPOS     = Models[Model].gpos;
  Protocol = Models[Model].protocol;

  if (Protocol > 1) {
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
  }
  
  switch (Protocol) {
  case 1:
    drv_generic_serial_write ("\014", 1);  // Clear Screen
    break;
  case 2:
    drv_generic_serial_write ("\376\130", 2);  // Clear Screen
    break;
  }
  
  drv_generic_serial_write ("\376B", 3);  // backlight on
  drv_generic_serial_write ("\376K", 2);  // cursor off
  drv_generic_serial_write ("\376T", 2);  // blink off
  drv_generic_serial_write ("\376D", 2);  // line wrapping off
  drv_generic_serial_write ("\376R", 2);  // auto scroll off

  // set contrast
  if (cfg_number(section, "Contrast", 0, 0, 255, &i)>0) {
    drv_MO_contrast(i);
  }

  // set backlight
  if (cfg_number(section, "Backlight", 0, 0, 255, &i)>0) {
    drv_MO_backlight(i);
  }

  return 0;
}


// ****************************************
// ***            plugins               ***
// ****************************************


static void plugin_contrast (RESULT *result, int argc, RESULT *argv[])
{
  double contrast;
  
  switch (argc) {
  case 0:
    contrast = drv_MO_contrast(-1);
    SetResult(&result, R_NUMBER, &contrast); 
    break;
  case 1:
    contrast = drv_MO_contrast(R2N(argv[0]));
    SetResult(&result, R_NUMBER, &contrast); 
    break;
  default:
    error ("%s::contrast(): wrong number of parameters", Name);
    SetResult(&result, R_STRING, ""); 
  }
}


static void plugin_backlight (RESULT *result, int argc, RESULT *argv[])
{
  double backlight;
  
  switch (argc) {
  case 0:
    backlight = drv_MO_backlight(-1);
    SetResult(&result, R_NUMBER, &backlight); 
    break;
  case 1:
    backlight = drv_MO_backlight(R2N(argv[0]));
    SetResult(&result, R_NUMBER, &backlight); 
    break;
  default:
    error ("%s::backlight(): wrong number of parameters");
    SetResult(&result, R_STRING, ""); 
  }
}


static void plugin_gpo (RESULT *result, int argc, RESULT *argv[])
{
  double gpo;
  
  switch (argc) {
  case 1:
    gpo = drv_MO_gpo(R2N(argv[0]), -1);
    SetResult(&result, R_NUMBER, &gpo); 
    break;
  case 2:
    gpo = drv_MO_gpo(R2N(argv[0]), R2N(argv[1]));
    SetResult(&result, R_NUMBER, &gpo); 
    break;
  default:
    error ("%s:gpo(): wrong number of parameters");
    SetResult(&result, R_STRING, ""); 
  }
}


static void plugin_pwm (RESULT *result, int argc, RESULT *argv[])
{
  double pwm;
  
  switch (argc) {
  case 1:
    pwm = drv_MO_pwm(R2N(argv[0]), -1);
    SetResult(&result, R_NUMBER, &pwm); 
    break;
  case 2:
    pwm = drv_MO_pwm(R2N(argv[0]), R2N(argv[1]));
    SetResult(&result, R_NUMBER, &pwm); 
    break;
  default:
    error ("%s:pwm(): wrong number of parameters");
    SetResult(&result, R_STRING, ""); 
  }
}


static void plugin_rpm (RESULT *result, RESULT *arg1)
{
  double rpm;

  rpm = drv_MO_rpm(R2N(arg1));
  SetResult(&result, R_NUMBER, &rpm); 
}


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
  
  // display preferences
  XRES=5;      // pixel width of one char 
  YRES=8;      // pixel height of one char 
  CHARS=8;     // number of user-defineable characters
  CHAR0=0;     // ASCII of first user-defineable char
  GOTO_COST=4; // number of bytes a goto command requires
  
  // real worker functions
  drv_generic_text_real_write   = drv_MO_write;
  drv_generic_text_real_defchar = drv_MO_defchar;


  // start display
  if ((ret=drv_MO_start (section))!=0)
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
  AddFunction ("LCD::contrast",  -1, plugin_contrast);
  AddFunction ("LCD::backlight", -1, plugin_backlight);
  AddFunction ("LCD::gpo",       -1, plugin_gpo);
  AddFunction ("LCD::pwm",       -1, plugin_pwm);
  AddFunction ("LCD::rpm",        1, plugin_rpm);
  
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

