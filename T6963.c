/* $Id: T6963.c,v 1.12 2003/09/29 06:12:56 reinelt Exp $
 *
 * driver for display modules based on the Toshiba T6963 chip
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: T6963.c,v $
 * Revision 1.12  2003/09/29 06:12:56  reinelt
 * changed default HD44780 wiring: unused signals are GND
 *
 * Revision 1.11  2003/09/13 06:45:43  reinelt
 * icons for all remaining drivers
 *
 * Revision 1.10  2003/08/16 07:31:35  reinelt
 * double buffering in all drivers
 *
 * Revision 1.9  2003/08/15 07:54:07  reinelt
 * HD44780 4 bit mode implemented
 *
 * Revision 1.8  2003/08/01 05:15:42  reinelt
 * last cleanups for 0.9.9
 *
 * Revision 1.7  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.6  2003/04/07 06:03:00  reinelt
 * further parallel port abstraction
 *
 * Revision 1.5  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.4  2002/08/21 06:09:53  reinelt
 * some T6963 fixes, ndelay wrap
 *
 * Revision 1.3  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.2  2002/08/17 12:54:08  reinelt
 * minor T6963 changes
 *
 * Revision 1.1  2002/04/29 11:00:26  reinelt
 *
 * added Toshiba T6963 driver
 * added ndelay() with nanosecond resolution
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD T6963[]
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "icon.h"
#include "parport.h"
#include "udelay.h"
#include "pixmap.h"


#define XRES 6
#define YRES 8

static LCD Lcd;
static int Icons;

unsigned char *Buffer1, *Buffer2;

static unsigned char SIGNAL_CE;
static unsigned char SIGNAL_CD;
static unsigned char SIGNAL_RD;
static unsigned char SIGNAL_WR;


// Fixme:
static int bug=0;

// perform normal status check
void T6_status1 (void)
{
  int n;
  
  // turn off data line drivers
  parport_direction (1);
  
  // lower CE and RD
  parport_control (SIGNAL_CE | SIGNAL_RD, 0);
  
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
  } while ((parport_read() & 0x03) != 0x03);
  
  // rise RD and CE
  parport_control (SIGNAL_RD | SIGNAL_CE, SIGNAL_RD | SIGNAL_CE);
  
  // Output Hold Time: 50 ns 
  ndelay(50);
  
  // turn on data line drivers
  parport_direction (0);
}


// perform status check in "auto mode"
void T6_status2 (void)
{
  int n;

  // turn off data line drivers
  parport_direction (1);
  
  // lower RD and CE
  parport_control (SIGNAL_RD | SIGNAL_CE, 0);
  
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
  } while ((parport_read() & 0x08) != 0x08);

  // rise RD and CE
  parport_control (SIGNAL_RD | SIGNAL_CE, SIGNAL_RD | SIGNAL_CE);

  // Output Hold Time: 50 ns 
  ndelay(50);

  // turn on data line drivers
  parport_direction (0);
}


static void T6_write_cmd (unsigned char cmd)
{
  // wait until the T6963 is idle
  T6_status1();

  // put data on DB1..DB8
  parport_data (cmd);
  
  // lower WR and CE
  parport_control (SIGNAL_WR | SIGNAL_CE, 0);
  
  // Pulse width
  ndelay(80);

  // rise WR and CE
  parport_control (SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

  // Data Hold Time
  ndelay(40);
}


static void T6_write_data (unsigned char data)
{
  // wait until the T6963 is idle
  T6_status1();

  // put data on DB1..DB8
  parport_data (data);

  // lower C/D
  parport_control (SIGNAL_CD, 0);
  
  // C/D Setup Time
  ndelay(20);

  // lower WR and CE
  parport_control (SIGNAL_WR | SIGNAL_CE, 0);
  
  // Pulse Width
  ndelay(80);
  
  // rise WR and CE
  parport_control (SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);
  
  // Data Hold Time
  ndelay(40);

  // rise CD
  parport_control (SIGNAL_CD, SIGNAL_CD);
}


static void T6_write_auto (unsigned char data)
{
  // wait until the T6963 is idle
  T6_status2();

  // put data on DB1..DB8
  parport_data (data);

  // lower C/D
  parport_control (SIGNAL_CD, 0);
  
  // C/D Setup Time
  ndelay(20);

  // lower WR and CE
  parport_control (SIGNAL_WR | SIGNAL_CE, 0);
  
  // Pulse Width
  ndelay(80);
  
  // rise WR and CE
  parport_control (SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);
  
  // Data Hold Time
  ndelay(40);

  // rise CD
  parport_control (SIGNAL_CD, SIGNAL_CD);
}


#if 0 // not used
static void T6_send_byte (unsigned char cmd, unsigned char data)
{
  T6_write_data(data);
  T6_write_cmd(cmd);
}
#endif

static void T6_send_word (unsigned char cmd, unsigned short data)
{
  T6_write_data(data&0xff);
  T6_write_data(data>>8);
  T6_write_cmd(cmd);
}


static void T6_memset(unsigned short addr, unsigned char data, int len)
{
  int i;

  T6_send_word (0x24, addr);      // Set Adress Pointer
  T6_write_cmd(0xb0);             // Set Data Auto Write
  for (i=0; i<len; i++) {
    T6_write_auto(data);
    if (bug) {
      debug("bug occured at byte %d of %d", i, len);
      bug=0;
    }
  }
  T6_status2();
  T6_write_cmd(0xb2);             // Auto Reset
}


static void T6_memcpy(unsigned short addr, unsigned char *data, int len)
{
  int i;

  T6_send_word (0x24, 0x0200+addr);  // Set Adress Pointer
  T6_write_cmd(0xb0);                // Set Data Auto Write
  for (i=0; i<len; i++) {
    T6_write_auto(*(data++));
    if (bug) {
      debug("bug occured at byte %d of %d, addr=%d", i, len, addr);
      bug=0;
    }
  }
  T6_status2();
  T6_write_cmd(0xb2);                // Auto Reset
}


int T6_clear (int full)
{
  int rows;
  
  if (full) {
    
    rows=(Lcd.rows>8 ? 8 : Lcd.rows);
    
    T6_memset(0x0000, 0, Lcd.cols*rows);    // clear text area 
    T6_memset(0x0200, 0, Lcd.cols*rows*8);  // clear graphic area
    
    if (Lcd.rows>8) {
      T6_memset(0x8000, 0, Lcd.cols*(Lcd.rows-rows));    // clear text area #2
      T6_memset(0x8200, 0, Lcd.cols*(Lcd.rows-rows)*8);  // clear graphic area #2
    }
    
    memset(Buffer1,0,Lcd.cols*Lcd.rows*Lcd.yres*sizeof(*Buffer1));
    memset(Buffer2,0,Lcd.cols*Lcd.rows*Lcd.yres*sizeof(*Buffer2));
  }
  
  return pix_clear();
}


int T6_init (LCD *Self)
{
  Lcd=*Self;
  
  if (pix_init (Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres)!=0) {
    error ("T6963: pix_init(%d, %d, %d, %d) failed", Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres);
    return -1;
  }
  
  if (cfg_number("Icons", 0, 0, 8, &Icons) < 0) return -1;
  if (Icons>0) {
    info ("allocating %d icons", Icons);
    icon_init(Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres, 8, Icons, pix_icon);
    Self->icons=Icons;
    Lcd.icons=Icons;
  }

  Buffer1=malloc(Lcd.cols*Lcd.rows*Lcd.yres);
  if (Buffer1==NULL) {
    error ("T6963: malloc(%d) failed: %s", Lcd.cols*Lcd.rows*Lcd.yres, strerror(errno));
    return -1;
  }

  Buffer2=malloc(Lcd.cols*Lcd.rows*Lcd.yres);
  if (Buffer2==NULL) {
    error ("T6963: malloc(%d) failed: %s", Lcd.cols*Lcd.rows*Lcd.yres, strerror(errno));
    return -1;
  }

  if ((SIGNAL_CE=parport_wire_ctrl ("CE", "STROBE"))==0xff) return -1;
  if ((SIGNAL_CD=parport_wire_ctrl ("CD", "SELECT"))==0xff) return -1;
  if ((SIGNAL_RD=parport_wire_ctrl ("RD", "AUTOFD"))==0xff) return -1;
  if ((SIGNAL_WR=parport_wire_ctrl ("WR", "INIT")  )==0xff) return -1;

  if (parport_open() != 0) {
    error ("T6963: could not initialize parallel port!");
    return -1;
  }

  // rise CE, CD, RD and WR
  parport_control (SIGNAL_CE | SIGNAL_CD | SIGNAL_RD | SIGNAL_WR,
		   SIGNAL_CE | SIGNAL_CD | SIGNAL_RD | SIGNAL_WR);
  
  // set direction: write
  parport_direction (0);

  debug ("setting %d columns", Lcd.cols);

  T6_send_word (0x40, 0x0000);    // Set Text Home Address
  T6_send_word (0x41, Lcd.cols);  // Set Text Area

  T6_send_word (0x42, 0x0200);    // Set Graphic Home Address
  T6_send_word (0x43, Lcd.cols);  // Set Graphic Area

  T6_write_cmd (0x80);            // Mode Set: OR mode, Internal CG RAM mode
  T6_send_word (0x22, 0x0002);    // Set Offset Register
  T6_write_cmd (0x98);            // Set Display Mode: Curser off, Text off, Graphics on
  T6_write_cmd (0xa0);            // Set Cursor Pattern: 1 line cursor
  T6_send_word (0x21, 0x0000);    // Set Cursor Pointer to (0,0)

  T6_clear(1);
  
  return 0;
}


int T6_put (int row, int col, char *text)
{
  return pix_put(row,col,text);
}


int T6_bar (int type, int row, int col, int max, int len1, int len2)
{
  return pix_bar(type,row,col,max,len1,len2);
}


int T6_icon (int num, int seq, int row, int col)
{
  return icon_draw (num, seq, row, col);
}


int T6_flush (void)
{
  int i, j, e;

  memset(Buffer1,0,Lcd.cols*Lcd.rows*Lcd.yres*sizeof(*Buffer1));
  
  for (i=0; i<Lcd.cols*Lcd.rows*Lcd.yres; i++) {
    for (j=0; j<Lcd.xres; j++) {
      Buffer1[i]<<=1;
      if (LCDpixmap[i*Lcd.xres+j]) Buffer1[i]|=1;
    }
  }
  
  for (i=0; i<Lcd.cols*Lcd.rows*Lcd.yres; i++) {
    if (Buffer1[i]==Buffer2[i]) continue;
    for (j=i, e=0; i<Lcd.cols*Lcd.rows*Lcd.yres; i++) {
      if (Buffer1[i]==Buffer2[i]) {
	if (++e>4) break;
      } else {
	e=0;
      }
    }
    T6_memcpy (j, Buffer1+j, i-j-e+1);
  }

  memcpy(Buffer2,Buffer1,Lcd.cols*Lcd.rows*Lcd.yres*sizeof(*Buffer1));
  
  return 0;
}


int T6_quit (void)
{
  return parport_close();
}


LCD T6963[] = {
  { name: "TLC1091", 
    rows:  16,
    cols:  40,
    xres:  6,
    yres:  8,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 | BAR_T,
    icons: 0,
    gpos:  0,
    init:  T6_init,
    clear: T6_clear,
    put:   T6_put,
    bar:   T6_bar,
    icon:  T6_icon,
    gpo:   NULL,
    flush: T6_flush,
    quit:  T6_quit 
  },
  { name: "DMF5002N",
    rows:  14,
    cols:  16,
    xres:  8,
    yres:  8,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 | BAR_T,
    icons: 0,
    gpos:  0,
    init:  T6_init,
    clear: T6_clear,
    put:   T6_put,
    bar:   T6_bar,
    icon:  T6_icon,
    gpo:   NULL,
    flush: T6_flush,
    quit:  T6_quit
  },
  { NULL }
};
