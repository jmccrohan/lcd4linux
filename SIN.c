/* $Id: SIN.c,v 1.4 2000/12/01 20:42:37 reinelt Exp $
 *
 * driver for SIN router displays
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
 * $Log: SIN.c,v $
 * Revision 1.4  2000/12/01 20:42:37  reinelt
 *
 * added debugging of SIN driver output, probably found the positioning bug (format %02x instead of %2x)
 *
 * Revision 1.3  2000/12/01 07:20:26  reinelt
 *
 * modified text positioning: row starts with 0, column is hexadecimal
 *
 * Revision 1.2  2000/11/28 17:27:19  reinelt
 *
 * changed decimal values for screen, row, column to ascii values (shame on you!)
 *
 * Revision 1.1  2000/11/28 16:46:11  reinelt
 *
 * first try to support display of SIN router
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD SIN[]
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "lock.h"
#include "display.h"

#define XRES 5
#define YRES 8

// Fixme: Bar Support disabled
#define BARS 0

static LCD Lcd;
static char *Port=NULL;
static int Device=-1;

static char Txt[8][40];

static int SIN_open (void)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  if ((pid=lock_port(Port))!=0) {
    if (pid==-1)
      error ("SIN: port %s could not be locked", Port);
    else
      error ("SIN: port %s is locked by process %d", Port, pid);
    return -1;
  }
  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("SIN: open(%s) failed: %s", Port, strerror(errno));
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    error ("SIN: tcgetattr(%s) failed: %s", Port, strerror(errno));
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, B9600);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("SIN: tcsetattr(%s) failed: %s", Port, strerror(errno));
    return -1;
  }
  return fd;
}

static void SIN_debug (char *string, int len)
{
  char buffer[256];
  char *p, *c;
  
  p=buffer;
  c=string;
  while (len-->0) {
    if (isprint(*c)) {
      *p++=*c;
    } else {
      p+=sprintf(p, "\\%02x", *c);
    }
    c++;
  }
  *p='\0';
  debug ("sending '%s'", buffer);
}

static void SIN_write (char *string, int len)
{
  SIN_debug(string, len);
  if (Device==-1) return;
  if (write (Device, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (Device, string, len)>=0) return;
    }
    error ("SIN: write(%s) failed: %s", Port, strerror(errno));
  }
}

int SIN_clear (void)
{
  int row, col;

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      Txt[row][col]='\t';
    }
  }
  
  SIN_write ("\033 ",2);
  return 0;
}

int SIN_init (LCD *Self)
{
  char *port;

  Lcd=*Self;

  if (Port) {
    free (Port);
    Port=NULL;
  }

  port=cfg_get ("Port");
  if (port==NULL || *port=='\0') {
    error ("SIN: no 'Port' entry in %s", cfg_file());
    return -1;
  }
  Port=strdup(port);

  debug ("using port %s at 9600 baud", Port);

  Device=SIN_open();
  if (Device==-1) return -1;

  SIN_write ("\015", 1);  // send 'Enter'
  // Fixme: should we read the identifier here....
  SIN_write ("\033S0", 3); // select screen #0
  sleep (1); // FIXME: handshaking
  SIN_clear();

  return 0;
}

int SIN_put (int row, int col, char *text)
{
  char *p=&Txt[row][col];
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}

int SIN_flush (void)
{
  char buffer[256]="\015\033T"; // place text
  char *p;
  int  row, col;
  
  for (row=0; row<Lcd.rows; row++) {
    buffer[3]='0'+row;
    for (col=0; col<Lcd.cols; col++) {
      if (Txt[row][col]=='\t') continue;
      sprintf (buffer+4, "%02x", col);
      for (p=buffer+6; col<Lcd.cols; col++, p++) {
	if (Txt[row][col]=='\t') break;
	*p=Txt[row][col];
      }
      SIN_write (buffer, p-buffer);
    }
  }
  return 0;
}

int SIN_quit (void)
{
  debug ("closing port %s", Port);
  close (Device);
  unlock_port(Port);
  return (0);
}

LCD SIN[] = {
  { "SIN", 8, 40, XRES, YRES, BARS, SIN_init, SIN_clear, SIN_put, NULL, SIN_flush, SIN_quit },
  { NULL }
};
