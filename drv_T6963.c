/* $Id: drv_T6963.c,v 1.1 2004/02/15 21:43:43 reinelt Exp $
 *
 * new style driver for T6963-based displays
 *
 * Copyright 1999-2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: drv_T6963.c,v $
 * Revision 1.1  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_T6963
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
#include <sys/time.h>

#include "debug.h"
#include "cfg.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_graphic.h"
#include "drv_generic_parport.h"

static char Name[]="T6963";
static int Model;

typedef struct {
  int type;
  char *name;
} MODEL;

static MODEL Models[] = {
  { 0x01, "generic" },
  { 0xff, "Unknown" }
};


static unsigned char SIGNAL_CE;
static unsigned char SIGNAL_CD;
static unsigned char SIGNAL_RD;
static unsigned char SIGNAL_WR;

// Fixme:
static int bug=0;


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

// perform normal status check
static void drv_T6_status1 (void)
{
  int n;
  
  // turn off data line drivers
  drv_generic_parport_direction (1);
  
  // lower CE and RD
  drv_generic_parport_control (SIGNAL_CE | SIGNAL_RD, 0);
  
  // Access Time: 150 ns 
  ndelay(150);
  
  // wait for STA0=1 and STA1=1
  n=0;
  do {
    rep_nop();
    if (++n>1000) {
      debug("hang in status1");
      bug=1;
      break;
    }
  } while ((drv_generic_parport_read() & 0x03) != 0x03);
  
  // rise RD and CE
  drv_generic_parport_control (SIGNAL_RD | SIGNAL_CE, SIGNAL_RD | SIGNAL_CE);
  
  // Output Hold Time: 50 ns 
  ndelay(50);
  
  // turn on data line drivers
  drv_generic_parport_direction (0);
}


// perform status check in "auto mode"
static void drv_T6_status2 (void)
{
  int n;

  // turn off data line drivers
  drv_generic_parport_direction (1);
  
  // lower RD and CE
  drv_generic_parport_control (SIGNAL_RD | SIGNAL_CE, 0);
  
  // Access Time: 150 ns 
  ndelay(150);

  // wait for STA3=1
  n=0;
  do {
    rep_nop();
    if (++n>1000) {
      debug("hang in status2");
      bug=1;
      break;
    }
  } while ((drv_generic_parport_read() & 0x08) != 0x08);

  // rise RD and CE
  drv_generic_parport_control (SIGNAL_RD | SIGNAL_CE, SIGNAL_RD | SIGNAL_CE);

  // Output Hold Time: 50 ns 
  ndelay(50);

  // turn on data line drivers
  drv_generic_parport_direction (0);
}


static void drv_T6_write_cmd (unsigned char cmd)
{
  // wait until the T6963 is idle
  drv_T6_status1();

  // put data on DB1..DB8
  drv_generic_parport_data (cmd);
  
  // lower WR and CE
  drv_generic_parport_control (SIGNAL_WR | SIGNAL_CE, 0);
  
  // Pulse width
  ndelay(80);

  // rise WR and CE
  drv_generic_parport_control (SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

  // Data Hold Time
  ndelay(40);
}


static void drv_T6_write_data (unsigned char data)
{
  // wait until the T6963 is idle
  drv_T6_status1();

  // put data on DB1..DB8
  drv_generic_parport_data (data);

  // lower C/D
  drv_generic_parport_control (SIGNAL_CD, 0);
  
  // C/D Setup Time
  ndelay(20);

  // lower WR and CE
  drv_generic_parport_control (SIGNAL_WR | SIGNAL_CE, 0);
  
  // Pulse Width
  ndelay(80);
  
  // rise WR and CE
  drv_generic_parport_control (SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);
  
  // Data Hold Time
  ndelay(40);

  // rise CD
  drv_generic_parport_control (SIGNAL_CD, SIGNAL_CD);
}


static void drv_T6_write_auto (unsigned char data)
{
  // wait until the T6963 is idle
  drv_T6_status2();

  // put data on DB1..DB8
  drv_generic_parport_data (data);

  // lower C/D
  drv_generic_parport_control (SIGNAL_CD, 0);
  
  // C/D Setup Time
  ndelay(20);

  // lower WR and CE
  drv_generic_parport_control (SIGNAL_WR | SIGNAL_CE, 0);
  
  // Pulse Width
  ndelay(80);
  
  // rise WR and CE
  drv_generic_parport_control (SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);
  
  // Data Hold Time
  ndelay(40);

  // rise CD
  drv_generic_parport_control (SIGNAL_CD, SIGNAL_CD);
}


#if 0 // not used
static void drv_T6_send_byte (unsigned char cmd, unsigned char data)
{
  drv_T6_write_data(data);
  drv_T6_write_cmd(cmd);
}
#endif

static void drv_T6_send_word (unsigned char cmd, unsigned short data)
{
  drv_T6_write_data(data&0xff);
  drv_T6_write_data(data>>8);
  drv_T6_write_cmd(cmd);
}


static void drv_T6_memset(unsigned short addr, unsigned char data, int len)
{
  int i;
  
  drv_T6_send_word (0x24, addr);      // Set Adress Pointer
  drv_T6_write_cmd(0xb0);             // Set Data Auto Write
  for (i=0; i<len; i++) {
    drv_T6_write_auto(data);
    if (bug) {
      bug=0;
      debug("bug occured at byte %d of %d", i, len);
    }
  }
  drv_T6_status2();
  drv_T6_write_cmd(0xb2);             // Auto Reset
}


static void drv_T6_memcpy(unsigned short addr, unsigned char *data, int len)
{
  int i;
  
  drv_T6_send_word (0x24, 0x0200+addr);  // Set Adress Pointer
  drv_T6_write_cmd(0xb0);                // Set Data Auto Write
  for (i=0; i<len; i++) {
    drv_T6_write_auto(*(data++));
    if (bug) {
      bug=0;
      debug("bug occured at byte %d of %d, addr=%d", i, len, addr);
    }
  }
  drv_T6_status2();
  drv_T6_write_cmd(0xb2);                // Auto Reset
}


static int drv_T6_start (char *section)
{
  char *model, *s;
  int rows, cols;
  
  model=cfg_get(section, "Model", "generic");
  if (model!=NULL && *model!='\0') {
    int i;
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
    error ("%s: empty '%s.Model' entry from %s", Name, section, cfg_source());
    return -1;
  }

  s=cfg_get(section, "Size", NULL);
  if (s==NULL || *s=='\0') {
    error ("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
    return -1;
  }

  DROWS = -1;
  DCOLS = -1;
  if (sscanf(s, "%dx%d", &DCOLS, &DROWS)!=2 || DCOLS<1 || DROWS<1) {
    error ("%s: bad Size '%s' from %s", Name, s, cfg_source());
    return -1;
  }
  
  s=cfg_get(section, "Font", "6x8");
  if (s==NULL || *s=='\0') {
    error ("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
    return -1;
  }

  XRES = -1;
  YRES = -1;
  if (sscanf(s, "%dx%d", &XRES, &YRES)!=2 || XRES<1 || YRES<1) {
    error ("%s: bad Font '%s' from %s", Name, s, cfg_source());
    return -1;
  }
  
  if (drv_generic_parport_open(section, Name) != 0) {
    error ("%s: could not initialize parallel port!", Name);
    return -1;
  }
  
  if ((SIGNAL_CE=drv_generic_parport_wire_ctrl ("CE", "STROBE"))==0xff) return -1;
  if ((SIGNAL_CD=drv_generic_parport_wire_ctrl ("CD", "SELECT"))==0xff) return -1;
  if ((SIGNAL_RD=drv_generic_parport_wire_ctrl ("RD", "AUTOFD"))==0xff) return -1;
  if ((SIGNAL_WR=drv_generic_parport_wire_ctrl ("WR", "INIT")  )==0xff) return -1;
  
  // rise CE, CD, RD and WR
  drv_generic_parport_control (SIGNAL_CE | SIGNAL_CD | SIGNAL_RD | SIGNAL_WR,
			       SIGNAL_CE | SIGNAL_CD | SIGNAL_RD | SIGNAL_WR);
  // set direction: write
  drv_generic_parport_direction (0);
  
  // initialize display
  
  rows=DROWS/8; // text rows, assume 6x8 font
  cols=DCOLS/6; // text cols, assume 6x8 font

  drv_T6_send_word (0x40, 0x0000);    // Set Text Home Address
  drv_T6_send_word (0x41, cols);      // Set Text Area
  
  drv_T6_send_word (0x42, 0x0200);    // Set Graphic Home Address
  drv_T6_send_word (0x43, cols);      // Set Graphic Area
  
  drv_T6_write_cmd (0x80);            // Mode Set: OR mode, Internal CG RAM mode
  drv_T6_send_word (0x22, 0x0002);    // Set Offset Register
  drv_T6_write_cmd (0x98);            // Set Display Mode: Curser off, Text off, Graphics on
  drv_T6_write_cmd (0xa0);            // Set Cursor Pattern: 1 line cursor
  drv_T6_send_word (0x21, 0x0000);    // Set Cursor Pointer to (0,0)
  
  
  // clear display
  
  // upper half
  if (rows>8) rows=8;
  drv_T6_memset(0x0000, 0, cols*rows);    // clear text area 
  drv_T6_memset(0x0200, 0, cols*rows*8);  // clear graphic area
  
  // lower half
  if (DROWS>8*8) {
    rows=DROWS/8-8;
    drv_T6_memset(0x8000, 0, cols*rows);    // clear text area #2
    drv_T6_memset(0x8200, 0, cols*rows*8);  // clear graphic area #2
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


// using drv_generic_graphic_draw(W)
// using drv_generic_graphic_icon_draw(W)
// using drv_generic_graphic_bar_draw(W)


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_T6_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_T6_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // display preferences
  GOTO_COST = 2;  // number of bytes a goto command requires
  
  // real worker functions
  // Fixme: which one?
  // drv_generic_text_real_write   = drv_T6_write;
  // drv_generic_text_real_goto    = drv_T6_goto;
  // drv_generic_text_real_defchar = drv_T6_defchar;
  
  // start display
  if ((ret=drv_T6_start (section))!=0)
    return ret;
  
  // initialize generic graphic driver
  if ((ret=drv_generic_graphic_init(section, Name))!=0)
    return ret;
  
  // register text widget
  wc=Widget_Text;
  wc.draw=drv_generic_graphic_draw;
  widget_register(&wc);

  // register icon widget
  wc=Widget_Icon;
  wc.draw=drv_generic_graphic_icon_draw;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_generic_graphic_bar_draw;
  widget_register(&wc);
  
  // register plugins
  // none at the moment...


  return 0;
}


// close driver & display
int drv_T6_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_parport_close();
  drv_generic_graphic_quit();
  
  return (0);
}


DRIVER drv_T6963 = {
  name: Name,
  list: drv_T6_list,
  init: drv_T6_init,
  quit: drv_T6_quit, 
};


