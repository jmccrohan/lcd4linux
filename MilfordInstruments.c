/* $Id: MilfordInstruments.c,v 1.5 2004/01/09 04:16:06 reinelt Exp $
 *
 * driver for Milford Instruments 'BPK' piggy-back serial interface board
 * for standard Hitachi 44780 compatible lcd modules.
 *
 * Copyright 2003 Andy Baxter <andy@earthsong.free-online.co.uk>
 *
 * based on the MatrixOrbital driver which is
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: MilfordInstruments.c,v $
 * Revision 1.5  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.4  2004/01/06 22:33:13  reinelt
 * Copyright statements cleaned up
 *
 * Revision 1.3  2003/10/08 13:39:53  andy-b
 * Cleaned up code in MilfordInstruments.c, and added descriptions for other display sizes (untested)
 *
 * Revision 1.1  2003/09/29 06:58:36  reinelt
 * new driver for Milford Instruments MI420 by Andy Baxter
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD MilfordInstruments[]
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
#include "icon.h"

#define XRES 5
#define YRES 8
#define CHARS 8

static LCD Lcd;
static char *Port=NULL;
static speed_t Speed;
static int Device=-1;
static int Icons;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;


static int MI_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("MilfordInstruments: port %s could not be locked", Port);
    else
      error ("MilfordInstruments: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("MilfordInstruments: open(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("MilfordInstruments: tcgetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("MilfordInstruments: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  return fd;
}


static void MI_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("MilfordInstruments: write(%s) failed: %s", Port, strerror(errno));
  }
}


static void MI_define_char (int ascii, char *buffer)
{
  char cmd[2]="\376x";
  if (ascii<8) {
    cmd[1]=(char)(64+ascii*8);
    MI_write (cmd, 2);
    MI_write (buffer, 8);
  }
}


static int MI_clear (int full)
{
  memset (FrameBuffer1, ' ', Lcd.rows*Lcd.cols*sizeof(char));

  icon_clear();
  bar_clear();
  
  if (full) {
    memset (FrameBuffer2, ' ', Lcd.rows*Lcd.cols*sizeof(char));
    MI_write ("\376\001", 2);  // clear screen
    }
  return 0;
}


static int MI_init (LCD *Self)
{
  char *port;
  int speed;

  Lcd=*Self;

  // Init the framebuffers
  FrameBuffer1 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  FrameBuffer2 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("MilfordInstruments: framebuffer could not be allocated: malloc() failed");
    return -1;
  }

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get (NULL, "Port", NULL);
  if (port==NULL || *port=='\0') {
    error ("MilfordInstruments: no 'Port' entry in %s", cfg_source());
    return -1;
  }
  Port=strdup(port);

  if (cfg_number(NULL, "Speed", 19200, 1200,19200, &speed)<0) return -1;
  switch (speed) {
  case 2400:
    Speed=B2400;
    break;
  case 9600:
    Speed=B9600;
    break;
  default:
    error ("MilfordInstruments: unsupported speed '%d' in %s", speed, cfg_source());
    return -1;
  }    
  
  debug ("using port %s at %d baud", Port, speed);
  
  Device=MI_open();
  if (Device==-1) return -1;

  if (cfg_number(NULL, "Icons", 0, 0, CHARS, &Icons)<0) return -1;
  if (Icons>0) {
    debug ("reserving %d of %d user-defined characters for icons", Icons, CHARS);
    icon_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS, Icons, MI_define_char);
    Self->icons=Icons;
    Lcd.icons=Icons;
  }
  
  bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS-Icons);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block

  MI_clear(1);

  MI_write ("\376\014", 2);  // cursor off

  return 0;
}


void MI_goto (int row, int col)
{
  char cmd[2]="\376x";
  char ddbase=128;
  if (row & 1) { // i.e. if row is 1 or 3
    ddbase += 64;
    };
  if (row & 2) { // i.e. if row is 0 or 2.
    ddbase += 20;
    };
  cmd[1]=(char)(ddbase+col);
  MI_write(cmd,2);
}


int MI_put (int row, int col, char *text)
{
  char *p=FrameBuffer1+row*Lcd.cols+col;
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int MI_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int MI_icon (int num, int seq, int row, int col)
{
  return icon_draw (num, seq, row, col);
}


static int MI_flush (void)
{
  int row, col, pos1, pos2;
  int c, equal;
  
  bar_process(MI_define_char);
  
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
      MI_goto (row, col);
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
      MI_write (FrameBuffer1+row*Lcd.cols+pos1, pos2-pos1+1);
    }
  }
  
  memcpy (FrameBuffer2, FrameBuffer1, Lcd.rows*Lcd.cols*sizeof(char));

  return 0;
}


int MI_quit (void)
{
  info("MilfordInstruments: shutting down.");

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


LCD MilfordInstruments[] = {
  { name: "MI216",
    rows:  2, 
    cols:  16,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  MI_init,
    clear: MI_clear,
    put:   MI_put,
    bar:   MI_bar,
    icon:  MI_icon,
    gpo:   NULL,
    flush: MI_flush,
    quit:  MI_quit 
  },
  { name: "MI220",
    rows:  2, 
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  MI_init,
    clear: MI_clear,
    put:   MI_put,
    bar:   MI_bar,
    icon:  MI_icon,
    gpo:   NULL,
    flush: MI_flush,
    quit:  MI_quit 
  },
  { name: "MI240",
    rows:  2, 
    cols:  40,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  MI_init,
    clear: MI_clear,
    put:   MI_put,
    bar:   MI_bar,
    icon:  MI_icon,
    gpo:   NULL,
    flush: MI_flush,
    quit:  MI_quit 
  },
  { name: "MI420",
    rows:  4, 
    cols:  20,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  MI_init,
    clear: MI_clear,
    put:   MI_put,
    bar:   MI_bar,
    icon:  MI_icon,
    gpo:   NULL,
    flush: MI_flush,
    quit:  MI_quit 
  },

  { NULL }
};
