/* $Id: drv_HD44780.c,v 1.37 2004/09/18 15:58:57 reinelt Exp $
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
 * Revision 1.37  2004/09/18 15:58:57  reinelt
 * even more HD44780 cleanups, hardwiring for LCM-162
 *
 * Revision 1.36  2004/09/18 10:57:29  reinelt
 * more parport/i2c cleanups
 *
 * Revision 1.35  2004/09/18 09:48:29  reinelt
 * HD44780 cleanup and prepararation for I2C backend
 * LCM-162 submodel framework
 *
 * Revision 1.34  2004/09/18 08:22:59  reinelt
 * drv_generic_parport_status() to read status lines
 *
 * Revision 1.33  2004/08/29 13:03:41  reinelt
 *
 * added RouterBoard driver
 *
 * Revision 1.32  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.31  2004/06/26 09:27:20  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.30  2004/06/20 10:09:54  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.29  2004/06/06 06:51:59  reinelt
 *
 * do not display end splash screen if quiet=1
 *
 * Revision 1.28  2004/06/05 06:41:39  reinelt
 *
 * chancged splash screen again
 *
 * Revision 1.27  2004/06/05 06:13:11  reinelt
 *
 * splash screen for all text-based display drivers
 *
 * Revision 1.26  2004/06/02 10:09:22  reinelt
 *
 * splash screen for HD44780
 *
 * Revision 1.25  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.24  2004/06/01 06:45:29  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.23  2004/05/31 05:38:02  reinelt
 *
 * fixed possible bugs with user-defined chars (clear high bits)
 * thanks to Andy Baxter for debugging the MilfordInstruments driver!
 *
 * Revision 1.22  2004/05/27 03:39:47  reinelt
 *
 * changed function naming scheme to plugin::function
 *
 * Revision 1.21  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.20  2004/05/22 04:23:49  reinelt
 *
 * removed 16*x fix again (next time think before commit :-)
 *
 * Revision 1.19  2004/05/22 04:21:02  reinelt
 *
 * fix for display RAM layout on 16x4 displays (thanks to toxicated101)
 *
 * Revision 1.18  2004/03/20 07:31:32  reinelt
 * support for HD66712 (which has a different RAM layout)
 * further threading development
 *
 * Revision 1.17  2004/03/19 09:17:46  reinelt
 *
 * removed the extra 'goto' function, row and col are additional parameters
 * of the write() function now.
 *
 * Revision 1.16  2004/03/11 06:39:58  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.15  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.14  2004/03/01 04:29:51  reinelt
 * cfg_number() returns -1 on error, 0 if value not found (but default val used),
 *  and 1 if value was used from the configuration.
 * HD44780 driver adopted to new cfg_number()
 * Crystalfontz 631 driver nearly finished
 *
 * Revision 1.13  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.12  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.11  2004/02/04 19:10:51  reinelt
 * Crystalfontz driver nearly finished
 *
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
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_parport.h"

static char Name[]="HD44780";

static int Bus;
static int Model;
static int Capabilities;


/* low level communication timings [nanoseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_CYCLE 1000 /* Enable cycle time */
#define T_PW     450 /* Enable pulse width */
#define T_AS      60 /* Address setup time */
#define T_H       40 /* Data hold time */
#define T_AH      20 /* Address hold time */


/* HD44780 execution timings [microseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_INIT1 4100 /* first init sequence:  4.1 msec */
#define T_INIT2  100 /* second init sequence: 100 usec */
#define T_EXEC    80 /* normal execution time */
#define T_WRCG   120 /* CG RAM Write */
#define T_CLEAR 1680 /* Clear Display */


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


/* Fixme: GPO's not yet implemented */
static int GPOS;
/* static int GPO=0; */


typedef struct {
  int type;
  char *name;
  int capabilities;
} MODEL;

#define CAP_PARPORT    (1<<0)
#define CAP_I2C        (1<<1)
#define CAP_GPO        (1<<2)
#define CAP_BRIGHTNESS (1<<3)
#define CAP_BUSY4BIT   (1<<4)
#define CAP_HD66712    (1<<5)
#define CAP_LCM162     (1<<6)

#define BUS_PP  CAP_PARPORT
#define BUS_I2C CAP_I2C


static MODEL Models[] = {
  { 0x01, "generic",  CAP_PARPORT|CAP_I2C|CAP_GPO },
  { 0x02, "Noritake", CAP_PARPORT|CAP_I2C|CAP_GPO|CAP_BRIGHTNESS },
  { 0x03, "Soekris",  CAP_PARPORT|CAP_BUSY4BIT },
  { 0x04, "HD66712",  CAP_PARPORT|CAP_I2C|CAP_GPO|CAP_HD66712 },
  { 0x05, "LCM-162",  CAP_PARPORT|CAP_LCM162 },
  { 0xff, "Unknown",  0 }
};



/****************************************/
/***  generic functions               ***/
/****************************************/

static int  (*drv_HD_load)   (const char *section);
static void (*drv_HD_command) (const unsigned char controller, const unsigned char cmd, const int delay);
static void (*drv_HD_data)    (const unsigned char controller, const char *string, const int len, const int delay);
static void (*drv_HD_stop)    (void);



/****************************************/
/***  parport dependant functions     ***/
/****************************************/

static void drv_HD_PP_busy(int controller)
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
        /* Set RW, clear RS */
        drv_generic_parport_control(SIGNAL_RW|SIGNAL_RS,SIGNAL_RW);
      } else {
        drv_generic_parport_data(SIGNAL_RW);
      }
      
      /* Address set-up time */
      ndelay(T_AS);
      
      /* rise ENABLE */
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
            /* determine the time when the timeout has expired */
            gettimeofday (&end, NULL);
            end.tv_usec+=MAX_BUSYFLAG_WAIT;
            while (end.tv_usec>1000000) {
              end.tv_usec-=1000000;
              end.tv_sec++;
            }
          }

          /* get the current time */
          gettimeofday(&now, NULL);
          if (now.tv_sec==end.tv_sec?now.tv_usec>=end.tv_usec:now.tv_sec>=end.tv_sec) {
            error ("%s: timeout waiting for busy flag on controller %x (%x)", Name, controller, data);
            break;

          }
        }
      }

      /* RS=low, RW=low, EN=low */
      if (Bits==8) {
        /* Lower EN */
        drv_generic_parport_control(enable,0);

        /* Address hold time */
        ndelay(T_AH);

        drv_generic_parport_control(SIGNAL_RW|SIGNAL_RS,0);
      } else {
        /* Lower EN */
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


static void drv_HD_PP_nibble(const unsigned char controller, const unsigned char nibble)
{
  unsigned char enable;

  /* enable signal: 'controller' is a bitmask */
  /* bit 0 .. send to controller #0 */
  /* bit 1 .. send to controller #1 */
  /* so we can send a byte to both controllers at the same time! */
  enable=0;
  if (controller&0x01) enable|=SIGNAL_ENABLE;
  if (controller&0x02) enable|=SIGNAL_ENABLE2;
  
  /* clear ENABLE */
  /* put data on DB1..DB4 */
  /* nibble already contains RS bit! */
  drv_generic_parport_data(nibble);

  /* Address set-up time */
  ndelay(T_AS);
  
  /* rise ENABLE */
  drv_generic_parport_data(nibble | enable);
  
  /* Enable pulse width */
  ndelay(T_PW);

  /* lower ENABLE */
  drv_generic_parport_data(nibble);
}


static void drv_HD_PP_byte (const unsigned char controller, const unsigned char data, const unsigned char RS)
{
  /* send high nibble of the data */
  drv_HD_PP_nibble (controller, ((data>>4)&0x0f)|RS);

  /* Make sure we honour T_CYCLE */
  ndelay(T_CYCLE-T_AS-T_PW);

  /* send low nibble of the data */
  drv_HD_PP_nibble(controller, (data&0x0f)|RS);
}


static void drv_HD_PP_command (const unsigned char controller, const unsigned char cmd, const int delay)
{
  unsigned char enable;

  if (UseBusy) drv_HD_PP_busy(controller);

  if (Bits==8) {
    
    /* enable signal: 'controller' is a bitmask */
    /* bit 0 .. send to controller #0 */
    /* bit 1 .. send to controller #1 */
    /* so we can send a byte to both controllers at the same time! */
    enable=0;
    if (controller&0x01) enable|=SIGNAL_ENABLE;
    if (controller&0x02) enable|=SIGNAL_ENABLE2;
    
    /* put data on DB1..DB8 */
    drv_generic_parport_data (cmd);
  
    /* clear RW and RS */
    drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, 0);
    
    /* Address set-up time */
    ndelay(T_AS);

    /* send command */
    drv_generic_parport_toggle (enable, 1, T_PW);
    
  } else {

    drv_HD_PP_byte (controller, cmd, 0);

  }
  
  /* wait for command completion */
  if (!UseBusy) udelay(delay);

}


static void drv_HD_PP_data (const unsigned char controller, const char *string, const int len, const int delay)
{
  int l = len;
  unsigned char enable;

  /* sanity check */
  if (len<=0) return;

  if (Bits==8) {

    /* enable signal: 'controller' is a bitmask */
    /* bit 0 .. send to controller #0 */
    /* bit 1 .. send to controller #1 */
    /* so we can send a byte to both controllers at the same time! */
    enable=0;
    if (controller&0x01) enable|=SIGNAL_ENABLE;
    if (controller&0x02) enable|=SIGNAL_ENABLE2;
    
    if (!UseBusy) {
      /* clear RW, set RS */
      drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);
      /* Address set-up time */
      ndelay(T_AS);
    }
    
    while (l--) {

      if (UseBusy) {
	drv_HD_PP_busy(controller);
	/* clear RW, set RS */
	drv_generic_parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);
	/* Address set-up time */
	ndelay(T_AS);
      }
      
      /* put data on DB1..DB8 */
      drv_generic_parport_data (*(string++));
      
      /* send command */
      drv_generic_parport_toggle (enable, 1, T_PW);
      
      /* wait for command completion */
      if (!UseBusy) udelay(delay);
    }
    
  } else { /* 4 bit mode */
    
    while (l--) {
      if (UseBusy) drv_HD_PP_busy(controller);

      /* send data with RS enabled */
      drv_HD_PP_byte (controller, *(string++), SIGNAL_RS);
      
      /* wait for command completion */
      if (!UseBusy) udelay(delay);
    }
  }
}


static int drv_HD_PP_load (const char *section)
{
  if (cfg_number(section, "Bits", 8, 4, 8, &Bits)<0) return -1;
  if (Bits!=4 && Bits!=8) {
    error ("%s: bad %s.Bits '%d' from %s, should be '4' or '8'", Name, section, Bits, cfg_source());
    return -1;
  }    
  
  /* LCM-162 only supports 8-bit-mode */
  if (Capabilities & CAP_LCM162 && Bits != 8) {
    error ("%s: Model '%s' does not support %d bit mode!", Name, Models[Model].name, Bits);
    Bits = 8;
  }
  info ("%s: using %d bit mode", Name, Bits);
  
  if (drv_generic_parport_open(section, Name) != 0) {
    error ("%s: could not initialize parallel port!", Name);
    return -1;
  }

  /* the LCM-162 is hardwired */
  if (Capabilities & CAP_LCM162) {
    if ((SIGNAL_RS      = drv_generic_parport_hardwire_ctrl ("RS",      "SELECT"))==0xff) return -1;
    if ((SIGNAL_RW      = drv_generic_parport_hardwire_ctrl ("RW",      "INIT"  ))==0xff) return -1;
    if ((SIGNAL_ENABLE  = drv_generic_parport_hardwire_ctrl ("ENABLE",  "AUTOFD"))==0xff) return -1;
    if ((SIGNAL_ENABLE2 = drv_generic_parport_hardwire_ctrl ("ENABLE2", "GND"   ))==0xff) return -1;
    if ((SIGNAL_GPO     = drv_generic_parport_hardwire_ctrl ("GPO",     "GND"   ))==0xff) return -1;
  } else {
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
  }
  
  /* clear all signals */
  if (Bits==8) {
    drv_generic_parport_control (SIGNAL_RS|SIGNAL_RW|SIGNAL_ENABLE|SIGNAL_ENABLE2|SIGNAL_GPO, 0);
  } else {
    drv_generic_parport_data (0);
  }
  
  /* set direction: write */
  drv_generic_parport_direction (0);
  
  /* initialize *both* displays */
  if (Bits==8) {
    drv_HD_PP_command (allControllers, 0x30, T_INIT1); /* 8 Bit mode, wait 4.1 ms */
    drv_HD_PP_command (allControllers, 0x30, T_INIT2); /* 8 Bit mode, wait 100 us */
    drv_HD_PP_command (allControllers, 0x38, T_EXEC);  /* 8 Bit mode, 1/16 duty cycle, 5x8 font */
  } else {
    drv_HD_PP_nibble  (allControllers, 0x03); udelay(T_INIT1); /* 4 Bit mode, wait 4.1 ms */
    drv_HD_PP_nibble  (allControllers, 0x03); udelay(T_INIT2); /* 4 Bit mode, wait 100 us */
    drv_HD_PP_nibble  (allControllers, 0x03); udelay(T_INIT1); /* 4 Bit mode, wait 4.1 ms */
    drv_HD_PP_nibble  (allControllers, 0x02); udelay(T_INIT2); /* 4 Bit mode, wait 100 us */
    drv_HD_PP_command (allControllers, 0x28, T_EXEC);	       /* 4 Bit mode, 1/16 duty cycle, 5x8 font */
  }

  /* maybe use busy-flag from now on  */
  /* (we can't use the busy flag during the init sequence) */
  cfg_number(section, "UseBusy", 0, 0, 1, &UseBusy);
  
  /* make sure we don't use the busy flag with RW wired to GND */
  if (UseBusy && !SIGNAL_RW)   {
    error("%s: busy-flag checking is impossible with RW wired to GND!", Name);
    UseBusy=0;
  }

  /* make sure the display supports busy-flag checking in 4-Bit-Mode */
  /* at the moment this is inly possible with Martin Hejl's gpio driver, */
  /* which allows to use 4 bits as input and 4 bits as output */
  if (UseBusy && Bits==4 && !(Capabilities&CAP_BUSY4BIT)) {
    error("%s: Model '%s' does not support busy-flag checking in 4 bit mode", Name, Models[Model].name);
    UseBusy=0;
  }
  
  info("%s: %susing busy-flag checking", Name, UseBusy ? "" : "not ");

  /* The LCM-162 should really use BusyFlag checking */
  if (!UseBusy && (Capabilities & CAP_LCM162))   {
    error("%s: Model '%s' should definitely use busy-flag checking!", Name, Models[Model].name);
  }
  
  return 0;
}


static void drv_HD_PP_stop (void) 
{
  /* clear all signals */
  if (Bits==8) {
    drv_generic_parport_control (SIGNAL_RS|SIGNAL_RW|SIGNAL_ENABLE|SIGNAL_ENABLE2|SIGNAL_GPO, 0);
  } else {
    drv_generic_parport_data (0);
  }

  drv_generic_parport_close();
  
}



/****************************************/
/***  i2c dependant functions         ***/
/****************************************/


static void drv_HD_I2C_command (const unsigned char controller, const unsigned char cmd, const int delay)
{
  /* send data with RS disabled */
  // drv_HD_I2C_byte (controller, cmd, 0);
}


static void drv_HD_I2C_data (const unsigned char controller, const char *string, const int len, const int delay)
{
  int l = len;
  unsigned char enable;
  
  /* sanity check */
  if (len<=0) return;
  
  while (l--) {
    /* send data with RS enabled */
    // drv_HD_I2C_byte (controller, *(string++), SIGNAL_RS);
  }
}


static int drv_HD_I2C_load (const char *section)
{
  return 0;
}


static void drv_HD_I2C_stop (void) 
{
  /* clear all signals */
  // drv_generic_i2c_data (0);
  /* slose port */
  // drv_generic_i2c_close();
}



/****************************************/
/***  display dependant functions     ***/
/****************************************/

static void drv_HD_clear (void)
{
  drv_HD_command (allControllers, 0x01, T_CLEAR); /* clear *both* displays */
}


static void drv_HD_goto (int row, int col)
{
  int pos;

  /* handle multiple displays/controllers */
  if (numControllers>1 && row>=DROWS/2) {
    row -= DROWS/2;
    currController = 2;
  } else {
    currController = 1;
  }
   
  /* 16x1 Displays are organized as 8x2 :-( */
  if (DCOLS==16 && DROWS==1 && col>7) {
    row++;
    col-=8;
  }
  
  if (Capabilities & CAP_HD66712) {
    /* the HD66712 doesn't have a braindamadged RAM layout */
    pos = row*32 + col;
  } else {
    /* 16x4 Displays use a slightly different layout */
    if (DCOLS==16 && DROWS==4) {
      pos = (row%2)*64+(row/2)*16+col;
    } else {  
      pos = (row%2)*64+(row/2)*20+col;
    }
  }
  drv_HD_command (currController, (0x80|pos), T_EXEC);
}


static void drv_HD_write (const int row, const int col, const char *data, const int len)
{
  drv_HD_goto (row, col);
  drv_HD_data (currController, data, len, T_EXEC);
}


static void drv_HD_defchar (const int ascii, const unsigned char *matrix)
{
  int i;
  char buffer[8];

  for (i = 0; i < 8; i++) {
    buffer[i] = matrix[i] & 0x1f;
  }
  
  /* define chars on *both* controllers! */
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
  
  drv_HD_command (allControllers, 0x38, T_EXEC); /* enable function */
  drv_HD_data (allControllers, &cmd, 1, T_WRCG); /* set brightness */

  return brightness;
}

  
/* Fixme: GPO's */
#if 0
static void drv_HD_setGPO (const int bits)
{
  if (Lcd.gpos>0) {
    
    /* put data on DB1..DB8 */
    drv_generic_parport_data (bits);
    
    /* 74HCT573 set-up time */
    ndelay(20);
    
    /* send data */
    /* 74HCT573 enable pulse width = 24ns */
    drv_generic_parport_toggle (SIGNAL_GPO, 1, 230);
  }
}
#endif


static int drv_HD_LCM162_keypad (void)
{
  static unsigned char data = 0x00;
  
  if (!(Capabilities & CAP_LCM162)) return -1;
  
}

  
static int drv_HD_start (const char *section, const int quiet)
{
  char *model, *size, *bus;
  int rows=-1, cols=-1, gpos=-1;
  
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
    Capabilities=Models[Model].capabilities;
    info ("%s: using model '%s'", Name, Models[Model].name);
  } else {
    error ("%s: empty '%s.Model' entry from %s", Name, section, cfg_source());
    if (model) free (model);
    return -1;
  }
  free (model);

  bus=cfg_get(section, "Bus", "parport");
  if (bus==NULL && *bus=='\0') {
    error ("%s: empty '%s.Bus' entry from %s", Name, section, cfg_source());
    if (bus) free (bus);
    return -1;
  }

  if (strcasecmp(bus, "parport") == 0) {
    info ("%s: using parallel port", Name);
    Bus = BUS_PP;
    drv_HD_load    = drv_HD_PP_load;
    drv_HD_command = drv_HD_PP_command;
    drv_HD_data    = drv_HD_PP_data;
    drv_HD_stop    = drv_HD_PP_stop;
    
  } else if (strcasecmp(bus, "i2c") == 0) {
    info ("%s: using I2C bus", Name);
    Bus = BUS_I2C;
    drv_HD_load    = drv_HD_I2C_load;
    drv_HD_command = drv_HD_I2C_command;
    drv_HD_data    = drv_HD_I2C_data;
    drv_HD_stop    = drv_HD_I2C_stop;

  } else {
    error ("%s: bad %s.Bus '%s' from %s, should be 'parport' or 'i2c'", Name, section, bus, cfg_source());
    free (bus);
    return -1;
  }

  /* sanity check: Model can use bus */
  if (!(Capabilities & Bus)) {
    error ("%s: Model '%s' cannot be used on the %s bus!", Name, Models[Model].name, bus);
    free (bus);
    return -1;
  }
  free(bus);
  
  size=cfg_get(section, "Size", NULL);
  if (size==NULL || *size=='\0') {
    error ("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
    free(size);
    return -1;
  }
  if (sscanf(size,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("%s: bad %s.Size '%s' from %s", Name, section, size, cfg_source());
    free(size);
    return -1;
  }
  free(size);
  DROWS = rows;
  DCOLS = cols;
  
  if (cfg_number(section, "Controllers", 1, 1, 2, &numControllers)<0) return -1;
  info ("%s: using %d controller(s)", Name, numControllers);
  
  /* current controller */
  currController=1;
  
  /* Bitmask for *all* Controllers */
  allControllers = numControllers==2 ? 3 : 1;
  
  if (cfg_number(section, "GPOs", 0, 0, 8, &gpos)<0) return -1;
  if (gpos > 0 && !(Capabilities & CAP_GPO)) {
    error ("%s: Model '%s' does not support GPO's!", Name, Models[Model].name);
    gpos = 0;
  }
  GPOS  = gpos;
  if (gpos > 0) {
    info ("%s: using %d GPO's", Name, GPOS);
  }
  
  if (drv_HD_load(section) < 0) {
    error ("%s: start display failed!", Name);
    return -1;
  }
  
  drv_HD_command (allControllers, 0x08, T_EXEC);  /* Display off, cursor off, blink off */
  drv_HD_command (allControllers, 0x0c, T_CLEAR); /* Display on, cursor off, blink off, wait 1.64 ms */
  drv_HD_command (allControllers, 0x06, T_EXEC);  /* curser moves to right, no shift */

  if ((Capabilities & CAP_HD66712) && DROWS > 2) {
    drv_HD_command (allControllers, Bits==8?0x3c:0x2c, T_EXEC); /* set extended register enable bit */
    drv_HD_command (allControllers, 0x09,              T_EXEC); /* set 4-line mode */
    drv_HD_command (allControllers, Bits==8?0x38:0x28, T_EXEC); /* clear extended register enable bit */
  }

  drv_HD_clear(); /* clear *both* displays */
  drv_HD_command (allControllers, 0x03, T_CLEAR); /* return home */
  
  /* maybe set brightness */
  if (Capabilities & CAP_BRIGHTNESS) {
    int brightness;
    if (cfg_number(section, "Brightness", 0, 0, 3, &brightness)>0) {
      drv_HD_brightness(brightness);
    }
  }

  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer, NULL)) {
      sleep (3);
      drv_HD_clear();
    }
  }
    
  return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/


static void plugin_brightness (RESULT *result, RESULT *arg1)
{
  double brightness;
  
  brightness=drv_HD_brightness(R2N(arg1));
  SetResult(&result, R_NUMBER, &brightness); 
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
int drv_HD_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


/* initialize driver & display */
int drv_HD_init (const char *section, const int quiet)
{
  WIDGET_CLASS wc;
  int asc255bug;
  int ret;  
  
  /* display preferences */
  XRES  = 5;      /* pixel width of one char  */
  YRES  = 8;      /* pixel height of one char  */
  CHARS = 8;      /* number of user-defineable characters */
  CHAR0 = 0;      /* ASCII of first user-defineable char */
  GOTO_COST = 2;  /* number of bytes a goto command requires */
  
  /* real worker functions */
  drv_generic_text_real_write   = drv_HD_write;
  drv_generic_text_real_defchar = drv_HD_defchar;


  /* start display */
  if ((ret=drv_HD_start (section, quiet))!=0)
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
  if (Capabilities & CAP_BRIGHTNESS) {
    AddFunction ("LCD::brightness", 1, plugin_brightness);
  }
  
  return 0;
}


/* close driver & display */
int drv_HD_quit (const int quiet) {

  info("%s: shutting down.", Name);

  drv_generic_text_quit();

  /* clear *both* displays */
  drv_HD_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }
  
  drv_HD_stop();
  
  return (0);
}


DRIVER drv_HD44780 = {
  name: Name,
  list: drv_HD_list,
  init: drv_HD_init,
  quit: drv_HD_quit, 
};


