/* $Id: Cwlinux.c,v 1.19 2004/01/30 20:57:55 reinelt Exp $
 *
 * driver for Cwlinux serial display modules
 *
 * Copyright 2002 Andrew Ip <aip@cwlinux.com>
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
 * $Log: Cwlinux.c,v $
 * Revision 1.19  2004/01/30 20:57:55  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.18  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.17  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.16  2003/11/30 16:18:36  reinelt
 * Cwlinux: invalidate Framebuffer in case a char got redefined
 *
 * Revision 1.15  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.14  2003/09/13 06:45:43  reinelt
 * icons for all remaining drivers
 *
 * Revision 1.13  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.12  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.11  2003/08/16 07:31:35  reinelt
 * double buffering in all drivers
 *
 * Revision 1.10  2003/08/01 05:15:42  reinelt
 * last cleanups for 0.9.9
 *
 * Revision 1.9  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.8  2003/05/19 05:55:17  reinelt
 * Cwlinux sleep optimization
 *
 * Revision 1.7  2003/05/14 06:17:39  reinelt
 * added support for CW1602
 *
 * Revision 1.6  2003/02/24 04:50:57  reinelt
 * cwlinux fixes
 *
 * Revision 1.5  2003/02/22 07:53:09  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.4  2003/02/22 07:23:24  reinelt
 * Cwlinux fixes
 *
 * Revision 1.3  2002/09/12 05:24:54  reinelt
 * code cleanup, character defining for bars
 *
 * Revision 1.2  2002/09/11 05:32:35  reinelt
 * changed to use new bar functions
 *
 * Revision 1.1  2002/09/11 05:16:32  reinelt
 * added Cwlinux driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD Cwlinux[]
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

#include "debug.h"
#include "cfg.h"
#include "lock.h"
#include "display.h"
#include "bar.h"
#include "icon.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define CHARS 8

static LCD Lcd;
static char *Port = NULL;
static speed_t Speed;
static int Device = -1;
static int Icons;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;


static int CW_open(void)
{
  int fd;
  pid_t pid;
  struct termios portset;

  if ((pid = lock_port(Port)) != 0) {
    if (pid == -1)
      error("Cwlinux: port %s could not be locked", Port);
    else
      error("Cwlinux: port %s is locked by process %d", Port, pid);
    return -1;
  }

  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    error("Cwlinux: open(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  if (tcgetattr(fd, &portset) == -1) {
    error("Cwlinux: tcgetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  cfmakeraw(&portset);
  cfsetispeed(&portset, Speed);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset) == -1) {
    error("Cwlinux: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }

  return fd;
}


static void CW_write(char *string, int len)
{
  int ret;
  
  ret=write (Device, string, len);
  if (ret<0 && errno==EAGAIN) {
    usleep(1000);
    ret=write (Device, string, len);
  }
  if (ret<0) {
    error ("Cwlinux: write(%s) failed: %s", Port, strerror(errno));
  }
}


#if 0
static int CW_read(char *string, int len)
{
  int ret;
  
  ret=read (Device, string, len);
  if (ret<0 && errno==EAGAIN) {
    debug ("read(): EAGAIN");
    usleep(10000);
    ret=read (Device, string, len);
  }
  debug ("read(%s)=%d", string, ret);

  if (ret<0) {
    error("Cwlinux: read() failed: %s", strerror(errno));
  }

  usleep(20);
  
  return ret;
}
#endif

static void CW_Goto(int row, int col)
{
  char cmd[6]="\376Gxy\375";
  cmd[2]=(char)col;
  cmd[3]=(char)row;
  CW_write(cmd,5);
}


static void CW12232_define_char (int ascii, char *buffer)
{
  int i, j;
  char cmd[10]="\376Nn123456\375";
  
  cmd[2]=(char)(ascii+1);
  
  // Cwlinux uses a vertical bitmap layout, so
  // we have to kind of 'rotate' the bitmap.
  
  for (i=0; i<6;i++) {
    cmd[3+i]=0;
    for (j=0; j<8;j++) {
      if (buffer[j] & (1<<(5-i))) {
	cmd[3+i]|=(1<<j);
      }
    }
  }
  CW_write(cmd,10);
}


static void CW1602_define_char (int ascii, char *buffer)
{
  int i;
  char cmd[12]="\376Nn12345678\375";

  cmd[2]=(char)(ascii+1);

  for (i=0; i<8; i++) {
    cmd[3+i]=buffer[i];
  }
  CW_write(cmd,12);
  usleep(20);  // delay for cw1602 to settle the character defined!
}


int CW_clear(int full)
{

  memset (FrameBuffer1, ' ', Lcd.rows*Lcd.cols*sizeof(char));

  icon_clear();
  bar_clear();

  if (full) {
    memset (FrameBuffer2, ' ', Lcd.rows*Lcd.cols*sizeof(char));
#if 0
    CW_write("\376X\375",3);
#else
    // for some mysterious reason, we have to sleep after 
    // the command _and_ after the CMD_END...
    usleep(20);
    CW_write("\376X",2);
    usleep(20);
    CW_write("\375",1);
    usleep(20);
#endif
  }

  return 0;
}


static void CW_Brightness(void)
{
  int level;
  char cmd[5]="\376A_\375";
  
  if (cfg_number(NULL, "Brightness", 8, 0, 8, &level)<0) return;
  
  switch (level) {
  case 0:
    // backlight off
    CW_write ("\376F\375", 3);
    break;
  case 8:
    // backlight on
    CW_write ("\376B\375", 3);
    break;
  default:
    // backlight level
    cmd[2]=level;
    CW_write (cmd, 4);
    break;
  }
}


int CW_init(LCD * Self)
{
  char *port;
  int speed;
  // char buffer[16];
  
  Lcd = *Self;

  // Init the framebuffers
  FrameBuffer1 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  FrameBuffer2 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("Cwlinux: framebuffer could not be allocated: malloc() failed");
    return -1;
  }

  if (Port) {
    free(Port);
    Port = NULL;
  }

  port = cfg_get(NULL, "Port",NULL);
  if (port == NULL || *port == '\0') {
    error("Cwlinux: no 'Port' entry in %s", cfg_source());
    return -1;
  }
  Port = strdup(port);

  if (cfg_number(NULL, "Speed", 19200, 9600,19200, &speed)<0) return -1;
  switch (speed) {
  case 9600:
    Speed = B9600;
    break;
  case 19200:
    Speed = B19200;
    break;
  default:
    error("Cwlinux: unsupported speed '%d' in %s", speed, cfg_source());
    return -1;
  }

  debug("using port %s at %d baud", Port, speed);

  Device = CW_open();
  if (Device == -1)
    return -1;

  // this does not work as I'd expect it...
#if 0
  // read firmware version
  CW_read(buffer,sizeof(buffer));
  usleep(100000);
  CW_write ("\3761", 2);
  usleep(100000);
  CW_write ("\375", 1);
  usleep(100000);
  if (CW_read(buffer,2)!=2) {
    info ("unable to read firmware version!");
  }
  info ("Cwlinux Firmware %d.%d", (int)buffer[0], (int)buffer[1]);
#endif

  CW_clear(1);

  // auto line wrap off
  CW_write ("\376D\375", 3);

  // auto scroll off
  // CW_write ("\376R\375", 3);

  // underline cursor off
  CW_write ("\376K\375", 3);

  // backlight on
  CW_write ("\376B\375", 3);

  // backlight brightness
  CW_Brightness();

  if (cfg_number(NULL, "Icons", 0, 0, CHARS, &Icons)<0) return -1;
  if (Icons>0) {
    debug ("reserving %d of %d user-defined characters for icons", Icons, CHARS);
    icon_init(Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres, CHARS, Icons, CW12232_define_char);
    Self->icons=Icons;
    Lcd.icons=Icons;
  }
  
  bar_init(Lcd.rows, Lcd.cols, Lcd.xres, Lcd.yres, CHARS-Icons);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
    
  return 0;
}


int CW_put(int row, int col, char *text)
{
  char *p=FrameBuffer1+row*Lcd.cols+col;
  char *t = text;

  while (*t && col++ <= Lcd.cols) {
    *p++ = *t++;
  }
  return 0;
}


int CW_bar(int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int CW_icon (int num, int seq, int row, int col)
{
  return icon_draw (num, seq, row, col);
}


int CW_flush(void)
{
  int row, col, pos1, pos2;
  int c, equal;

  for (row = 0; row < Lcd.rows; row++) {
    for (col = 0; col < Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c==-1) c=icon_peek(row, col);
      if (c!=-1) {
	if (c!=32) c++; // blank
	FrameBuffer1[row*Lcd.cols+col]=(char)c;
	// invalidate FrameBuffer:
	// Cwlinux does not update the display of a user-defined char
	// if it is redefined only. We have to definitely write it
	// to the display! We force this by invalidating the DoubleBuffer.
	// Fixme: This is bad: we should try to remember which chars
	// got redefined, and invalidate only those...
	FrameBuffer2[row*Lcd.cols+col]=0;
      }
    }
    for (col = 0; col < Lcd.cols; col++) {
      if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) continue;
      CW_Goto(row, col);
      for (pos1=col++, pos2=pos1, equal=0; col<Lcd.cols; col++) {
	if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes five bytes, too.
	  if (++equal>5) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      CW_write (FrameBuffer1+row*Lcd.cols+pos1, pos2-pos1+1);
    }
  }

  memcpy (FrameBuffer2, FrameBuffer1, Lcd.rows*Lcd.cols*sizeof(char));
  
  return 0;
}

int CW12232_flush(void)
{
  bar_process(CW12232_define_char);
  return CW_flush();
}

int CW1602_flush(void)
{
  bar_process(CW1602_define_char);
  return CW_flush();
}


int CW_quit(void)
{
  info("Cwlinux: shutting down.");

  debug("closing port %s", Port);
  close(Device);
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


LCD Cwlinux[] = {
  { name: "CW12232", 
    rows:  4,
    cols:  20,
    xres:  6, 
    yres:  8,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  CW_init,
    clear: CW_clear,
    put:   CW_put,
    bar:   CW_bar,
    icon:  CW_icon,
    gpo:   NULL,
    flush: CW12232_flush,
    quit:  CW_quit
  },
  { name: "CW1602",
    rows:  2,
    cols:  16,
    xres:  5,
    yres:  8,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    icons: 0,
    gpos:  0,
    init:  CW_init,
    clear: CW_clear,
    put:   CW_put,
    bar:   CW_bar,
    icon:  CW_icon,
    gpo:   NULL,
    flush: CW1602_flush,
    quit:  CW_quit
  },
  {NULL}
};
