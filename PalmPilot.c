/* $Id: PalmPilot.c,v 1.15 2004/01/09 04:16:06 reinelt Exp $
 *
 * driver for 3Com Palm Pilot
 *
 * Copyright 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: PalmPilot.c,v $
 * Revision 1.15  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.14  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.13  2003/09/13 06:45:43  reinelt
 * icons for all remaining drivers
 *
 * Revision 1.12  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.11  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.10  2003/08/17 12:11:58  reinelt
 * framework for icons prepared
 *
 * Revision 1.9  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.8  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.7  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.6  2001/04/27 05:04:57  reinelt
 *
 * replaced OPEN_MAX with sysconf()
 * replaced mktemp() with mkstemp()
 * unlock serial port if open() fails
 *
 * Revision 1.5  2001/03/16 16:40:17  ltoetsch
 * implemented time bar
 *
 * Revision 1.4  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.3  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.2  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.1  2000/05/02 06:05:00  reinelt
 *
 * driver for 3Com Palm Pilot added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD PalmPilot[]
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "cfg.h"
#include "lock.h"
#include "display.h"
#include "bar.h"
#include "icon.h"
#include "pixmap.h"


static LCD Lcd;
static char *Port=NULL;
static speed_t Speed;
static int Device=-1;

static int pixel=-1;
static int pgap=0;
static int rgap=0;
static int cgap=0;
static int border=0;
static int icons;

static int Palm_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("PalmPilot: port %s could not be locked", Port);
    else
      error ("PalmPilot: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("PalmPilot: open(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("PalmPilot: tcgetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("PalmPilot: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  return fd;
}

static void Palm_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("PalmPilot: write(%s) failed: %s", Port, strerror(errno));
  }
}

int Palm_flush (void)
{
  static unsigned char *bitbuf=NULL;
  static unsigned char *rowbuf=NULL;
  int xsize, ysize, row, col;

  xsize=2*border+(Lcd.cols-1)*cgap+Lcd.cols*Lcd.xres*pixel+(Lcd.cols*Lcd.xres-1)*pgap;
  ysize=2*border+(Lcd.rows-1)*rgap+Lcd.rows*Lcd.yres*pixel+(Lcd.rows*Lcd.yres-1)*pgap;
  
  if (bitbuf==NULL) {
    if ((bitbuf=malloc(xsize*ysize*sizeof(*bitbuf)))==NULL) {
      error ("PalmPilot: malloc(%d) failed: %s", xsize*ysize*sizeof(*bitbuf), strerror(errno));
      return -1;
    }
  }
  
  if (rowbuf==NULL) {
    if ((rowbuf=malloc(((xsize+7)/8)*sizeof(*rowbuf)))==NULL) {
      error ("PalmPilot: malloc(%d) failed: %s", ((xsize+7)/8)*sizeof(*rowbuf), strerror(errno));
      return -1;
    }
  }
  
  memset (bitbuf, 0, xsize*ysize*sizeof(*bitbuf));
  
  for (row=0; row<Lcd.rows*Lcd.yres; row++) {
    int y=border+(row/Lcd.yres)*rgap+row*(pixel+pgap);
    for (col=0; col<Lcd.cols*Lcd.xres; col++) {
      int x=border+(col/Lcd.xres)*cgap+col*(pixel+pgap);
      int a, b;
      for (a=0; a<pixel; a++)
	for (b=0; b<pixel; b++)
	  bitbuf[y*xsize+x+a*xsize+b]=LCDpixmap[row*Lcd.cols*Lcd.xres+col];
    }
  }
  
  sprintf (rowbuf, "%c%c", xsize, ysize);
  Palm_write (rowbuf, 2);

  for (row=0; row<ysize; row++) {
    memset (rowbuf, 0, (xsize+7)/8);
    for (col=0; col<xsize; col++) {
      rowbuf[col/8]<<=1;
      rowbuf[col/8]|=bitbuf[row*xsize+col];
    }
    rowbuf[xsize/8]<<=(8-xsize%8)&7;
    Palm_write (rowbuf, (xsize+7)/8);
  }
  
  return 0;
}

int Palm_clear (int full)
{
  if (pix_clear()!=0) 
    return -1;
  
  if (full)
    return Palm_flush();

  return 0;
}

int Palm_init (LCD *Self)
{
  char *port, *s;
  int speed;
  int rows=-1, cols=-1;
  int xres=1, yres=-1;

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get (NULL, "Port", NULL);
  if (port==NULL || *port=='\0') {
    error ("PalmPilot: no 'Port' entry in %s", cfg_source());
    return -1;
  }
  Port=strdup(port);

  if (cfg_number(NULL, "Speed", 19200, 1200,19200, &speed)<0) return -1;
  
  switch (speed) {
  case 1200:
    Speed=B1200;
    break;
  case 2400:
    Speed=B2400;
    break;
  case 4800:
    Speed=B4800;
    break;
  case 9600:
    Speed=B9600;
    break;
  case 19200:
    Speed=B19200;
    break;
  default:
    error ("PalmPilot: unsupported speed '%d' in %s", speed, cfg_source());
    return -1;
  }    

  debug ("using port %s at %d baud", Port, speed);

  if (sscanf(s=cfg_get(NULL, "size", "20x4"), "%dx%d", &cols, &rows)!=2 || rows<1 || cols<1) {
    error ("PalmPilot: bad size '%s'", s);
    return -1;
  }

  if (sscanf(s=cfg_get(NULL, "font", "6x8"), "%dx%d", &xres, &yres)!=2 || xres<5 || yres<7) {
    error ("PalmPilot: bad font '%s'", s);
    return -1;
  }

  if (sscanf(s=cfg_get(NULL, "pixel", "1+0"), "%d+%d", &pixel, &pgap)!=2 || pixel<1 || pgap<0) {
    error ("PalmPilot: bad pixel '%s'", s);
    return -1;
  }

  if (sscanf(s=cfg_get(NULL, "gap", "0x0"), "%dx%d", &cgap, &rgap)!=2 || cgap<-1 || rgap<-1) {
    error ("PalmPilot: bad gap '%s'", s);
    return -1;
  }
  if (rgap<0) rgap=pixel+pgap;
  if (cgap<0) cgap=pixel+pgap;

  if (cfg_number(NULL, "border", 0, 0, 1000000, &border)<0) return -1;

  if (pix_init (rows, cols, xres, yres)!=0) {
    error ("PalmPilot: pix_init(%d, %d, %d, %d) failed", rows, cols, xres, yres);
    return -1;
  }

  if (cfg_number(NULL, "Icons", 0, 0, 8, &icons) < 0) return -1;
  if (icons>0) {
    info ("allocating %d icons", icons);
    icon_init(rows, cols, xres, yres, 8, icons, pix_icon);
  }


  Self->rows=rows;
  Self->cols=cols;
  Self->xres=xres;
  Self->yres=yres;
  Self->icons=icons;
  Lcd=*Self;

  // Device=open ("PalmOrb.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  Device=Palm_open();
  if (Device==-1) return -1;

  pix_clear();

  return 0;
}

int Palm_put (int row, int col, char *text)
{
  return pix_put (row, col, text);
}

int Palm_bar (int type, int row, int col, int max, int len1, int len2)
{
  return pix_bar (type, row, col, max, len1, len2);
}

int Palm_icon (int num, int seq, int row, int col)
{
  return icon_draw (num, seq, row, col);
}

int Palm_quit (void)
{
  debug ("closing port %s", Port);
  close (Device);
  unlock_port (Port);
  return 0;
}

LCD PalmPilot[] = {
  { name: "PalmPilot",
    rows:  0,
    cols:  0,
    xres:  0,
    yres:  0,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 | BAR_T,
    icons: 0,
    gpos:  0,
    init:  Palm_init,
    clear: Palm_clear,
    put:   Palm_put,
    bar:   Palm_bar,
    icon:  Palm_icon,
    gpo:   NULL,
    flush: Palm_flush,
    quit:  Palm_quit },
  { NULL }
};
