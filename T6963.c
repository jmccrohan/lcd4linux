/* $Id: T6963.c,v 1.3 2002/08/19 04:41:20 reinelt Exp $
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
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>

#if defined (HAVE_LINUX_PARPORT_H) && defined (HAVE_LINUX_PPDEV_H)
#define WITH_PPDEV
#include <linux/parport.h>
#include <linux/ppdev.h>
#else
#error The T6963 driver needs ppdev
#error cannot compile T6963 driver
#endif

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "udelay.h"
#include "pixmap.h"


/*
 * /CE (Chip Enable)  <==>  /Strobe
 * C/D (Command/Data) <==>  /Select
 * /RD (Read)         <==>  /AutoFeed
 * /WR (Write)        <==>  Init
 *
 */

#define CE_L PARPORT_CONTROL_STROBE
#define CE_H 0

#define CD_L PARPORT_CONTROL_SELECT
#define CD_H 0

#define RD_L PARPORT_CONTROL_AUTOFD
#define RD_H 0

#define WR_L 0
#define WR_H PARPORT_CONTROL_INIT


#define XRES 6
#define YRES 8
#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 | BAR_T)

static LCD Lcd;

static char *PPdev=NULL;
static int PPfd=-1;

unsigned char *Buffer1, *Buffer2;

// perform normal status check
void T6_status1 (void)
{
  int direction;
  int n;
  unsigned char ctrl, data;

  // turn off data line drivers
  direction=1;
  ioctl (PPfd, PPDATADIR, &direction);
  
  // lower RD and CE
  ctrl = RD_L | WR_H | CE_L | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(150); // Access Time: 150 ns 

  // wait for STA0=1 and STA1=1
  n=0;
  do {
    rep_nop();
    if (++n>1000) {
      debug("hang in status1");
      break;
    }
    ioctl (PPfd, PPRDATA, &data);
  } while ((data & 0x03) != 0x03);
  
  // rise RD and CE
  ctrl = RD_H | WR_H | CE_H | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  // turn on data line drivers
  direction=0;
  ioctl (PPfd, PPDATADIR, &direction);

}
// Fixme:
static int bug=0;

// perform status check in "auto mode"
void T6_status2 (void)
{
  int direction;
  int n;
  unsigned char ctrl, data;

  // turn off data line drivers
  direction=1;
  ioctl (PPfd, PPDATADIR, &direction);

  // lower RD and CE
  ctrl = RD_L | WR_H | CE_L | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(150); // Access Time: 150 ns 

  // wait for STA3=1
  n=0;
  do {
    rep_nop();
    if (++n>1000) {
      debug("hang in status2");
      bug=1;
      break;
    }
    ioctl (PPfd, PPRDATA, &data);
    // } while ((data & 0x08) == 0);
  } while ((data & 0x08) != 0x08);

  // rise RD and CE
  ctrl = RD_H | WR_H | CE_H | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);

  // turn on data line drivers
  direction=0;
  ioctl (PPfd, PPDATADIR, &direction);
}


static void T6_write_cmd (unsigned char cmd)
{
  unsigned char ctrl;

  // wait until the T6963 is idle
  T6_status1();

  // put data on DB1..DB8
  ioctl(PPfd, PPWDATA, &cmd);

  // lower WR and CE
  ctrl = RD_H | WR_L | CE_L | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(80); // Pulse width

  // rise WR and CE
  ctrl = RD_H | WR_H | CE_H | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);

  ndelay(40); // Data Hold Time
}

static void T6_write_data (unsigned char data)
{
  unsigned char ctrl;
    
  // wait until the T6963 is idle
  T6_status1();

  // put data on DB1..DB8
  ioctl(PPfd, PPWDATA, &data);

  // lower C/D
  ctrl = RD_H | WR_H | CE_H | CD_L;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(20); // C/D Setup Time

  // lower WR and CE
  ctrl = RD_H | WR_L | CE_L | CD_L;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(80); // Pulse Width
  
  // rise WR and CE
  ctrl = RD_H | WR_H | CE_H | CD_L;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(40); // Data Hold Time

  // rise CD
  ctrl = RD_H | WR_H | CE_H | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);
}

static void T6_write_auto (unsigned char data)
{
  unsigned char ctrl;
    
  // wait until the T6963 is idle
  T6_status2();

  // put data on DB1..DB8
  ioctl(PPfd, PPWDATA, &data);

  // lower C/D
  ctrl = RD_H | WR_H | CE_H | CD_L;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(20); // C/D Setup Time

  // lower WR and CE
  ctrl = RD_H | WR_L | CE_L | CD_L;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(80); // Pulse Width
  
  // rise WR and CE
  ctrl = RD_H | WR_H | CE_H | CD_L;
  ioctl (PPfd, PPWCONTROL, &ctrl);
  
  ndelay(40); // Data Hold Time

  // rise CD
  ctrl = RD_H | WR_H | CE_H | CD_H;
  ioctl (PPfd, PPWCONTROL, &ctrl);
}

static void T6_send_byte (unsigned char cmd, unsigned char data)
{
  T6_write_data(data);
  T6_write_cmd(cmd);
}

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


static int T6_open (void)
{
  debug ("using ppdev %s", PPdev);
  PPfd=open(PPdev, O_RDWR);
  if (PPfd==-1) {
    error ("open(%s) failed: %s", PPdev, strerror(errno));
    return -1;
  }

#if 0
  if (ioctl(PPfd, PPEXCL)) {
    debug ("ioctl(%s, PPEXCL) failed: %s", PPdev, strerror(errno));
  } else {
    debug ("ioctl(%s, PPEXCL) succeded.");
  }
#endif

  if (ioctl(PPfd, PPCLAIM)) {
    error ("ioctl(%s, PPCLAIM) failed: %d %s", PPdev, errno, strerror(errno));
    return -1;
  }

  return 0;
}

int T6_clear (void)
{
  int rows;
  
  rows=(Lcd.rows>8 ? 8 : Lcd.rows);

  T6_memset(0x0000, 0, Lcd.cols*rows);    // clear text area 
  T6_memset(0x0200, 0, Lcd.cols*rows*8);  // clear graphic area

  if (Lcd.rows>8) {
    T6_memset(0x8000, 0, Lcd.cols*(Lcd.rows-rows));    // clear text area #2
    T6_memset(0x8200, 0, Lcd.cols*(Lcd.rows-rows)*8);  // clear graphic area #2
  }

  memset(Buffer1,0,Lcd.cols*Lcd.rows*Lcd.yres*sizeof(*Buffer1));
  memset(Buffer2,0,Lcd.cols*Lcd.rows*Lcd.yres*sizeof(*Buffer2));

  return pix_clear();
}

int T6_init (LCD *Self)
{
  char *port;

  Lcd=*Self;

  if (PPdev) {
    free (PPdev);
    PPdev=NULL;
  }

  port=cfg_get ("Port");
  if (port==NULL || *port=='\0') {
    error ("T6963: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  PPdev=strdup(port);
  
  if (pix_init (Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres)!=0) {
    error ("T6963: pix_init(%d, %d, %d, %d) failed", Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres);
    return -1;
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

  udelay_init();

  if (T6_open()!=0)
    return -1;
  
  debug ("setting %d columns", Lcd.cols);

  T6_send_word (0x40, 0x0000);    // Set Text Home Address
  T6_send_word (0x41, 40);        // Set Text Area

  T6_send_word (0x42, 0x0200);    // Set Graphic Home Address
  T6_send_word (0x43, 40);        // Set Graphic Area

  T6_write_cmd (0x80);            // Mode Set: OR mode, Internal CG RAM mode
  T6_send_word (0x22, 0x0002);    // Set Offset Register
  T6_write_cmd (0x98);            // Set Display Mode: Curser off, Text off, Graphics on
  T6_write_cmd (0xa0);            // Set Cursor Pattern: 1 line cursor
  T6_send_word (0x21, 0x0000);    // Set Cursor Pointer to (0,0)

  T6_clear();
  
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
  debug ("closing ppdev %s", PPdev);
  if (ioctl(PPfd, PPRELEASE)) {
    error ("ioctl(%s, PPRELEASE) failed: %s", PPdev, strerror(errno));
  }
  if (close(PPfd)==-1) {
    error ("close(%s) failed: %s", PPdev, strerror(errno));
    return -1;
  }
  return 0;
}

LCD T6963[] = {
  { "TLC1091", 16,40,XRES,YRES,BARS,0,T6_init,T6_clear,T6_put,T6_bar,NULL,T6_flush,T6_quit },
  { "DMF5002N",14,40,XRES,YRES,BARS,0,T6_init,T6_clear,T6_put,T6_bar,NULL,T6_flush,T6_quit },
  { NULL }
};
