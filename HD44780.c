/* $Id: HD44780.c,v 1.46 2003/10/08 06:48:47 nicowallmeier Exp $
 *
 * driver for display modules based on the HD44780 chip
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 *
 * This file is part of LCD4Linux.
 *
 * Modification for 4-Bit mode
 * 2003 Martin Hejl (martin@hejl.de)
 *
 * Modification for 2nd controller support
 * 2003 Jesse Brook Kovach <jkovach@wam.umd.edu>
 *
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
 * $Log: HD44780.c,v $
 * Revision 1.46  2003/10/08 06:48:47  nicowallmeier
 * special handling for 16x4 displays
 *
 * Revision 1.45  2003/10/08 06:45:00  nicowallmeier
 * Support of two displays of the same size
 *
 * Revision 1.44  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.43  2003/09/29 06:12:56  reinelt
 * changed default HD44780 wiring: unused signals are GND
 *
 * Revision 1.42  2003/09/21 06:43:02  reinelt
 *
 *
 * MatrixOrbital: bidirectional communication
 * HD44780: special handling for 16x1 displays (thanks to Maciej Witkowiak)
 *
 * Revision 1.41  2003/09/13 06:20:39  reinelt
 * HD44780 timings changed; deactivated libtool
 *
 * Revision 1.40  2003/09/11 04:09:52  reinelt
 * minor cleanups
 *
 * Revision 1.39  2003/09/10 03:48:22  reinelt
 * Icons for M50530, new processing scheme (Ticks.Text...)
 *
 * Revision 1.38  2003/09/09 11:47:47  reinelt
 * basic icon support for HD44780
 *
 * Revision 1.37  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.36  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.35  2003/08/22 03:45:08  reinelt
 * bug in parallel port code fixed, more icons stuff
 *
 * Revision 1.34  2003/08/19 05:23:55  reinelt
 * HD44780 dual-controller patch from Jesse Brook Kovach
 *
 * Revision 1.33  2003/08/19 04:28:41  reinelt
 * more Icon stuff, minor glitches fixed
 *
 * Revision 1.32  2003/08/16 07:31:35  reinelt
 * double buffering in all drivers
 *
 * Revision 1.31  2003/08/15 07:54:07  reinelt
 * HD44780 4 bit mode implemented
 *
 * Revision 1.30  2003/08/12 05:10:31  reinelt
 * first version of HD44780 4Bit-Mode patch
 *
 * Revision 1.29  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.28  2003/04/07 06:02:58  reinelt
 * further parallel port abstraction
 *
 * Revision 1.27  2003/04/04 06:01:59  reinelt
 * new parallel port abstraction scheme
 *
 * Revision 1.26  2003/02/22 07:53:09  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.25  2002/08/19 09:11:34  reinelt
 * changed HD44780 to use generic bar functions
 *
 * Revision 1.24  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.23  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.22  2002/08/17 14:14:21  reinelt
 *
 * USBLCD fixes
 *
 * Revision 1.21  2002/04/30 07:20:15  reinelt
 *
 * implemented the new ndelay(nanoseconds) in all parallel port drivers
 *
 * Revision 1.20  2001/09/09 12:26:03  reinelt
 * GPO bug: INIT is _not_ inverted
 *
 * Revision 1.19  2001/03/15 15:49:22  ltoetsch
 * fixed compile HD44780.c, cosmetics
 *
 * Revision 1.18  2001/03/15 09:47:13  reinelt
 *
 * some fixes to ppdef
 * off-by-one bug in processor.c fixed
 *
 * Revision 1.17  2001/03/14 16:47:41  reinelt
 * minor cleanups
 *
 * Revision 1.16  2001/03/14 15:30:53  reinelt
 *
 * make ppdev compatible to earlier kernels
 *
 * Revision 1.15  2001/03/14 15:14:59  reinelt
 * added ppdev parallel port access
 *
 * Revision 1.14  2001/03/12 13:44:58  reinelt
 *
 * new udelay() using Time Stamp Counters
 *
 * Revision 1.13  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.12  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.11  2001/02/13 12:43:24  reinelt
 *
 * HD_gpo() was missing
 *
 * Revision 1.10  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.9  2000/10/20 07:17:07  reinelt
 *
 *
 * corrected a bug in HD_goto()
 * Thanks to Gregor Szaktilla <gregor@szaktilla.de>
 *
 * Revision 1.8  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.7  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.6  2000/07/31 10:43:44  reinelt
 *
 * some changes to support kernel-2.4 (different layout of various files in /proc)
 *
 * Revision 1.5  2000/07/31 06:46:35  reinelt
 *
 * eliminated some compiler warnings with glibc
 *
 * Revision 1.4  2000/04/15 16:56:52  reinelt
 *
 * moved delay loops to udelay.c
 * renamed -d (debugging) switch to -v (verbose)
 * new switch -d to calibrate delay loop
 * 'Delay' entry for HD44780 back again
 * delay loops will not calibrate automatically, because this will fail with hich CPU load
 *
 * Revision 1.3  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 * Revision 1.2  2000/04/13 06:09:52  reinelt
 *
 * added BogoMips() to system.c (not used by now, maybe sometimes we can
 * calibrate our delay loop with this value)
 *
 * added delay loop to HD44780 driver. It seems to be quite fast now. Hopefully
 * no compiler will optimize away the delay loop!
 *
 * Revision 1.1  2000/04/12 08:05:45  reinelt
 *
 * first version of the HD44780 driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD HD44780[]
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "icon.h"
#include "parport.h"
#include "udelay.h"

#define XRES 5
#define YRES 8
#define CHARS 8



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


static LCD Lcd;
static int Icons;
static int Bits=0;
static int GPO=0;
static int Controllers = 0;
static int Controller  = 0;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;

static unsigned char SIGNAL_RW;
static unsigned char SIGNAL_RS;
static unsigned char SIGNAL_ENABLE;
static unsigned char SIGNAL_ENABLE2;
static unsigned char SIGNAL_GPO;

static void HD_nibble(unsigned char controller, unsigned char nibble)
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
  parport_data(nibble);

  // Address set-up time
  ndelay(T_AS);
  
  // rise ENABLE
  parport_data(nibble | enable);
  
  // Enable pulse width
  ndelay(T_PW);

  // lower ENABLE
  parport_data(nibble);
}


static void HD_byte (unsigned char controller, unsigned char data, unsigned char RS)
{
  // send high nibble of the data
  HD_nibble (controller, ((data>>4)&0x0f)|RS);

  // Make sure we honour T_CYCLE
  ndelay(T_CYCLE-T_AS-T_PW);

  // send low nibble of the data
  HD_nibble(controller, (data&0x0f)|RS);
}


static void HD_command (unsigned char controller, unsigned char cmd, int delay)
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
    parport_data (cmd);
  
    // clear RW and RS
    parport_control (SIGNAL_RW | SIGNAL_RS, 0);
    
    // Address set-up time
    ndelay(T_AS);

    // send command
    parport_toggle (enable, 1, T_PW);
    
  } else {

    HD_byte (controller, cmd, 0);

  }
  
  // wait for command completion
  udelay(delay);

}


static void HD_write (unsigned char controller, char *string, int len, int delay)
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
    parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);

    // Address set-up time
    ndelay(T_AS);
    
    while (len--) {
      
      // put data on DB1..DB8
      parport_data (*(string++));
      
      // send command
      parport_toggle (enable, 1, T_PW);
      
      // wait for command completion
      udelay(delay);
    }
    
  } else { // 4 bit mode
    
    while (len--) {

      // send data with RS enabled
      HD_byte (controller, *(string++), SIGNAL_RS);
      
      // wait for command completion
      udelay(delay);
    }
  }
}


static void HD_setGPO (int bits)
{
  if (Lcd.gpos>0) {
    
    // put data on DB1..DB8
    parport_data (bits);
    
    // 74HCT573 set-up time
    ndelay(20);
    
    // send data
    // 74HCT573 enable pulse width = 24ns
    parport_toggle (SIGNAL_GPO, 1, 230);
  }
}


static void HD_define_char (int ascii, char *buffer)
{
  // define chars on *both* controllers!
  HD_command (0x03, 0x40|8*ascii, T_EXEC);
  HD_write (0x03, buffer, 8, T_WRCG);
}


int HD_clear (int full)
{

  memset (FrameBuffer1, ' ', Lcd.rows*Lcd.cols*sizeof(char));

  icon_clear();
  bar_clear();

  GPO=0;
  
  if (full) {
    memset (FrameBuffer2, ' ', Lcd.rows*Lcd.cols*sizeof(char));
    HD_command (0x03, 0x01, T_CLEAR); // clear *both* displays
    HD_command (0x03, 0x03, T_CLEAR); // return home
    HD_setGPO (GPO);                  // all GPO's off
  }
  
  return 0;
}


int HD_init (LCD *Self)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s;
  
  s=cfg_get("Size",NULL);
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Size' entry in %s", cfg_source());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("HD44780: bad size '%s'",s);
    return -1;
  }
  
  if (cfg_number("GPOs", 0, 0, 8, &gpos)<0) return -1;
  info ("HD44780: controlling %d GPO's", gpos);

  if (cfg_number("Controllers", 1, 1, 2, &Controllers)<0) return -1;
  info ("wiring: using display with %d controllers", Controllers);
  
  // current controller
  Controller=1;
  
  Self->rows=rows;
  Self->cols=cols;
  Self->gpos=gpos;
  Lcd=*Self;
  
  // Init the framebuffers
  FrameBuffer1 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  FrameBuffer2 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("HD44780: framebuffer could not be allocated: malloc() failed");
    return -1;
  }
  
  if (cfg_number("Bits", 8, 4, 8, &Bits)<0) return -1;
  if (Bits!=4 && Bits!=8) {
    error ("HD44780: bad Bits '%s' in %s, should be '4' or '8'", s, cfg_source());
    return -1;
  }    
  info ("wiring: using %d bit mode", Bits);
  
  if (Bits==8) {
    if ((SIGNAL_RS      = parport_wire_ctrl ("RS",      "AUTOFD"))==0xff) return -1;
    if ((SIGNAL_RW      = parport_wire_ctrl ("RW",      "GND"   ))==0xff) return -1;
    if ((SIGNAL_ENABLE  = parport_wire_ctrl ("ENABLE",  "STROBE"))==0xff) return -1;
    if ((SIGNAL_ENABLE2 = parport_wire_ctrl ("ENABLE2", "GND"   ))==0xff) return -1;
    if ((SIGNAL_GPO     = parport_wire_ctrl ("GPO",     "GND"   ))==0xff) return -1;
  } else {
    if ((SIGNAL_RS      = parport_wire_data ("RS",      "DB4"))==0xff) return -1;
    if ((SIGNAL_RW      = parport_wire_data ("RW",      "DB5"))==0xff) return -1;
    if ((SIGNAL_ENABLE  = parport_wire_data ("ENABLE",  "DB6"))==0xff) return -1;
    if ((SIGNAL_ENABLE2 = parport_wire_data ("ENABLE2", "GND"))==0xff) return -1;
    if ((SIGNAL_GPO     = parport_wire_data ("GPO",     "GND"))==0xff) return -1;
  }
  
  if (parport_open() != 0) {
    error ("HD44780: could not initialize parallel port!");
    return -1;
  }

  // clear all signals
  if (Bits==8) {
    parport_control (SIGNAL_RS|SIGNAL_RW|SIGNAL_ENABLE|SIGNAL_ENABLE2|SIGNAL_GPO, 0);
  } else {
    parport_data (0);
  }

  // set direction: write
  parport_direction (0);

  // initialize *both* displays
  if (Bits==8) {
    HD_command (0x03, 0x30, T_INIT1); // 8 Bit mode, wait 4.1 ms
    HD_command (0x03, 0x30, T_INIT2); // 8 Bit mode, wait 100 us
    HD_command (0x03, 0x38, T_EXEC);  // 8 Bit mode, 1/16 duty cycle, 5x8 font
  } else {
    HD_nibble(0x03, 0x03); udelay(T_INIT1); // 4 Bit mode, wait 4.1 ms
    HD_nibble(0x03, 0x03); udelay(T_INIT2); // 4 Bit mode, wait 100 us
    HD_nibble(0x03, 0x03); udelay(T_INIT1); // 4 Bit mode, wait 4.1 ms
    HD_nibble(0x03, 0x02); udelay(T_INIT2); // 4 Bit mode, wait 100 us
    HD_command (0x03, 0x28, T_EXEC);	    // 4 Bit mode, 1/16 duty cycle, 5x8 font
  }
  
  HD_command (0x03, 0x08, T_EXEC);  // Display off, cursor off, blink off
  HD_command (0x03, 0x0c, T_CLEAR); // Display on, cursor off, blink off, wait 1.64 ms
  HD_command (0x03, 0x06, T_EXEC);  // curser moves to right, no shift

  if (cfg_number("Icons", 0, 0, CHARS, &Icons)<0) return -1;
  if (Icons>0) {
    debug ("reserving %d of %d user-defined characters for icons", Icons, CHARS);
    icon_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS, Icons, HD_define_char);
    Self->icons=Icons;
    Lcd.icons=Icons;
  }
  
  bar_init(rows, cols, XRES, YRES, CHARS-Icons);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block
  
  HD_clear(1);
  
  return 0;
}


void HD_goto (int row, int col)
{
  int pos;

  if (Controllers>1 && row>=Lcd.rows/2) {
    row -= Lcd.rows/2;
    Controller = 2;
  } else {
    Controller = 1;
  }
   
  // 16x1 Displays are organized as 8x2 :-(
  if (Lcd.rows==1 && Lcd.cols==16 && col>7) {
    row++;
    col-=8;
  }
  
  if (Lcd.rows==4 && Lcd.cols==16) {
  	pos=(row%2)*64+(row/2)*16+col;
  } else {  
    pos=(row%2)*64+(row/2)*20+col;
  }
  
  HD_command (Controller, (0x80|pos), T_EXEC);
}


int HD_put (int row, int col, char *text)
{
  char *p=FrameBuffer1+row*Lcd.cols+col;
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int HD_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int HD_icon (int num, int seq, int row, int col)
{
  return icon_draw (num, seq, row, col);
}


int HD_gpo (int num, int val)
{
  if (num>=Lcd.gpos) 
    return -1;

  if (val) {
    GPO |= 1<<num;     // set bit
  } else {
    GPO &= ~(1<<num);  // clear bit
  }
  return 0;
}


int HD_flush (void)
{
  int row, col, pos1, pos2;
  int c, equal;
  
  bar_process(HD_define_char);

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c==-1) c=icon_peek(row, col);
      if (c!=-1) {
	FrameBuffer1[row*Lcd.cols+col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) continue;
      HD_goto (row, col);
      for (pos1=col++, pos2=pos1, equal=0; col<Lcd.cols; col++) {
	if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes one byte, too.
	  if (++equal>2) break;
	} else {
	  pos2=col;
	  equal=0;
	}
	// special handling of 16x1 displays
	if (Lcd.rows==1 && Lcd.cols==16 && col==7) {
	  break;
	}
      }
      HD_write (Controller, FrameBuffer1+row*Lcd.cols+pos1, pos2-pos1+1, T_EXEC);
    }
  }

  memcpy (FrameBuffer2, FrameBuffer1, Lcd.rows*Lcd.cols*sizeof(char));

  HD_setGPO(GPO);

  return 0;
}


int HD_quit (void)
{
  info("HD44780: shutting down.");

  if (FrameBuffer1) {
    free(FrameBuffer1);
    FrameBuffer1=NULL;
  }

  if (FrameBuffer2) {
    free(FrameBuffer2);
    FrameBuffer2=NULL;
  }

  return parport_close();
}


LCD HD44780[] = {
  { name: "HD44780",
    rows:  0,
    cols:  0,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  HD_init,
    clear: HD_clear,
    put:   HD_put,
    bar:   HD_bar,
    icon:  HD_icon,
    gpo:   HD_gpo,
    flush: HD_flush,
    quit:  HD_quit 
  },
  { NULL }
};
