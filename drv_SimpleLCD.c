/* $Id: drv_SimpleLCD.c,v 1.1 2005/02/24 07:06:48 reinelt Exp $
 * 
 * driver for a simple serial terminal.
 * This driver simply send out caracters on the serial port, without any 
 * formatting instructions for a particular LCD device. 
 * This is useful for custom boards of for very simple LCD.
 *
 * I use it for tests on a custom-made board based on a AVR microcontroler
 * and also for driver a Point-of-Sale text-only display.
 * I assume the following :
 * - CR (0x0d) Return to the begining of the line without erasing,
 * - LF (0x0a) Initiate a new line (but without sending the cursor to 
 * the begining of the line)
 * - BS (0x08) Erase the previous caracter on the line.
 * - It's not possible to return to the first line. Thus a back buffer is used
 * in this driver.
 *
 * The code come mostly taken from the LCDTerm driver in LCD4Linux, from 
 * Michaels Reinelt, many thanks to him.
 *
 * This driver is released under the GPL.
 * 
 * Copyright (C) 2005 Julien Aube <ob@obconseil.net>
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
 * $Log: drv_SimpleLCD.c,v $
 * Revision 1.1  2005/02/24 07:06:48  reinelt
 * SimpleLCD driver added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_SimpleLCD
 *
 */

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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


static char Name[]="SimpleLCD";
static char *backbuffer=0;
static int   backbuffer_size=0;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/** No clear function on SimpleLCD : Just send CR-LF * number of lines **/
static void drv_SL_clear (void)
{
  char cmd[2] = { '\r', '\n' };
  int i;
  for (i=0;i<DROWS;++i) {
    drv_generic_serial_write (cmd, 2);
  }
  memset(backbuffer,' ',backbuffer_size);
}

/* If full_commit = true, then the whole buffer is to be sent to screen.
   if full_commit = false, then only the last line is to be sent (faster on slow screens)
*/
static void drv_SL_commit(int full_commit)
{
  int row;
  char cmd[2] = { '\r', '\n' };
  if (full_commit) {
    for (row=0;row < DROWS; row++) {
       drv_generic_serial_write (cmd, 2);
       drv_generic_serial_write(backbuffer+(DCOLS*row),DCOLS);
    }
  } else {
    drv_generic_serial_write (cmd, 1); /* Go to the beginning of the line only */
    drv_generic_serial_write(backbuffer+(DCOLS*(DROWS-1)),DCOLS);
  }
}

static void drv_SL_write (const int row, const int col, const char *data, int len)
{
  memcpy(backbuffer+(row*DCOLS)+col,data,len);
  if (row == DROWS-1)
     drv_SL_commit(0);
  else
     drv_SL_commit(1);
}



static int drv_SL_start (const char *section, const int quiet)
{
  int rows=-1, cols=-1;
  unsigned int flags=0;
  char *s;
 
  cfg_number(section,"Options",0,0,0xffff,&flags);
 
  if (drv_generic_serial_open(section, Name, flags) < 0) return -1;

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

  backbuffer_size = DROWS * DCOLS;
  backbuffer = malloc(backbuffer_size);
  if ( ! backbuffer ){  
     return -1;
  }
  drv_SL_clear();        /* clear */

  
  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer, NULL)) {
      sleep (3);
      drv_SL_clear();
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


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_SL_list (void)
{
  printf ("generic");
  return 0;
}


/* initialize driver & display */
int drv_SL_init (const char *section, const int quiet)
{
  WIDGET_CLASS wc;
  int ret;  
  
  /* display preferences */
  XRES  = 5;      /* pixel width of one char  */
  YRES  = 8;      /* pixel height of one char  */
  CHARS = 0;      /* number of user-defineable characters */
  CHAR0 = 0;      /* ASCII of first user-defineable char */

  GOTO_COST = -1;  /* number of bytes a goto command requires */
  
  /* real worker functions */
  drv_generic_text_real_write   = drv_SL_write;


  /* start display */
  if ((ret=drv_SL_start (section, quiet))!=0)
    return ret;
  
  /* initialize generic text driver */
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  /* register text widget */
  wc=Widget_Text;
  wc.draw=drv_generic_text_draw;
  widget_register(&wc);
  
  /* register plugins */
  /* none */

  return 0;
}


/* close driver & display */
int drv_SL_quit (const int quiet)
{

  info("%s: shutting down.", Name);
  
  drv_generic_text_quit();
  
  /* clear display */
  drv_SL_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }
  
  drv_generic_serial_close();
  
  if ( backbuffer ) {
    free(backbuffer) ; backbuffer=0; backbuffer_size=0;
  }
  return (0);
}


DRIVER drv_SimpleLCD = {
  name: Name,
  list: drv_SL_list,
  init: drv_SL_init,
  quit: drv_SL_quit, 
};

