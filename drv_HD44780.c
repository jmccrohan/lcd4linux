/* $Id: drv_HD44780.c,v 1.10 2004/02/02 05:22:16 reinelt Exp $
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
 * Revision 1.10  2004/02/02 05:22:16  reinelt
 * Brightness fpr Noritake Displays avaliable as a plugin
 *
 * Revision 1.9  2004/02/01 11:51:22  hejl
 * Fixes for busy flag
 *
 * Revision 1.8  2004/02/01 08:05:12  reinelt
 * Crystalfontz 633 extensions (CRC checking and stuff)
 * Models table for HD44780
 * Noritake VFD BVrightness patch from Bill Paxton
 *
 * Revision 1.7  2004/01/30 07:12:35  reinelt
 * HD44780 busy-flag support from Martin Hejl
 * loadavg() uClibc replacement from Martin Heyl
 * round() uClibc replacement from Martin Hejl
 * warning in i2c_sensors fixed
 * [
 *
 * Revision 1.6  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.5  2004/01/23 07:04:17  reinelt
 * icons finished!
 *
 * Revision 1.4  2004/01/23 04:53:48  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.3  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.2  2004/01/21 06:39:27  reinelt
 * HD44780 missed the "clear display' sequence
 * asc255bug handling added
 * HD44780 tested, works here!
 *
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
#include "drv_generic_text.h"
#include "drv_generic_parport.h"

static char Name[]="HD44780";

static int Model;
static int Capabilities;

/* low level communication timings [nanoseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_CYCLE 1000 // Enable cycle time
#define T_PW     450 // Enable pulse width
#define T_AS      60 // Address setup time
#define T_H       40 // Data hold time
#define T_AH      20 // Address hold time


/* HD44780 execution timings [microseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_INIT1 4100 // first init sequence:  4.1 msec
#define T_INIT2  100 // second init sequence: 100 usec
#define T_EXEC    80 // normal execution time
#define T_WRCG   120 // CG RAM Write
#define T_CLEAR 1680 // Clear Display


static int Bits=0;
static int numControllers = 0;
static int allControllers = 0;
static int currController = 0;

static unsigned char SIGNAL_RW;
static unsigned char SIGNAL_RS;
static unsigned char SIGNAL_ENABLE;
static unsigned char SIGNAL_ENABLE2;
static unsigned char SIGNAL_GPO;

/* maximum time to wait for the busy-flag (in usec) */
#define MAX_BUSYFLAG_WAIT 100000
static int UseBusy = 0;


// Fixme
static int GPOS;
// static int GPO=0;


typedef struct {
  int type;
  char *name;
  int capabilities;
} MODEL;

#define CAP_BRIGHTNESS (1<<0)
#define CAP_BUSY4BIT   (1<<1)

static MODEL Models[] = {
  { 0x01, "generic",  0 },
  { 0x02, "Noritake", CAP_BRIGHTNESS },
  { 0x03, "Soekris",  CAP_BUSY4BIT },
  { 0xff, "Unknown",  0 }
};


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

static void wait_for_busy_flag(int controller)
{
  unsigned char enable;
  unsigned int counter;
  unsigned char data=0xFF;
  unsigned char busymask=0;

  if (Bits==8) {
    busymask = 0x80;
  } else {
    /* Since in 4-Bit mode DB0 on the parport is mapped to DB4 on the LCD
    (and consequently, DB3 on the partport is mapped to DB7 on the LCD)
    we need to listen for DB3 on the parport to go low
    */
    busymask = 0x8;
  }

  enable=SIGNAL_ENABLE;

  while (controller > 0) {
    if (controller&0x01 && enable!=0) {
      /* set data-lines to input*/
      drv_generic_parport_direction(1);
      
      if (Bits==8) {
        // Set RW, clear RS
        drv_generic_parport_control(SIGNAL_RW|SIGNAL_RS,SIGNAL_RW);
      } else {
        drv_generic_parport_data(SIGNAL_RW);
      }
      
      // Address set-up time
      ndelay(T_AS);
      
      // rise ENABLE
      if (Bits==8) {
        drv_generic_parport_control(enable,enable);
      } else {
        drv_generic_parport_data(SIGNAL_RW|enable);
      }

      counter=0;
      while (1) {
        /* read the busy flag */
        data = drv_generic_parport_read();
        if ((data&busymask)==0) break;

        /* make sure we don't wait forever
        - but only check after 5 iterations
        that way, we won't slow down normal mode
        (where we don't need the timeout anyway) */
        counter++;

        if (counter >= 5) {
          struct timeval now, end;

          if (counter == 5) {
            // determine the time when the timeout has expired
            gettimeofday (&end, NULL);
            end.tv_usec+=MAX_BUSYFLAG_WAIT;
            while (end.tv_usec>1000000) {
              end.tv_usec-=1000000;
              end.tv_sec++;
            }
          }

          // get the current time
          gettimeofday(&now, NULL);
          if (now.tv_sec==end.tv_sec?now.tv_usec>=end.tv_usec:now.tv_sec>=end.tv_sec) {
            error ("HD44780: timeout waiting for busy flag on controller %x (%x)", controller, data);
            break;

          }
        }
      }

      // RS=low, RW=low, EN=low
      if (Bits==8) {
        // Lower EN
        drv_generic_parport_control(enable,0);

        // Address hold time
        ndelay(T_AH);

        drv_generic_parport_control(SIGNAL_RW|SIGNAL_RS,0);
      } else {
        // Lower EN
        drv_generic_parport_data(SIGNAL_RW);
        ndelay(T_AH);
        drv_generic_parport_data(0);
      }

      /* set data-lines to output*/
      drv_generic_parport_direction(0);
    }
    enable=SIGNAL_ENABLE2;
    controller=controller >> 1;
  }
}


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

  if (UseBusy) wait_for_busy_flag(controller);

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
  if (!UseBusy) udelay(delay);
  
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
    
    if (!UseBusy) {
      // clear RW, set RS
      drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);
      // Address set-up time
      ndelay(T_AS);
    }
    
    while (len--) {

      if (UseBusy) {
	wait_for_busy_flag(controller);
	// clear RW, set RS
	drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);
	// Address set-up time
	ndelay(T_AS);
      }
      
      // put data on DB1..DB8
      drv_generic_parport_data (*(string++));
      
      // send command
      drv_generic_parport_toggle (enable, 1, T_PW);
      
      // wait for command completion
      if (!UseBusy) udelay(delay);
    }
    
  } else { // 4 bit mode
    
    while (len--) {
      if (UseBusy) wait_for_busy_flag(controller);

      // send data with RS enabled
      drv_HD_byte (controller, *(string++), SIGNAL_RS);
      
      // wait for command completion
      if (!UseBusy) udelay(delay);
    }
  }
}


static void drv_HD_goto (int row, int col)
{
  int pos;

  // handle multiple displays/controllers
  if (numControllers>1 && row>=DROWS/2) {
    row -= DROWS/2;
    currController = 2;
  } else {
    currController = 1;
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
  
  drv_HD_command (currController, (0x80|pos), T_EXEC);
}


static void drv_HD_write (char *string, int len)
{
  drv_HD_data (currController, string, len, T_EXEC);
}


static void drv_HD_defchar (int ascii, char *buffer)
{
  // define chars on *both* controllers!
  drv_HD_command (allControllers, 0x40|8*ascii, T_EXEC);
  drv_HD_data (allControllers, buffer, 8, T_WRCG);
}


static int drv_HD_brightness (int brightness)
{
  char cmd;
  
  if (!(Capabilities & CAP_BRIGHTNESS)) return -1;

  if (brightness<0) brightness=0;
  if (brightness>3) brightness=3;

  cmd='0'+brightness;
  
  drv_HD_command (allControllers, 0x38, T_EXEC); // enable function
  drv_HD_data (allControllers, &cmd, 1, T_WRCG); // set brightness

  return brightness;
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


static int drv_HD_start (char *section)
{
  char *model, *s;
  int rows=-1, cols=-1, gpos=-1;
  
  model=cfg_get(section, "Model", NULL);
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
    Capabilities=Models[Model].capabilities;
    info ("%s: using model '%s'", Name, Models[Model].name);
  } else {
    error ("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
    return -1;
  }

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

  if (cfg_number(section, "Controllers", 1, 1, 2, &numControllers)<0) return -1;
  info ("%s: using display with %d controllers", Name, numControllers);
  
  // current controller
  currController=1;
  
  // Bitmask for *all* Controllers
  allControllers = numControllers==2 ? 3 : 1;
  
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
    drv_HD_command (allControllers, 0x30, T_INIT1); // 8 Bit mode, wait 4.1 ms
    drv_HD_command (allControllers, 0x30, T_INIT2); // 8 Bit mode, wait 100 us
    drv_HD_command (allControllers, 0x38, T_EXEC);  // 8 Bit mode, 1/16 duty cycle, 5x8 font
  } else {
    drv_HD_nibble  (allControllers, 0x03); udelay(T_INIT1); // 4 Bit mode, wait 4.1 ms
    drv_HD_nibble  (allControllers, 0x03); udelay(T_INIT2); // 4 Bit mode, wait 100 us
    drv_HD_nibble  (allControllers, 0x03); udelay(T_INIT1); // 4 Bit mode, wait 4.1 ms
    drv_HD_nibble  (allControllers, 0x02); udelay(T_INIT2); // 4 Bit mode, wait 100 us
    drv_HD_command (allControllers, 0x28, T_EXEC);	    // 4 Bit mode, 1/16 duty cycle, 5x8 font
  }

  drv_HD_command (allControllers, 0x08, T_EXEC);  // Display off, cursor off, blink off
  drv_HD_command (allControllers, 0x0c, T_CLEAR); // Display on, cursor off, blink off, wait 1.64 ms
  drv_HD_command (allControllers, 0x06, T_EXEC);  // curser moves to right, no shift
  
  // maybe use busy-flag from now on 
  // (we can't use the busy flag during the init sequence)
  cfg_number(section, "UseBusy", 0, 0, 1, &UseBusy);
  
  // make sure we don't use the busy flag with RW wired to GND
  if (UseBusy && !SIGNAL_RW)   {
    error("%s: Busyflag is to be used, but RW is wired to GND", Name);
    UseBusy=0;
  }

  // make sure the display supports busy-flag checking in 4-Bit-Mode
  // at the moment this is inly possible with Martin Hejl's gpio driver,
  // which allows to use 4 bits as input and 4 bits as output
  if (UseBusy && Bits==4 && !(Capabilities&CAP_BUSY4BIT)) {
    error("%s: Model '%s' does not support busy-flag checking in 4-bit-mode", Name, Models[Model].name);
    UseBusy=0;
  }
  
  info("%s: %susing busy-flag checking", Name, UseBusy?"":"not ");

  drv_HD_command (allControllers, 0x01, T_CLEAR); // clear *both* displays
  drv_HD_command (allControllers, 0x03, T_CLEAR); // return home
  
  // maybe set brightness
  if (Capabilities & CAP_BRIGHTNESS) {
    int brightness;
    if (cfg_number(section, "Brightness", 0, 0, 3, &brightness)==0) {
      drv_HD_brightness(brightness);
    }
  }

  return 0;
}


// ****************************************
// ***            plugins               ***
// ****************************************


static void plugin_brightness (RESULT *result, RESULT *arg1)
{
  double brightness;
  
  brightness=drv_HD_brightness(R2N(arg1));
  SetResult(&result, R_NUMBER, &brightness); 
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
int drv_HD_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_HD_init (char *section)
{
  WIDGET_CLASS wc;
  int asc255bug;
  int ret;  
  
  // display preferences
  XRES  = 5;      // pixel width of one char 
  YRES  = 8;      // pixel height of one char 
  CHARS = 8;      // number of user-defineable characters
  CHAR0 = 0;      // ASCII of first user-defineable char
  GOTO_COST = 2;  // number of bytes a goto command requires
  
  // real worker functions
  drv_generic_text_real_write   = drv_HD_write;
  drv_generic_text_real_goto    = drv_HD_goto;
  drv_generic_text_real_defchar = drv_HD_defchar;


  // start display
  if ((ret=drv_HD_start (section))!=0)
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
  // most displays have a full block on ascii 255, but some have kind of 
  // an 'inverted P'. If you specify 'asc255bug 1 in the config, this
  // char will not be used, but rendered by the bar driver
  cfg_number(section, "asc255bug", 0, 0, 1, &asc255bug);
  drv_generic_text_bar_add_segment (  0,  0,255, 32); // ASCII  32 = blank
  if (!asc255bug) 
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
  if (Capabilities & CAP_BRIGHTNESS)
    AddFunction ("brightness", 1, plugin_brightness);
  
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


#if 0
+
+// Change Noritake CU series VFD brightness level
+  char tmpbuffer[2];
+  int cu_vfd_brightness;
+  if (cfg_number("CU_VFD_Brightness", 0, 0, 3, &cu_vfd_brightness)<0) return -1;
+  if (cu_vfd_brightness) {
  +    snprintf (tmpbuffer, 2, "\%o", cu_vfd_brightness);
  +    HD_command (0x03, 0x38, T_EXEC);           // enable function
  +    HD_write (0x03, tmpbuffer, 1, T_WRCG);     // set brightness
  +    info ("HD44780: Noritake CU VFD detected. Brightness = %d (0-3)", cu_vfd_brightness);
  +    info ("         Settings: 0=100\%, 1=75\%, 2=50\%, 3=25\%");
  + }
 
#endif
