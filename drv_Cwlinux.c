/* $Id: drv_Cwlinux.c,v 1.21 2005/01/18 06:30:22 reinelt Exp $
 *
 * new style driver for Cwlinux display modules
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_Cwlinux.c,v $
 * Revision 1.21  2005/01/18 06:30:22  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.20  2004/11/28 15:50:24  reinelt
 * Cwlinux fixes (invalidation of user-defined chars)
 *
 * Revision 1.19  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.18  2004/06/26 09:27:20  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.17  2004/06/20 10:09:54  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.16  2004/06/06 06:51:59  reinelt
 *
 * do not display end splash screen if quiet=1
 *
 * Revision 1.15  2004/06/05 14:56:48  reinelt
 *
 * Cwlinux splash screen fixed
 * USBLCD splash screen fixed
 * plugin_i2c qprintf("%f") replaced with snprintf()
 *
 * Revision 1.14  2004/06/05 06:41:39  reinelt
 *
 * chancged splash screen again
 *
 * Revision 1.13  2004/06/05 06:13:11  reinelt
 *
 * splash screen for all text-based display drivers
 *
 * Revision 1.12  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.11  2004/06/01 06:45:29  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.10  2004/05/31 21:05:13  reinelt
 *
 * fixed lots of bugs in the Cwlinux driver
 * do not emit EAGAIN error on the first retry
 * made plugin_i2c_sensors a bit less 'chatty'
 * moved init and exit functions to the bottom of plugin_pop3
 *
 * Revision 1.9  2004/05/31 16:39:06  reinelt
 *
 * added NULL display driver (for debugging/profiling purposes)
 * added backlight/contrast initialisation for matrixOrbital
 * added Backlight initialisation for Cwlinux
 *
 * Revision 1.8  2004/05/31 05:38:02  reinelt
 *
 * fixed possible bugs with user-defined chars (clear high bits)
 * thanks to Andy Baxter for debugging the MilfordInstruments driver!
 *
 * Revision 1.7  2004/05/28 13:51:42  reinelt
 *
 * ported driver for Beckmann+Egle Mini-Terminals
 * added 'flags' parameter to serial_init()
 *
 * Revision 1.6  2004/05/27 03:39:47  reinelt
 *
 * changed function naming scheme to plugin::function
 *
 * Revision 1.5  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.4  2004/03/19 09:17:46  reinelt
 *
 * removed the extra 'goto' function, row and col are additional parameters
 * of the write() function now.
 *
 * Revision 1.3  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.2  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.1  2004/01/27 06:34:14  reinelt
 * Cwlinux driver portet to NextGeneration (compiles, but not tested!)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Cwlinux
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_serial.h"


static char Name[]="Cwlinux";

static int Model;
static int Protocol;

/* Fixme: GPO's not yet implemented */
/* static int GPO[8]; */
static int GPOS;


typedef struct {
  int type;
  char *name;
  int rows;
  int cols;
  int xres;
  int gpos;
  int protocol;
} MODEL;


/* Fixme: number of gpo's should be verified */

static MODEL Models[] = {
  { 0x01, "CW1602",   2, 16,  5,  0,  1 },
  { 0x02, "CW12232",  4, 20,  6,  0,  2 },
  { 0xff, "Unknown", -1, -1, -1, -1, -1 }
};


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_CW_send (const char *string, const int len)
{
  drv_generic_serial_write (string, len);
  usleep (20);
}


static void drv_CW_write (const int row, const int col, const char *data, const int len)
{
  char cmd[6]="\376Gxy\375";
  
  cmd[2]=(char)col;
  cmd[3]=(char)row;
  drv_CW_send (cmd, 5);
  drv_CW_send (data, len);
}


static void drv_CW1602_defchar (const int ascii, const unsigned char *buffer)
{
  int i;
  char cmd[12]="\376Nn12345678\375";

  cmd[2]=(char)ascii;

  for (i=0; i<8; i++) {
    cmd[3+i] = buffer[i] & 0x1f;
  }
  drv_CW_send(cmd,12);
}


static void drv_CW12232_defchar (const int ascii, const unsigned char *buffer)
{
  int i, j;
  char cmd[10]="\376Nn123456\375";
  
  cmd[2]=(char)ascii;
  
  /* The CW12232 uses a vertical bitmap layout, */
  /* so we have to 'rotate' the bitmap. */
  
  for (i=0; i<6;i++) {
    cmd[3+i]=0;
    for (j=0; j<8;j++) {
      if (buffer[j] & (1<<(5-i))) {
	cmd[3+i]|=(1<<j);
      }
    }
  }
  drv_CW_send (cmd, 10);
}


static void drv_CW_clear (void)
{
#if 1
  drv_CW_send("\376X\375",3); /* Clear Display */
  usleep(500000);
#else
  /* for some mysterious reason, we have to sleep after  */
  /* the command _and_ after the CMD_END... */
  drv_CW_send("\376X",2); /* Clear Display */
  drv_CW_send("\375",1);  /* Command End */
#endif
}


static int drv_CW_brightness (int brightness)
{
  static unsigned char Brightness = 0;
  char cmd[5] = "\376A_\375";

  /* -1 is used to query the current brightness */
  if (brightness == -1) return Brightness;
  
  if (brightness < 0  ) brightness = 0;
  if (brightness > 255) brightness = 255;
  Brightness = brightness;
  
  switch (Brightness) {
  case 0:
    /* backlight off */
    drv_CW_send ("\376F\375", 3);
    break;
  case 8:
    /* backlight on */
    drv_CW_send ("\376B\375", 3);
    break;
  default:
    /* backlight level */
    cmd[2] = (char)Brightness;
    drv_CW_send (cmd, 4);
    break;
  }

  return Brightness;
}


static int drv_CW_start (const char *section)
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
  
  /* open serial port */
  if (drv_generic_serial_open(section, Name, 0) < 0) return -1;

  /* this does not work as I'd expect it... */
#if 0
  /* read firmware version */
  generic_serial_read(buffer,sizeof(buffer));
  usleep(100000);
  generic_serial_write ("\3761", 2);
  usleep(100000);
  generic_serial_write ("\375", 1);
  usleep(100000);
  if (generic_serial_read(buffer,2)!=2) {
    info ("unable to read firmware version!");
  }
  info ("Cwlinux Firmware %d.%d", (int)buffer[0], (int)buffer[1]);
#endif

  /* initialize global variables */
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  XRES     = Models[Model].xres;
  GPOS     = Models[Model].gpos;
  Protocol = Models[Model].protocol;

  drv_CW_clear();

  drv_CW_send ("\376D\375", 3); /* auto line wrap off */
  drv_CW_send ("\376R\375", 3); /* auto scroll off */
  drv_CW_send ("\376K\375", 3); /* underline cursor off */
  drv_CW_send ("\376B\375", 3); /* backlight on */

  /* set brightness */
  if (cfg_number(section, "Brightness", 0, 0, 8, &i) > 0) {
    drv_CW_brightness(i);
  }

  return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/


static void plugin_brightness (RESULT *result, const int argc, RESULT *argv[])
{
  double brightness;
  
  switch (argc) {
  case 0:
    brightness = drv_CW_brightness(-1);
    SetResult(&result, R_NUMBER, &brightness); 
    break;
  case 1:
    brightness = drv_CW_brightness(R2N(argv[0]));
    SetResult(&result, R_NUMBER, &brightness); 
    break;
  default:
    error ("%s.brightness(): wrong number of parameters", Name);
    SetResult(&result, R_STRING, ""); 
  }
}


/****************************************/
/***        widget callbacks          ***/
/****************************************/

/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_CW_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


/* initialize driver & display */
int drv_CW_init (const char *section, const int quiet)
{
  WIDGET_CLASS wc;
  int ret;  
  
  /* display preferences */
  XRES  = 6;      /* pixel width of one char  */
  YRES  = 8;      /* pixel height of one char  */
  CHARS = 16;     /* number of user-defineable characters */
  CHAR0 = 1;      /* ASCII of first user-defineable char */
  GOTO_COST  = 3; /* number of bytes a goto command requires */
  INVALIDATE = 1; /* re-defined chars must be re-sent to the display */

  /* start display */
  if ((ret=drv_CW_start (section))!=0)
    return ret;
  
  /* real worker functions */
  drv_generic_text_real_write = drv_CW_write;

  switch (Protocol) {
    case 1:
      drv_generic_text_real_defchar = drv_CW1602_defchar;
      break;
    case 2:
      drv_generic_text_real_defchar = drv_CW12232_defchar;
      break;
  }
  
  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %s", Name, Models[Model].name);
    if (drv_generic_text_greet (buffer, "www.cwlinux.com")) {
      sleep (3);
      drv_CW_clear();
    }
  }

  /* initialize generic text driver */
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  /* initialize generic icon driver */
  if ((ret=drv_generic_text_icon_init())!=0)
    return ret;
  
  /* initialize generic bar driver */
  if ((ret=drv_generic_text_bar_init(0))!=0)
    return ret;
  
  /* add fixed chars to the bar driver */
  drv_generic_text_bar_add_segment (0, 0, 255, 32); /* ASCII 32 = blank */
  
  /* register text widget */
  wc=Widget_Text;
  wc.draw=drv_generic_text_draw;
  widget_register(&wc);
  
  /* register icon widget */
  wc=Widget_Icon;
  wc.draw=drv_generic_text_icon_draw;
  widget_register(&wc);
  
  /* register bar widget */
  wc=Widget_Bar;
  wc.draw=drv_generic_text_bar_draw;
  widget_register(&wc);
  
  /* register plugins */
  AddFunction ("LCD::brightness", -1, plugin_brightness);
  
  return 0;
}


/* close driver & display */
int drv_CW_quit (const int quiet) {

  info("%s: shutting down.", Name);
  drv_generic_text_quit();

  /* clear display */
  drv_CW_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }

  drv_generic_serial_close();
  
  return (0);
}


DRIVER drv_Cwlinux = {
  name: Name,
  list: drv_CW_list,
  init: drv_CW_init,
  quit: drv_CW_quit, 
};

