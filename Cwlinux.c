/* $Id: Cwlinux.c,v 1.5 2003/02/22 07:53:09 reinelt Exp $
 *
 * driver for Cwlinux serial display modules
 *
 * Copyright 2002 by Andrew Ip (aip@cwlinux.com)
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
 * $Log: Cwlinux.c,v $
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

#define XRES 6
#define YRES 8
#define CHARS 7

#define DELAY 200

static LCD Lcd;
static char *Port = NULL;
static speed_t Speed;
static int Device = -1;

static char Txt[4][20];


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
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("Cwlinux: write(%s) failed: %s", Port, strerror(errno));
  }
  usleep(DELAY);
}


static int CW_read(char *string, int len)
{
  int ret;
  
  ret=read (Device, string, len);
  if (ret<0 && errno==EAGAIN) {
    usleep(1000);
    ret=read (Device, string, len);
  }
  debug ("read(%s)=%d", string, ret);

  if (ret<0) {
    error("Cwlinux: read() failed: %s", strerror(errno));
  }

  usleep(DELAY);
  
  return ret;
}


static void CW_Goto(int row, int col)
{
  char cmd[5]="\376Gxy\375";
  
  cmd[2]=(char)col;
  cmd[3]=(char)row;
  CW_write(cmd,5);
  usleep(DELAY);

}

static void CW_define_char (int ascii, char *buffer)
{
  int i, j;
  char cmd[10]="\376Nn123456\375";
  
  cmd[2]=(char)ascii+1;
  
  // Cwlinux uses a vertical bitmap layout, so
  // we have to kind of 'rotate' the bitmap.
  
  for (i=0; i<6;i++) {
    cmd[3+i]=0;
    for (j=0; j<8;j++) {
      if (buffer[j] & (1<<(5-i))) {
	cmd[3+i]|=(1<<(7-j));
      }
    }
  }
  CW_write(cmd,10);
  usleep(DELAY);
}

int CW_clear(void)
{
  int row, col;

  for (row = 0; row < Lcd.rows; row++) {
    for (col = 0; col < Lcd.cols; col++) {
      Txt[row][col] = '\t';
    }
  }

  bar_clear();

  CW_write("\376X\375",3);
  usleep(1000);

  return 0;
}

int CW_init(LCD * Self)
{
  char *port;
  char *speed;
  char buffer[16];
  
  Lcd = *Self;

  if (Port) {
    free(Port);
    Port = NULL;
  }

  port = cfg_get("Port",NULL);
  if (port == NULL || *port == '\0') {
    error("Cwlinux: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  Port = strdup(port);

  speed = cfg_get("Speed","19200");

  switch (atoi(speed)) {
  case 9600:
    Speed = B9600;
    break;
  case 19200:
    Speed = B19200;
    break;
  default:
    error("Cwlinux: unsupported speed '%s' in %s", speed, cfg_file());
    return -1;
  }

  debug("using port %s at %d baud", Port, atoi(speed));

  Device = CW_open();
  if (Device == -1)
    return -1;

  // read firmware version
  CW_read(buffer,sizeof(buffer));
  CW_write ("\3761\375", 3);
  if (CW_read(buffer,2)!=2) {
    info ("unable to read firmware version!");
  } else {
    info ("Cwlinux Firmware %d.%d", (int)buffer[0], (int)buffer[1]);
  }
  info ("Cwlinux Firmware %d.%d", (int)buffer[0], (int)buffer[1]);

  CW_clear();

  // auto line wrap off
  CW_write ("\376D\375", 3);
  usleep(DELAY);

  // auto scroll off
  CW_write ("\376R\375", 3);
  usleep(DELAY);

  // underline cursor off
  CW_write ("\376K\375", 3);
  usleep(DELAY);

  // backlight on
  CW_write ("\376B\375", 3);
  usleep(DELAY);

  bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
    
  return 0;
}


int CW_put(int row, int col, char *text)
{
  char *p = &Txt[row][col];
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


int CW_flush(void)
{
  char buffer[256];
  char *p;
  int c, row, col;

  bar_process(CW_define_char);

  for (row = 0; row < Lcd.rows; row++) {
    for (col = 0; col < Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	if (c!=32) c++; //blank
	Txt[row][col]=(char)c;
      }
    }
    for (col = 0; col < Lcd.cols; col++) {
      if (Txt[row][col] == '\t')
	continue;
      CW_Goto(row, col);
      usleep(DELAY);
      for (p = buffer; col < Lcd.cols; col++, p++) {
	if (Txt[row][col] == '\t')
	  break;
	*p = Txt[row][col];
      }
      CW_write(buffer, p - buffer);
    }
  }
  return 0;
}


int CW_quit(void)
{
  debug("closing port %s", Port);
  close(Device);
  unlock_port(Port);
  return (0);
}


LCD Cwlinux[] = {
  {name: "CW12232", 
   rows:  4, 
   cols:  20, 
   xres:  XRES, 
   yres:  YRES, 
   bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
   gpos:  0, 
   init:  CW_init, 
   clear: CW_clear,
   put:   CW_put, 
   bar:   CW_bar, 
   gpo:   NULL, 
   flush: CW_flush, 
   quit:  CW_quit
  },
  {NULL}
};
