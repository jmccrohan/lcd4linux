/* $Id: drv_HD44780.c,v 1.1 2004/01/20 15:32:49 reinelt Exp $
 *
 * new style driver for HD44780-based displays
 *
 * Copyright 1999-2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Modification for 4-Bit mode
 * Copyright 2003 Martin Hejl (martin@hejl.de)
 *
 * Modification for 2nd controller support
 * Copyright 2003 Jesse Brook Kovach <jkovach@wam.umd.edu>
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
 * $Log: drv_HD44780.c,v $
 * Revision 1.1  2004/01/20 15:32:49  reinelt
 * first version of Next Generation HD44780 (untested! but it compiles...)
 * some cleanup in the other drivers
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_HD44780
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_parport.h"

static char Name[]="HD44780";


/* low level communication timings [nanoseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_CYCLE 1000 // Enable cycle time
#define T_PW     450 // Enable pulse width
#define T_AS      60 // Address setup time
#define T_H       40 // Data hold time


/* HD44780 execution timings [microseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_INIT1 4100 // first init sequence:  4.1 msec
#define T_INIT2  100 // second init sequence: 100 usec
#define T_EXEC    80 // normal execution time
#define T_WRCG   120 // CG RAM Write
#define T_CLEAR 1640 // Clear Display


static int Icons;
static int Bits=0;
static int GPO=0;
static int Controllers = 0;
static int Controller  = 0;

static unsigned char SIGNAL_RW;
static unsigned char SIGNAL_RS;
static unsigned char SIGNAL_ENABLE;
static unsigned char SIGNAL_ENABLE2;
static unsigned char SIGNAL_GPO;

// Fixme
static int GPOS;

// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void drv_HD_nibble(unsigned char controller, unsigned char nibble)
{
  unsigned char enable;

  // enable signal: 'controller' is a bitmask
  // bit 0 .. send to controller #0
  // bit 1 .. send to controller #1
  // so we can send a byte to both controllers at the same time!
  enable=0;
  if (controller&0x01) enable|=SIGNAL_ENABLE;
  if (controller&0x02) enable|=SIGNAL_ENABLE2;
  
  // clear ENABLE
  // put data on DB1..DB4
  // nibble already contains RS bit!
  drv_generic_parport_data(nibble);

  // Address set-up time
  ndelay(T_AS);
  
  // rise ENABLE
  drv_generic_parport_data(nibble | enable);
  
  // Enable pulse width
  ndelay(T_PW);

  // lower ENABLE
  drv_generic_parport_data(nibble);
}


static void drv_HD_byte (unsigned char controller, unsigned char data, unsigned char RS)
{
  // send high nibble of the data
  drv_HD_nibble (controller, ((data>>4)&0x0f)|RS);

  // Make sure we honour T_CYCLE
  ndelay(T_CYCLE-T_AS-T_PW);

  // send low nibble of the data
  drv_HD_nibble(controller, (data&0x0f)|RS);
}


static void drv_HD_command (unsigned char controller, unsigned char cmd, int delay)
{
  unsigned char enable;
   
  if (Bits==8) {
    
    // enable signal: 'controller' is a bitmask
    // bit 0 .. send to controller #0
    // bit 1 .. send to controller #1
    // so we can send a byte to both controllers at the same time!
    enable=0;
    if (controller&0x01) enable|=SIGNAL_ENABLE;
    if (controller&0x02) enable|=SIGNAL_ENABLE2;
    
    // put data on DB1..DB8
    drv_generic_parport_data (cmd);
  
    // clear RW and RS
    drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, 0);
    
    // Address set-up time
    ndelay(T_AS);

    // send command
    drv_generic_parport_toggle (enable, 1, T_PW);
    
  } else {

    drv_HD_byte (controller, cmd, 0);

  }
  
  // wait for command completion
  udelay(delay);

}


static void drv_HD_data (unsigned char controller, char *string, int len, int delay)
{
  unsigned char enable;

  // sanity check
  if (len<=0) return;

  if (Bits==8) {

    // enable signal: 'controller' is a bitmask
    // bit 0 .. send to controller #0
    // bit 1 .. send to controller #1
    // so we can send a byte to both controllers at the same time!
    enable=0;
    if (controller&0x01) enable|=SIGNAL_ENABLE;
    if (controller&0x02) enable|=SIGNAL_ENABLE2;
    
    // clear RW, set RS
    drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);

    // Address set-up time
    ndelay(T_AS);
    
    while (len--) {
      
      // put data on DB1..DB8
      drv_generic_parport_data (*(string++));
      
      // send command
      drv_generic_parport_toggle (enable, 1, T_PW);
      
      // wait for command completion
      udelay(delay);
    }
    
  } else { // 4 bit mode
    
    while (len--) {

      // send data with RS enabled
      drv_HD_byte (controller, *(string++), SIGNAL_RS);
      
      // wait for command completion
      udelay(delay);
    }
  }
}


static void drv_HD_goto (int row, int col)
{
  int pos;

  // handle multiple displays/controllers
  if (Controllers>1 && row>=DROWS/2) {
    row -= DROWS/2;
    Controller = 2;
  } else {
    Controller = 1;
  }
   
  // 16x1 Displays are organized as 8x2 :-(
  if (DCOLS==16 && DROWS==1 && col>7) {
    row++;
    col-=8;
  }
  
  // 16x4 Displays use a slightly different layout
  if (DCOLS==16 && DROWS==4) {
    pos=(row%2)*64+(row/2)*16+col;
  } else {  
    pos=(row%2)*64+(row/2)*20+col;
  }
  
  drv_HD_command (Controller, (0x80|pos), T_EXEC);
}


static int drv_HD_start (char *section)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s;
  
  s=cfg_get(section, "Size", NULL);
  if (s==NULL || *s=='\0') {
    error ("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("%s: bad size '%s'", Name, s);
    return -1;
  }
  
  if (cfg_number(section, "GPOs", 0, 0, 8, &gpos)<0) return -1;
  info ("%s: controlling %d GPO's", Name, gpos);

  if (cfg_number(section, "Controllers", 1, 1, 2, &Controllers)<0) return -1;
  info ("%s: using display with %d controllers", Name, Controllers);
  
  // current controller
  Controller=1;
  
  DROWS = rows;
  DCOLS = cols;
  GPOS  = gpos;
  
  if (cfg_number(section, "Bits", 8, 4, 8, &Bits)<0) return -1;
  if (Bits!=4 && Bits!=8) {
    error ("%s: bad %s.Bits '%s' from %s, should be '4' or '8'", Name, section, s, cfg_source());
    return -1;
  }    
  info ("%s: using %d bit mode", Name, Bits);
  
  if (drv_generic_parport_open(section, Name) != 0) {
    error ("%s: could not initialize parallel port!", Name);
    return -1;
  }

  if (Bits==8) {
    if ((SIGNAL_RS      = drv_generic_parport_wire_ctrl ("RS",      "AUTOFD"))==0xff) return -1;
    if ((SIGNAL_RW      = drv_generic_parport_wire_ctrl ("RW",      "GND"   ))==0xff) return -1;
    if ((SIGNAL_ENABLE  = drv_generic_parport_wire_ctrl ("ENABLE",  "STROBE"))==0xff) return -1;
    if ((SIGNAL_ENABLE2 = drv_generic_parport_wire_ctrl ("ENABLE2", "GND"   ))==0xff) return -1;
    if ((SIGNAL_GPO     = drv_generic_parport_wire_ctrl ("GPO",     "GND"   ))==0xff) return -1;
  } else {
    if ((SIGNAL_RS      = drv_generic_parport_wire_data ("RS",      "DB4"))==0xff) return -1;
    if ((SIGNAL_RW      = drv_generic_parport_wire_data ("RW",      "DB5"))==0xff) return -1;
    if ((SIGNAL_ENABLE  = drv_generic_parport_wire_data ("ENABLE",  "DB6"))==0xff) return -1;
    if ((SIGNAL_ENABLE2 = drv_generic_parport_wire_data ("ENABLE2", "GND"))==0xff) return -1;
    if ((SIGNAL_GPO     = drv_generic_parport_wire_data ("GPO",     "GND"))==0xff) return -1;
  }
  
  // clear all signals
  if (Bits==8) {
    drv_generic_parport_control (SIGNAL_RS|SIGNAL_RW|SIGNAL_ENABLE|SIGNAL_ENABLE2|SIGNAL_GPO, 0);
  } else {
    drv_generic_parport_data (0);
  }
  
  // set direction: write
  drv_generic_parport_direction (0);
  
  // initialize *both* displays
  if (Bits==8) {
    drv_HD_command (0x03, 0x30, T_INIT1); // 8 Bit mode, wait 4.1 ms
    drv_HD_command (0x03, 0x30, T_INIT2); // 8 Bit mode, wait 100 us
    drv_HD_command (0x03, 0x38, T_EXEC);  // 8 Bit mode, 1/16 duty cycle, 5x8 font
  } else {
    drv_HD_nibble(0x03, 0x03); udelay(T_INIT1); // 4 Bit mode, wait 4.1 ms
    drv_HD_nibble(0x03, 0x03); udelay(T_INIT2); // 4 Bit mode, wait 100 us
    drv_HD_nibble(0x03, 0x03); udelay(T_INIT1); // 4 Bit mode, wait 4.1 ms
    drv_HD_nibble(0x03, 0x02); udelay(T_INIT2); // 4 Bit mode, wait 100 us
    drv_HD_command (0x03, 0x28, T_EXEC);	    // 4 Bit mode, 1/16 duty cycle, 5x8 font
  }
  
  drv_HD_command (0x03, 0x08, T_EXEC);  // Display off, cursor off, blink off
  drv_HD_command (0x03, 0x0c, T_CLEAR); // Display on, cursor off, blink off, wait 1.64 ms
  drv_HD_command (0x03, 0x06, T_EXEC);  // curser moves to right, no shift
  
  return 0;
}


static void drv_HD_write (char *string, int len)
{
  drv_HD_data (Controller, string, len, T_EXEC);
}


static void drv_HD_define_char (int ascii, char *buffer)
{
  // define chars on *both* controllers!
  drv_HD_command (0x03, 0x40|8*ascii, T_EXEC);
  drv_HD_data (0x03, buffer, 8, T_WRCG);
}


// Fixme
#if 0
static void drv_HD_setGPO (int bits)
{
  if (Lcd.gpos>0) {
    
    // put data on DB1..DB8
    drv_generic_parport_data (bits);
    
    // 74HCT573 set-up time
    ndelay(20);
    
    // send data
    // 74HCT573 enable pulse width = 24ns
    drv_generic_parport_toggle (SIGNAL_GPO, 1, 230);
  }
}
#endif

// ****************************************
// ***            plugins               ***
// ****************************************


// ****************************************
// ***        widget callbacks          ***
// ****************************************


int drv_HD_draw_text (WIDGET *W)
{
  return drv_generic_text_draw_text(W, 2, drv_HD_goto, drv_HD_write);
}


int drv_HD_draw_bar (WIDGET *W)
{
  return drv_generic_text_draw_bar(W, 2, drv_HD_define_char, drv_HD_goto, drv_HD_write);
}


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_HD_list (void)
{
  printf ("any");
  return 0;
}


// initialize driver & display
int drv_HD_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  XRES=5;
  YRES=8;
  CHARS=8;
  
  // start display
  if ((ret=drv_HD_start (section))!=0)
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
  wc.draw=drv_HD_draw_text;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_HD_draw_bar;
  widget_register(&wc);
  
  // register plugins
  // Fixme: plugins for HD44780?
  
  return 0;
}


// close driver & display
int drv_HD_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_parport_close();
  drv_generic_text_quit();
  
  return (0);
}


DRIVER drv_HD44780 = {
  name: Name,
  list: drv_HD_list,
  init: drv_HD_init,
  quit: drv_HD_quit, 
};

