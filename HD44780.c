/* $Id: HD44780.c,v 1.30 2003/08/12 05:10:31 reinelt Exp $
 *
 * driver for display modules based on the HD44780 chip
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
 *
 * Modification for 4-Bit mode
 * 2003 Martin Hejl (martin@hejl.de)
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
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

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "parport.h"
#include "udelay.h"

#define XRES 5
#define YRES 8
#define CHARS 8



/* low level communication timings [nanoseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_CYCLE 600 // Enable cycle time
#define T_PW    400 // Enable pulse width
#define T_AS     20 // Address setup time
#define T_H      40 // Data hold time
// Fixme: hejl: genau verdreht??


static LCD Lcd;

static char Txt[4][40];
static int  GPO=0;

static unsigned char SIGNAL_RW;
static unsigned char SIGNAL_RS;
static unsigned char SIGNAL_ENABLE;
static unsigned char SIGNAL_GPO;


static void HD_nibble(unsigned char nibble)
{

  // clear ENABLE
  // put data on DB1..DB4
  // nibble already contains RS bit!
  parport_data(nibble);

  // Address set-up time
  ndelay(T_AS);
  
  // rise ENABLE
  parport_data(nibble | SIGNAL_ENABLE);
  
  // Enable pulse width
  ndelay(T_PW);

  // lower ENABLE
  parport_data(nibble);
}


static void HD_byte (unsigned char data, unsigned char RS)
{
  // send high nibble of the data
  HD_nibble (((data>>4)&0x0f)|RS);

  // Make sure we honour T_CYCLE
  ndelay(T_CYCLE-T_AS-T_PW);

  // send low nibble of the data
  HD_nibble((data&0x0f)|RS);
}


static void HD_command (unsigned char cmd, int delay)
{
    
  // put data on DB1..DB8
  parport_data (cmd);
  
  // clear RW and RS
  parport_control (SIGNAL_RW | SIGNAL_RS, 0);
    
  // Address set-up time
  ndelay(T_AS);

  // send command
  parport_toggle (SIGNAL_ENABLE, 1, T_PW);
    
  // wait for command completion
  udelay(delay);

}


static void HD_write (char *string, int len, int delay)
{
  // clear RW, set RS
  parport_control (SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);

  // Address set-up time
  ndelay(40);
  
  while (len--) {
    
    // put data on DB1..DB8
    parport_data (*(string++));
    
    // send command
    // Enable cycle time = 230ns
    parport_toggle (SIGNAL_ENABLE, 1, 230);
    
    // wait for command completion
    udelay(delay);
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
  HD_command (0x40|8*ascii, 40);
  HD_write (buffer, 8, 120); // 120 usec delay for CG RAM write
}


int HD_clear (int full)
{
  int row, col;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
    }
  }

  bar_clear();

  GPO=0;

  if (full) {
    HD_command (0x01, 1640); // clear display
    HD_setGPO (GPO);         // All GPO's off
  }

  return 0;
}


int HD_init (LCD *Self)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s, *e;
  
  s=cfg_get("Size",NULL);
  if (s==NULL || *s=='\0') {
    error ("HD44780: no 'Size' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("HD44780: bad size '%s'",s);
    return -1;
  }

  s=cfg_get ("GPOs",NULL);
  if (s==NULL) {
    gpos=0;
  }
  else if ((gpos=strtol(s, &e, 0))==0 || *e!='\0' || gpos<0 || gpos>8) {
    error ("HD44780: bad GPOs '%s' in %s", s, cfg_file());
    return -1;
  }    
  
  Self->rows=rows;
  Self->cols=cols;
  Self->gpos=gpos;
  Lcd=*Self;
  
  if ((SIGNAL_RW=parport_wire ("RW", "GND"))==0xff) return -1;
  if ((SIGNAL_RS=parport_wire ("RS", "AUTOFD"))==0xff) return -1;
  if ((SIGNAL_ENABLE=parport_wire ("ENABLE", "STROBE"))==0xff) return -1;
  if ((SIGNAL_GPO=parport_wire ("GPO", "INIT"))==0xff) return -1;

  if (parport_open() != 0) {
    error ("HD44780: could not initialize parallel port!");
    return -1;
  }

  // clear RW
  parport_control (SIGNAL_RW, 0);

  // set direction: write
  parport_direction (0);

  // initialize display
  HD_command (0x30, 4100); // 8 Bit mode, wait 4.1 ms
  HD_command (0x30, 100);  // 8 Bit mode, wait 100 us
  HD_command (0x30, 4100); // 8 Bit mode, wait 4.1 ms
  HD_command (0x38, 40);   // 8 Bit mode, 1/16 duty cycle, 5x8 font
  HD_command (0x08, 40);   // Display off, cursor off, blink off
  HD_command (0x0c, 1640); // Display on, cursor off, blink off, wait 1.64 ms
  HD_command (0x06, 40);   // curser moves to right, no shift


  bar_init(rows, cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block
  
  HD_clear(1);
  
  return 0;
}


void HD_goto (int row, int col)
{
  int pos;

  pos=(row%2)*64+(row/2)*20+col;
  HD_command (0x80|pos, 40);
}


int HD_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
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
  char buffer[256];
  char *p;
  int c, row, col;
  
  bar_process(HD_define_char);

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	Txt[row][col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      HD_goto (row, col);
      for (p=buffer; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      HD_write (buffer, p-buffer, 40); // 40 usec delay for write
    }
  }

  HD_setGPO(GPO);

  return 0;
}


int HD_quit (void)
{
  info("HD44780: shutting down.");
  return parport_close();
}


LCD HD44780[] = {
  { name: "HD44780",
    rows:  0,
    cols:  0,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  HD_init,
    clear: HD_clear,
    put:   HD_put,
    bar:   HD_bar,
    gpo:   HD_gpo,
    flush: HD_flush,
    quit:  HD_quit 
  },
  { NULL }
};
