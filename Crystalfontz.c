/* $Id: Crystalfontz.c,v 1.13 2003/08/24 05:17:58 reinelt Exp $
 *
 * driver for display modules from Crystalfontz
 *
 * Copyright 2000 by Herbert Rosmanith (herp@wildsau.idv.uni-linz.ac.at)
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
 * $Log: Crystalfontz.c,v $
 * Revision 1.13  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.12  2003/08/19 04:28:41  reinelt
 * more Icon stuff, minor glitches fixed
 *
 * Revision 1.11  2003/08/17 06:57:04  reinelt
 * complete rewrite of the Crystalfontz driver
 *
 * Revision 1.10  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.9  2003/02/22 07:53:09  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.8  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.7  2001/04/27 05:04:57  reinelt
 *
 * replaced OPEN_MAX with sysconf()
 * replaced mktemp() with mkstemp()
 * unlock serial port if open() fails
 *
 * Revision 1.6  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.5  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.4  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.3  2000/06/04 21:43:50  herp
 * minor bugfix (zero length)
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
static int GPO;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;


static int CF_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("Crystalfontz: port %s could not be locked", Port);
    else
      error ("Crystalfontz: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("Crystalfontz: open(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("Crystalfontz: tcgetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("Crystalfontz: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  return fd;
}


static void CF_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("Crystalfontz: write(%s) failed: %s", Port, strerror(errno));
  }
}


static int CF_backlight (void)
{
  char buffer[3];
  int  backlight;

  backlight=atoi(cfg_get("Backlight","0"));
  snprintf (buffer, 3, "\016%c", backlight);
  CF_write (buffer, 2);
  return 0;
}


static int CF_contrast (void)
{
  char buffer[3];
  int  contrast;

  contrast=atoi(cfg_get("Contrast","50"));
  snprintf (buffer, 3, "\017%c", contrast);
  CF_write (buffer, 2);
  return 0;
}


static void CF_define_char (int ascii, char *buffer)
{
  char cmd[3]="031"; // set custom char bitmap

  cmd[1]=128+(char)ascii;
  CF_write (cmd, 2);
  CF_write (buffer, 8);
}


static int CF_clear (int full)
{
  memset (FrameBuffer1, ' ', Lcd.rows*Lcd.cols*sizeof(char));
  bar_clear();
  GPO=0;
  
  if (full) {
    memset (FrameBuffer2, ' ', Lcd.rows*Lcd.cols*sizeof(char));
    CF_write ("\014", 1);  // Form Feed (Clear Display)
  }
  
  return 0;
}


static int CF_init (LCD *Self)
{
  char *port;
  char *speed;

  Lcd=*Self;

  // Init the framebuffers
  FrameBuffer1 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  FrameBuffer2 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("Crystalfontz: framebuffer could not be allocated: malloc() failed");
    return -1;
  }

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port",NULL);
  if (port==NULL || *port=='\0') {
    error ("Crystalfontz: no 'Port' entry in %s", cfg_source());
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
    error ("Crystalfontz: unsupported speed '%s' in %s", speed, cfg_source());
    return -1;
  }    

  debug ("using port %s at %d baud", Port, atoi(speed));

  Device=CF_open();
  if (Device==-1) return -1;

  bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block

  // MR: why such a large delay?
  usleep(350000);

  CF_clear(1);

  CF_write ("\004", 1);  // hide cursor
  CF_write ("\024", 1);  // scroll off
  CF_write ("\030", 1);  // wrap off

  CF_backlight();
  CF_contrast();

  return 0;
}


void CF_goto (int row, int col)
{
  char cmd[3]="\021"; // set cursor position

  if (row==0 && col==0) {
    CF_write ("\001", 1); // cursor home
  } else {
    cmd[1]=(char)col;
    cmd[2]=(char)row;
    CF_write(cmd,3);
  }
}


int CF_put (int row, int col, char *text)
{
  char *p=FrameBuffer1+row*Lcd.cols+col;
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int CF_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


static int CF_flush (void)
{
  int row, col, pos1, pos2;
  int c, equal;
  
  bar_process(CF_define_char);
  
  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	if (c!=32) c+=128; //blank
	FrameBuffer1[row*Lcd.cols+col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) continue;
      CF_goto (row, col);
      for (pos1=col++, pos2=pos1, equal=0; col<Lcd.cols; col++) {
	if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes one byte, too.
	  if (++equal>4) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      CF_write (FrameBuffer1+row*Lcd.cols+pos1, pos2-pos1+1);
    }
  }
  
  memcpy (FrameBuffer2, FrameBuffer1, Lcd.rows*Lcd.cols*sizeof(char));

  return 0;
}


int CF_quit (void)
{
  info("Crystalfontz: shutting down.");

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


LCD Crystalfontz[] = {
  { name: "626",
    rows:  2, 
    cols:  16,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  CF_init,
    clear: CF_clear,
    put:   CF_put,
    bar:   CF_bar,
    gpo:   NULL,
    flush: CF_flush,
    quit:  CF_quit 
  },
  { name: "636",
    rows:  2, 
    cols:  16,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  CF_init,
    clear: CF_clear,
    put:   CF_put,
    bar:   CF_bar,
    gpo:   NULL,
    flush: CF_flush,
    quit:  CF_quit 
  },
  { name: "632",
    rows:  2, 
    cols:  16,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  CF_init,
    clear: CF_clear,
    put:   CF_put,
    bar:   CF_bar,
    gpo:   NULL,
    flush: CF_flush,
    quit:  CF_quit 
  },
  { name: "634",
    rows:  4, 
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  CF_init,
    clear: CF_clear,
    put:   CF_put,
    bar:   CF_bar,
    gpo:   NULL,
    flush: CF_flush,
    quit:  CF_quit 
  },
  { NULL }
};
