/* $Id: BeckmannEgle.c,v 1.9 2002/08/19 09:43:43 reinelt Exp $
 *
 * driver for Beckmann+Egle mini terminals
 *
 * Copyright 2000 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: BeckmannEgle.c,v $
 * Revision 1.9  2002/08/19 09:43:43  reinelt
 * BeckmannEgle using new generic bar functions
 *
 * Revision 1.8  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
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
 * Revision 1.5  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.4  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.3  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.2  2000/04/30 06:40:42  reinelt
 *
 * bars for Beckmann+Egle driver
 *
 * Revision 1.1  2000/04/28 05:19:55  reinelt
 *
 * first release of the Beckmann+Egle driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD BeckmannEgle[]
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
#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 )

typedef struct {
  int cols;
  int rows;
  int type;
} MODEL;

static LCD Lcd;
static char *Port=NULL;
static int Device=-1;
static int Type=-1;

static char Txt[4][40];

static MODEL Model[]= {{ 16, 1,  0 },
		       { 16, 2,  1 },
		       { 16, 4,  2 },
		       { 20, 1,  3 },
		       { 20, 2,  4 },
		       { 20, 4,  5 },
		       { 24, 1,  6 },
		       { 24, 2,  7 },
		       { 32, 1,  8 },
		       { 32, 2,  9 },
		       { 40, 1, 10 },
		       { 40, 2, 11 },
		       { 40, 4, 12 },
		       {  0, 0,  0 }};




static int BE_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("BeckmannEgle: port %s could not be locked", Port);
    else
      error ("BeckmannEgle: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("BeckmannEgle: open(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("BeckmannEgle: tcgetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  cfmakeraw(&portset);           // 8N1
  portset.c_cflag |= CSTOPB;     // 2 stop bits
  cfsetospeed(&portset, B9600);  // 9600 baud
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("BeckmannEgle: tcsetattr(%s) failed: %s", Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  return fd;
}


static void BE_write (char *string, int len)
{
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("BeckmannEgle: write(%s) failed: %s", Port, strerror(errno));
  }
}


static void BE_define_char (int ascii, char *buffer)
{
  int  i;
  char cmd[32];
  char *p;
  
  p=cmd;
  *p++='\033';
  *p++='&';
  *p++='T';            // enter transparent mode
  *p++='\0';           // write cmd
  *p++=0x40|8*ascii;   // write CGRAM
  for (i=0; i<YRES; i++) {
    *p++='\1';         // write data
    *p++=buffer[i];    // character bitmap
  }
  *p++='\377';         // leave transparent mode

  BE_write (cmd, p-cmd);

}


int BE_clear (void)
{
  int row, col;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
    }
  }

  bar_clear();

  BE_write ("\033&#", 3);
  return 0;
}


int BE_init (LCD *Self)
{
  int i, rows=-1, cols=-1;
  char *port, *s;
  char buffer[5];

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port");
  if (port==NULL || *port=='\0') {
    error ("BeckmannEgle: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  Port=strdup(port);

  s=cfg_get("Type");
  if (s==NULL || *s=='\0') {
    error ("BeckmannEgle: no 'Type' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("BeckmannEgle: bad type '%s'", s);
    return -1;
  }

  Type=-1;
  for (i=0; Model[i].cols; i++) {
    if (Model[i].cols==cols && Model[i].rows==rows) {
      Type=Model[i].type;
      break;
    }
  }
  if (Type==-1) {
    error ("BeckmannEgle: unsupported model '%dx%d'", cols, rows);
    return -1;
  }

  debug ("using %dx%d display at port %s", cols, rows, Port);

  Self->rows=rows;
  Self->cols=cols;
  Lcd=*Self;

  Device=BE_open();
  if (Device==-1) return -1;

  snprintf (buffer, sizeof(buffer), "\033&s%c", Type);
  BE_write (buffer, 4);    // select display type
  BE_write ("\033&D", 3);  // cursor off

  bar_init(rows, cols, XRES, YRES, CHARS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block

  BE_clear();

  return 0;
}


int BE_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int BE_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int BE_flush (void)
{
  char buffer[256]="\033[y;xH";
  char *p;
  int c, row, col;
  
  bar_process(BE_define_char);
  
  for (row=0; row<Lcd.rows; row++) {
    buffer[2]=row;
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	Txt[row][col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      buffer[4]=col;
      for (p=buffer+6; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      BE_write (buffer, p-buffer);
    }
  }
  return 0;
}


int BE_quit (void)
{
  debug ("closing port %s", Port);
  close (Device);
  unlock_port(Port);
  return 0;
}


LCD BeckmannEgle[] = {
  { name: "BLC100x",
    rows:  0,
    cols:  0,
    xres:  XRES,
    yres:  YRES,
    bars:  BARS,
    gpos:  0,
    init:  BE_init,
    clear: BE_clear,
    put:   BE_put,
    bar:   BE_bar,
    gpo:   NULL,
    flush: BE_flush,
    quit:  BE_quit 
  },
  { NULL }
};
