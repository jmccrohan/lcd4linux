/* $Id: drv_LCDTerm.c,v 1.2 2005/01/18 06:30:23 reinelt Exp $
 *
 * driver for the LCDTerm serial-to-HD44780 adapter boards
 * http://www.bobblick.com/techref/projects/lcdterm/lcdterm.html
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_LCDTerm.c,v $
 * Revision 1.2  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.1  2005/01/15 13:13:57  reinelt
 * LCDTerm driver added, take 2
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_LCDTerm
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

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

#define LCD_CLEAR 0x03    
#define LCD_CMD   0x12
#define LCD_DATA  0x14

static char Name[]="LCDTerm";


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_LT_clear (void)
{
  char cmd[1];
  
  cmd[0] = LCD_CLEAR; /* clear display */
  drv_generic_serial_write (cmd, 1);  /* clear screen */
}


static int drv_LT_send (const char request, const char value)
{
  char buf[2];

  buf[0] = request;
  buf[1] = value;
  drv_generic_serial_write (buf, 2);
  
  return 0;
}


static void drv_LT_command (const char cmd)
{
  drv_LT_send (LCD_CMD, cmd);
}


static void drv_LT_write (const int row, const int col, const char *data, int len)
{
  int pos;
  
  /* 16x4 Displays use a slightly different layout */
  if (DCOLS==16 && DROWS==4) {
    pos = (row%2)*64+(row/2)*16+col;
  } else {  
    pos = (row%2)*64+(row/2)*20+col;
  }

  drv_LT_command (0x80|pos);
  
  while (len--) {
    drv_LT_send (LCD_DATA, *data++);
  }
}

static void drv_LT_defchar (const int ascii, const unsigned char *matrix)
{
  int i;
  
  drv_LT_command (0x40|8*ascii);

  for (i = 0; i < 8; i++) {
    drv_LT_send (LCD_DATA, *matrix++ & 0x1f);
  }
}


static int drv_LT_start (const char *section, const int quiet)
{
  int rows=-1, cols=-1;
  char *s;

  if (drv_generic_serial_open(section, Name, 0) < 0) return -1;

  s=cfg_get(section, "Size", NULL);
  if (s==NULL || *s=='\0') {
    error ("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
    free (s);
    return -1;
  }
  
  DROWS = rows;
  DCOLS = cols;
  
  /* initialize display */
  drv_LT_command (0x29); /* 8 Bit mode, 1/16 duty cycle, 5x8 font */
  drv_LT_command (0x08); /* Display off, cursor off, blink off */
  drv_LT_command (0x0c); /* Display on, cursor off, blink off */
  drv_LT_command (0x06); /* curser moves to right, no shift */

  drv_LT_clear();        /* clear display */
  
  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer, "www.bwct.de")) {
      sleep (3);
      drv_LT_clear();
    }
  }
  
  return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none */


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
int drv_LT_list (void)
{
  printf ("generic");
  return 0;
}


/* initialize driver & display */
int drv_LT_init (const char *section, const int quiet)
{
  WIDGET_CLASS wc;
  int asc255bug;
  int ret;  
  
  /* display preferences */
  XRES  = 5;      /* pixel width of one char  */
  YRES  = 8;      /* pixel height of one char  */
  CHARS = 8;      /* number of user-defineable characters */
  CHAR0 = 0;      /* ASCII of first user-defineable char */

  /* Fixme: */
  GOTO_COST = 2;  /* number of bytes a goto command requires */
  
  /* real worker functions */
  drv_generic_text_real_write   = drv_LT_write;
  drv_generic_text_real_defchar = drv_LT_defchar;


  /* start display */
  if ((ret=drv_LT_start (section, quiet))!=0)
    return ret;
  
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
  /* most displays have a full block on ascii 255, but some have kind of  */
  /* an 'inverted P'. If you specify 'asc255bug 1 in the config, this */
  /* char will not be used, but rendered by the bar driver */
  cfg_number(section, "asc255bug", 0, 0, 1, &asc255bug);
  drv_generic_text_bar_add_segment (  0,  0,255, 32); /* ASCII  32 = blank */
  if (!asc255bug) 
    drv_generic_text_bar_add_segment (255,255,255,255); /* ASCII 255 = block */
  
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
  /* none */

  return 0;
}


/* close driver & display */
int drv_LT_quit (const int quiet)
{

  info("%s: shutting down.", Name);
  
  drv_generic_text_quit();
  
  /* clear display */
  drv_LT_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }
  
  drv_generic_serial_close();
  
  return (0);
}


DRIVER drv_LCDTerm = {
  name: Name,
  list: drv_LT_list,
  init: drv_LT_init,
  quit: drv_LT_quit, 
};


