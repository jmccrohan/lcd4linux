/* $Id: drv_LCDLinux.c,v 1.1 2005/01/22 22:57:57 reinelt Exp $
 *
 * driver for the LCD-Linux HD44780 kernel driver
 * http://lcd-linux.sourceforge.net
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
 * $Log: drv_LCDLinux.c,v $
 * Revision 1.1  2005/01/22 22:57:57  reinelt
 * LCD-Linux driver added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_LCDLinux
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
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

static char Name[]="LCD-Linux";


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_LL_clear (void)
{
  char cmd[1];
  
  /* Fixme */
#if 0
  cmd[0] = LCD_CLEAR; /* clear display */
  drv_generic_serial_write (cmd, 1);  /* clear screen */
#endif

}


static int drv_LL_send (const char request, const char value)
{
  char buf[2];

  buf[0] = request;
  buf[1] = value;

  // Fixme
  // drv_generic_serial_write (buf, 2);
  
  return 0;
}


static void drv_LL_command (const char cmd)
{
  // Fixme
  // drv_LL_send (LCD_CMD, cmd);
}


static void drv_LL_write (const int row, const int col, const char *data, int len)
{
  int pos;
  
  /* 16x4 Displays use a slightly different layout */
  if (DCOLS==16 && DROWS==4) {
    pos = (row%2)*64+(row/2)*16+col;
  } else {  
    pos = (row%2)*64+(row/2)*20+col;
  }

  drv_LL_command (0x80|pos);
  
  while (len--) {
    // Fixme
    // drv_LL_send (LCD_DATA, *data++);
  }
}

static void drv_LL_defchar (const int ascii, const unsigned char *matrix)
{
  int i;
  
  drv_LL_command (0x40|8*ascii);

  for (i = 0; i < 8; i++) {
    // Fixme
    // drv_LL_send (LCD_DATA, *matrix++ & 0x1f);
  }
}


static int drv_LL_start (const char *section, const int quiet)
{
  int rows=-1, cols=-1;
  char *s;

  /* Fixme: open device */

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
  drv_LL_command (0x29); /* 8 Bit mode, 1/16 duty cycle, 5x8 font */
  drv_LL_command (0x08); /* Display off, cursor off, blink off */
  drv_LL_command (0x0c); /* Display on, cursor off, blink off */
  drv_LL_command (0x06); /* curser moves to right, no shift */

  drv_LL_clear();        /* clear display */
  
  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer, "www.bwct.de")) {
      sleep (3);
      drv_LL_clear();
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
int drv_LL_list (void)
{
  printf ("generic");
  return 0;
}


/* initialize driver & display */
int drv_LL_init (const char *section, const int quiet)
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
  drv_generic_text_real_write   = drv_LL_write;
  drv_generic_text_real_defchar = drv_LL_defchar;


  /* start display */
  if ((ret=drv_LL_start (section, quiet))!=0)
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
int drv_LL_quit (const int quiet)
{

  info("%s: shutting down.", Name);
  
  drv_generic_text_quit();
  
  /* clear display */
  drv_LL_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }
  
  // Fixme: close device
  
  return (0);
}


DRIVER drv_LCDLinux = {
  name: Name,
  list: drv_LL_list,
  init: drv_LL_init,
  quit: drv_LL_quit, 
};


