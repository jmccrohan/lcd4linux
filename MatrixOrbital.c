/* $Id: MatrixOrbital.c,v 1.27 2003/02/22 07:53:10 reinelt Exp $
 *
 * driver for Matrix Orbital serial display modules
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
 * $Log: MatrixOrbital.c,v $
 * Revision 1.27  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.26  2003/02/13 10:40:17  reinelt
 *
 * changed "copyright" to "2003"
 * added slightly different protocol for MatrixOrbital "LK202" displays
 *
 * Revision 1.25  2002/08/19 09:30:18  reinelt
 * MatrixOrbital uses generic bar funnctions
 *
 * Revision 1.24  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.23  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.22  2001/04/27 05:04:57  reinelt
 *
 * replaced OPEN_MAX with sysconf()
 * replaced mktemp() with mkstemp()
 * unlock serial port if open() fails
 *
 * Revision 1.21  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.20  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.19  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.18  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.17  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 * Revision 1.16  2000/04/13 06:09:52  reinelt
 *
 * added BogoMips() to system.c (not used by now, maybe sometimes we can
 * calibrate our delay loop with this value)
 *
 * added delay loop to HD44780 driver. It seems to be quite fast now. Hopefully
 * no compiler will optimize away the delay loop!
 *
 * Revision 1.15  2000/04/12 08:05:45  reinelt
 *
 * first version of the HD44780 driver
 *
 * Revision 1.14  2000/04/10 04:40:53  reinelt
 *
 * minor changes and cleanups
 *
 * Revision 1.13  2000/04/07 05:42:20  reinelt
 *
 * UUCP style lockfiles for the serial port
 *
 * Revision 1.12  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.11  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.10  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.9  2000/03/22 07:33:50  reinelt
 *
 * FAQ added
 * new modules 'processor.c' contains all data processing
 *
 * Revision 1.8  2000/03/19 08:41:28  reinelt
 *
 * documentation available! README, README.MatrixOrbital, README.Drivers
 * added Skeleton.c as a starting point for new drivers
 *
 * Revision 1.7  2000/03/18 08:07:04  reinelt
 *
 * vertical bars implemented
 * bar compaction improved
 * memory information implemented
 *
 * Revision 1.6  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.5  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.4  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD MatrixOrbital[]
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
#include "lock.h"
#include "display.h"
#include "bar.h"

#define XRES 5
#define YRES 8
#define CHARS 8

static LCD Lcd;
static char *Port=NULL;
static speed_t Speed;
static int Device=-1;

static char Txt[4][40];
static int  GPO;


static int MO_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("MatrixOrbital: port %s could not be locked", Port);
    else
      error ("MatrixOrbital: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("MatrixOrbital: open(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("MatrixOrbital: tcgetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("MatrixOrbital: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  return fd;
}


static void MO_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("MatrixOrbital: write(%s) failed: %s", Port, strerror(errno));
  }
}


static int MO_contrast (void)
{
  char buffer[4];
  int  contrast;

  contrast=atoi(cfg_get("Contrast","160"));
  snprintf (buffer, 4, "\376P%c", contrast);
  MO_write (buffer, 3);
  return 0;
}


static void MO_define_char (int ascii, char *buffer)
{
  char cmd[3]="\376N";

  cmd[2]=(char)ascii;
  MO_write (cmd, 3);
  MO_write (buffer, 8);
}


static int MO_clear (int protocol)
{
  int row, col, gpo;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
    }
  }

  bar_clear();

  switch (protocol) {
  case 1:
    MO_write ("\014",  1);  // Clear Screen
    MO_write ("\376V", 2);  // GPO off
    break;
  case 2:
    MO_write ("\376\130",  2);  // Clear Screen
    for (gpo=1; gpo<=Lcd.gpos; gpo++) {
      char cmd[3]="\376V";
      cmd[2]=(char)gpo;
      MO_write (cmd, 3);  // GPO off
    }
    break;
  }
  
  GPO=0;
  return 0;
}

int MO_clear1 (void)
{
  return MO_clear(1);
}

int MO_clear2 (void)
{
  return MO_clear(2);
}


static int MO_init (LCD *Self, int protocol)
{
  char *port;
  char *speed;

  Lcd=*Self;

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port",NULL);
  if (port==NULL || *port=='\0') {
    error ("MatrixOrbital: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  Port=strdup(port);

  speed=cfg_get("Speed","19200");
  
  switch (atoi(speed)) {
  case 1200:
    Speed=B1200;
    break;
  case 2400:
    Speed=B2400;
    break;
  case 9600:
    Speed=B9600;
    break;
  case 19200:
    Speed=B19200;
    break;
  default:
    error ("MatrixOrbital: unsupported speed '%s' in %s", speed, cfg_file());
    return -1;
  }    

  debug ("using port %s at %d baud", Port, atoi(speed));

  Device=MO_open();
  if (Device==-1) return -1;

  bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block

  MO_clear(protocol);
  MO_contrast();

  MO_write ("\376B", 3);  // backlight on
  MO_write ("\376K", 2);  // cursor off
  MO_write ("\376T", 2);  // blink off
  MO_write ("\376D", 2);  // line wrapping off
  MO_write ("\376R", 2);  // auto scroll off

  return 0;
}

int MO_init1 (LCD *Self)
{
  return MO_init(Self, 1);
}

int MO_init2 (LCD *Self)
{
  return MO_init(Self, 2);
}



int MO_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int MO_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int MO_gpo (int num, int val)
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


static int MO_flush (int protocol)
{
  char buffer[256]="\376G";
  char *p;
  int c, row, col, gpo;
  
  bar_process(MO_define_char);
  
  for (row=0; row<Lcd.rows; row++) {
    buffer[3]=row+1;
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	Txt[row][col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      buffer[2]=col+1;
      for (p=buffer+4; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      MO_write (buffer, p-buffer);
    }
  }

  switch (protocol) {
  case 1:
    if (GPO & 1) {
      MO_write ("\376W", 2);  // GPO on
    } else {
      MO_write ("\376V", 2);  // GPO off
    }
    break;
  case 2:
    for (gpo=1; gpo<=Lcd.gpos; gpo++) {
      char cmd[3]="\376";
      cmd[1]=(GPO&(1<<(gpo-1))) ? 'W':'V';
      cmd[2]=(char)gpo;
      MO_write (cmd, 3);
    }
    break;
  }

  return 0;
}

int MO_flush1 (void)
{
  return MO_flush(1);
}

int MO_flush2 (void)
{
  return MO_flush(2);
}


int MO_quit (void)
{
  debug ("closing port %s", Port);
  close (Device);
  unlock_port(Port);
  return (0);
}


LCD MatrixOrbital[] = {
  { name: "LCD0821",
    rows:  2, 
    cols:  8,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush1,
    quit:  MO_quit 
  },
  { name: "LCD1621",
    rows:  2,
    cols:  16,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush1,
    quit:  MO_quit 
  },
  { name: "LCD2021",
    rows:  2,
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush1,
    quit:  MO_quit 
  },
  { name: "LCD2041",
    rows:  4,
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush1,
    quit:  MO_quit 
  },
  { name: "LCD4021",
    rows:  2,
    cols:  40,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush1,
    quit:  MO_quit 
  },
  { name: "LK202-25",
    rows:  2,
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  6,
    init:  MO_init2,
    clear: MO_clear2,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush2,
    quit:  MO_quit 
  },
  { name: "LK204-25",
    rows:  4,
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  6,
    init:  MO_init2,
    clear: MO_clear2,
    put:   MO_put,
    bar:   MO_bar,
    gpo:   MO_gpo,
    flush: MO_flush2,
    quit:  MO_quit 
  },
  { NULL }
};
