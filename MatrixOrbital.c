/* $Id: MatrixOrbital.c,v 1.34 2003/08/24 05:17:58 reinelt Exp $
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
 * Revision 1.34  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.33  2003/08/24 04:31:56  reinelt
 * icon.c icon.h added
 *
 * Revision 1.32  2003/08/22 03:45:08  reinelt
 * bug in parallel port code fixed, more icons stuff
 *
 * Revision 1.31  2003/08/19 04:28:41  reinelt
 * more Icon stuff, minor glitches fixed
 *
 * Revision 1.30  2003/08/17 16:37:39  reinelt
 * more icon framework
 *
 * Revision 1.29  2003/08/16 07:31:35  reinelt
 * double buffering in all drivers
 *
 * Revision 1.28  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
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
#include "icon.h"
#include "bar.h"

#define XRES 5
#define YRES 8
#define CHARS 8

static LCD Lcd;
static char *Port=NULL;
static speed_t Speed;
static int Device=-1;
static int Icons;
static int GPO;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;


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
  int gpo;
  
  memset (FrameBuffer1, ' ', Lcd.rows*Lcd.cols*sizeof(char));
  icon_clear();
  bar_clear();
  GPO=0;

  if (protocol) {
    memset (FrameBuffer2, ' ', Lcd.rows*Lcd.cols*sizeof(char));
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
  }
  
  return 0;
}

int MO_clear1 (int full)
{
  return MO_clear(full?1:0);
}

int MO_clear2 (int full)
{
  return MO_clear(full?2:0);
}


static int MO_init (LCD *Self, int protocol)
{
  char *port;
  char *s, *e;

  Lcd=*Self;

  // Init the framebuffers
  FrameBuffer1 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  FrameBuffer2 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("MatrixOrbital: framebuffer could not be allocated: malloc() failed");
    return -1;
  }

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port",NULL);
  if (port==NULL || *port=='\0') {
    error ("MatrixOrbital: no 'Port' entry in %s", cfg_source());
    return -1;
  }
  Port=strdup(port);

  s=cfg_get("Speed","19200");
  
  switch (atoi(s)) {
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
    error ("MatrixOrbital: unsupported speed '%s' in %s", s, cfg_source());
    return -1;
  }    

  debug ("using port %s at %d baud", Port, atoi(s));

  Device=MO_open();
  if (Device==-1) return -1;

  s=cfg_get("Icons", "0");
  Icons=strtol(s, &e, 0);
  if (*e!='\0' || Icons<0 || Icons>8) {
    debug ("Icons=%d e=<%s>", Icons, e);
    error ("MatrixOrbital: bad Icons '%s' in %s, must be between 0 and 8", s, cfg_source());
    return -1;
  }    
  if (Icons>0) {
    info ("reserving %d of %d user-defined characters for icons", Icons, CHARS);
    Self->icons=Icons;
    Lcd.icons=Icons;
  }

  icon_init(Lcd.rows, Lcd.cols, XRES, YRES, Icons);

  bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS-Icons);
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


void MO_goto (int row, int col)
{
  char cmd[5]="\376Gyx";
  cmd[2]=(char)col+1;
  cmd[3]=(char)row+1;
  MO_write(cmd,4);
}


int MO_put (int row, int col, char *text)
{
  char *p=FrameBuffer1+row*Lcd.cols+col;
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


int MO_icon (int num, int row, int col, unsigned char *bitmap)
{
  // icons use last ascii codes
  char ascii=CHARS-num;

  MO_define_char (ascii, bitmap);
  MO_goto(row, col);
  MO_write(&ascii, 1);
  FrameBuffer1[row*Lcd.cols+col]=(char)ascii;
  FrameBuffer2[row*Lcd.cols+col]=(char)ascii;

  return 0;
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
  int row, col, pos1, pos2;
  int c, equal;
  int gpo;

  bar_process(MO_define_char);
  
  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	FrameBuffer1[row*Lcd.cols+col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) continue;
      MO_goto (row, col);
      for (pos1=col++, pos2=pos1, equal=0; col<Lcd.cols; col++) {
	if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes one byte, too.
	  if (++equal>5) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      MO_write (FrameBuffer1+row*Lcd.cols+pos1, pos2-pos1+1);
    }
  }
  
  memcpy (FrameBuffer2, FrameBuffer1, Lcd.rows*Lcd.cols*sizeof(char));

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
  info("MatrixOrbital: shutting down.");

  debug ("closing port %s", Port);
  close (Device);
  unlock_port(Port);

  if (FrameBuffer1) {
    free(FrameBuffer1);
    FrameBuffer1=NULL;
  }

  if (FrameBuffer2) {
    free(FrameBuffer2);
    FrameBuffer2=NULL;
  }

  return (0);
}


LCD MatrixOrbital[] = {
  { name: "LCD0821",
    rows:  2, 
    cols:  8,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
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
    icons: 0,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
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
    icons: 0,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
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
    icons: 0,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
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
    icons: 0,
    gpos:  1,
    init:  MO_init1,
    clear: MO_clear1,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
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
    icons: 0,
    gpos:  6,
    init:  MO_init2,
    clear: MO_clear2,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
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
    icons: 0,
    gpos:  6,
    init:  MO_init2,
    clear: MO_clear2,
    put:   MO_put,
    bar:   MO_bar,
    icon:  MO_icon,
    gpo:   MO_gpo,
    flush: MO_flush2,
    quit:  MO_quit 
  },
  { NULL }
};
