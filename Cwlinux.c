/* $Id: Cwlinux.c,v 1.3 2002/09/12 05:24:54 reinelt Exp $
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

#define LCD_CMD			254
#define LCD_CMD_END		253
#define LCD_INIT_CHINESE_T	56
#define LCD_INIT_CHINESE_S	55
#define LCD_LIGHT_ON		66
#define LCD_LIGHT_OFF		70
#define LCD_CLEAR		88
#define LCD_SET_INSERT		71
#define LCD_INIT_INSERT		72
#define LCD_SET_BAUD		57
#define LCD_ENABLE_WRAP		67
#define LCD_DISABLE_WRAP	68
#define LCD_SETCHAR		78
#define LCD_ENABLE_CURSOR	81
#define LCD_DISABLE_CURSOR	82

#define LCD_LENGTH		20

#define DELAY			20
#define UPDATE_DELAY		0	/* 1 sec */
#define SETUP_DELAY		0	/* 2 sec */

static LCD Lcd;
static char *Port = NULL;
static speed_t Speed;
static int Device = -1;

static char Txt[4][20];

static int CwLnx_open(void)
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
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset) == -1) {
    error("Cwlinux: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  return fd;
}

static void CwLnx_write(char *string, int len)
{
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("Cwlinux: write(%s) failed: %s", Port, strerror(errno));
  }

  // Fixme: Why?
  // usleep(DELAY);
}

static void CwLnx_Goto(int row, int col)
{
  char cmd[5];
  
  cmd[0]=LCD_CMD;
  cmd[1]=LCD_SET_INSERT;
  cmd[2]=(char)col;
  cmd[3]=(char)row;
  cmd[4]=LCD_CMD_END;
  
  CwLnx_write(cmd,5);
}

static void CwLnx_define_char (int ascii, char *buffer)
{
  char cmd[10];
  int i, j;
  
  cmd[0]=LCD_CMD;
  cmd[1]=LCD_SETCHAR;
  cmd[2]=(char)ascii;
  
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
  cmd[10]=LCD_CMD_END;
  CwLnx_write(cmd,10);
}

int CwLnx_clear(void)
{
  char cmd[3];
  
  int row, col;
  for (row = 0; row < Lcd.rows; row++) {
    for (col = 0; col < Lcd.cols; col++) {
      Txt[row][col] = '\t';
    }
  }

  bar_clear();

  cmd[0]=LCD_CMD;
  cmd[1]=LCD_CLEAR;
  cmd[2]=LCD_CMD_END;
  CwLnx_write(cmd,3);
  
  return 0;
}

int CwLnx_init(LCD * Self)
{
  char *port;
  char *speed;
  char  cmd[3];

  Lcd = *Self;

  if (Port) {
    free(Port);
    Port = NULL;
  }

  port = cfg_get("Port");
  if (port == NULL || *port == '\0') {
    error("Cwlinux: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  Port = strdup(port);

  speed = cfg_get("Speed") ? : "19200";

  switch (atoi(speed)) {
  case 1200:
    Speed = B1200;
    break;
  case 2400:
    Speed = B2400;
    break;
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

  Device = CwLnx_open();
  if (Device == -1)
    return -1;

  bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block
    
  CwLnx_clear();

  // turn backlight on
  cmd[0]=LCD_CMD;
  cmd[1]=LCD_LIGHT_ON;
  cmd[2]=LCD_CMD_END;
  CwLnx_write(cmd,3);

  // Fixme:
  // enable (disable?) linewrap
  cmd[0]=LCD_CMD;
  cmd[1]=LCD_ENABLE_WRAP;
  cmd[2]=LCD_CMD_END;
  CwLnx_write(cmd,3);

  return 0;
}

int CwLnx_put(int row, int col, char *text)
{
  char *p = &Txt[row][col];
  char *t = text;

  while (*t && col++ <= Lcd.cols) {
    *p++ = *t++;
  }
  return 0;
}

int CwLnx_bar(int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}

int CwLnx_flush(void)
{
  char buffer[256];
  char *p;
  int c, row, col;

  bar_process(CwLnx_define_char);

  for (row = 0; row < Lcd.rows; row++) {
    for (col = 0; col < Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	Txt[row][col]=(char)c;
      }
    }
    for (col = 0; col < Lcd.cols; col++) {
      if (Txt[row][col] == '\t')
	continue;
      CwLnx_Goto(row, col);
      for (p = buffer; col < Lcd.cols; col++, p++) {
	if (Txt[row][col] == '\t')
	  break;
	*p = Txt[row][col];
      }
      CwLnx_write(buffer, p - buffer);
    }
  }
  return 0;
}

int CwLnx_quit(void)
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
   init:  CwLnx_init, 
   clear: CwLnx_clear,
   put:   CwLnx_put, 
   bar:   CwLnx_bar, 
   gpo:   NULL, 
   flush: CwLnx_flush, 
   quit:  CwLnx_quit
  },
  {NULL}
};
