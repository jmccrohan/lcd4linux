/* $Id: drv_generic.c,v 1.1 2004/01/20 04:51:39 reinelt Exp $
 *
 * generic driver helper for text- and graphic-based displays
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_generic.c,v $
 * Revision 1.1  2004/01/20 04:51:39  reinelt
 * moved generic stuff from drv_MatrixOrbital to drv_generic
 * implemented new-stylish bars which are nearly finished
 *
 */

/* 
 *
 * exported fuctions:
 *
 * Fixme: document me!
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
#include "plugin.h"
#include "lock.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic.h"


static char   *Driver;
static char   *Port;
static speed_t Speed;
static int     Device=-1;


int DROWS, DCOLS; // display size
int LROWS, LCOLS; // layout size
int XRES,  YRES;  // pixels of one char cell
int CHARS;        // number of user-defineable characters


char *LayoutFB  = NULL;
char *DisplayFB = NULL;


int drv_generic_serial_open (char *driver, char *port, speed_t speed)
{
  int fd;
  pid_t pid;
  struct termios portset;
  
  Driver = driver;
  Port   = port;
  Speed  = speed;
  
  if ((pid=lock_port(port))!=0) {
    if (pid==-1)
      error ("%s: port %s could not be locked", Driver, Port);
    else
      error ("%s: port %s is locked by process %d", Driver, Port, pid);
    return -1;
  }

  fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    error ("%s: open(%s) failed: %s", Driver, Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }

  if (tcgetattr(fd, &portset)==-1) {
    error ("%s: tcgetattr(%s) failed: %s", Driver, Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }

  cfmakeraw(&portset);
  cfsetospeed(&portset, Speed);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    error ("%s: tcsetattr(%s) failed: %s", Driver, Port, strerror(errno));
    unlock_port(Port);
    return -1;
  }
  
  Device=fd;
  return Device;
}


int drv_generic_serial_read (char *string, int len)
{
  int run, ret;
  
  if (Device==-1) return -1;
  for (run=0; run<10; run++) {
    ret=read (Device, string, len);
    if (ret>=0 || errno!=EAGAIN) break;
    debug ("read(): EAGAIN");
    usleep(1000);
  }
  
  if (ret<0) {
    error("%s: read(%s, %d) failed: %s", Driver, Port, len, strerror(errno));
  } else if (ret!=len) {
    error ("%s: partial read: len=%d ret=%d", Driver, len, ret);
  }
  
  return ret;
}


void drv_generic_serial_write (char *string, int len)
{
  int run, ret;
  
  if (Device==-1) return;
  for (run=0; run<10; run++) {
    ret=write (Device, string, len);
    if (ret>=0 || errno!=EAGAIN) break;
    debug ("write(): EAGAIN");
    usleep(1000);
  }
  
  if (ret<0) {
    error ("MatrixOrbital: write(%s) failed: %s", Port, strerror(errno));
  } else if (ret!=len) {
    error ("MatrixOrbital: partial write: len=%d ret=%d", len, ret);
  }
  
  return;
}


int drv_generic_serial_close (void)
{
  debug ("%s: closing port %s", Driver, Port);
  close (Device);
  unlock_port(Port);
  return 0;
}


void drv_generic_text_resizeFB (int rows, int cols)
{
  char *newFB;
  int row, col;
  
  // Fixme: resize Bar FB too!!!!


  // Layout FB is large enough
  if (rows<=LROWS && cols<=LCOLS)
    return;
  
  // allocate new Layout FB
  newFB=malloc(cols*rows*sizeof(char));
  memset (newFB, ' ', rows*cols*sizeof(char));

  // transfer contents
  if (LayoutFB!=NULL) {
    for (row=0; row<LROWS; row++) {
      for (col=0; col<LCOLS; col++) {
	newFB[row*cols+col]=LayoutFB[row*LCOLS+col];
      }
    }
    free (LayoutFB);
  }
  
  LayoutFB = newFB;
  LCOLS    = cols;
  LROWS    = rows;
}



// ****************************************
// ***        widget callbacks          ***
// ****************************************


int drv_generic_text_draw_text (WIDGET *W, int goto_len, 
			       void (*drv_goto)(int row, int col), 
			       void (*drv_write)(char *buffer, int len))
{
  WIDGET_TEXT *T=W->data;
  char *txt, *fb1, *fb2;
  int row, col, len, end;
  
  row=W->row;
  col=W->col;
  txt=T->buffer;
  len=strlen(txt);
  end=col+len;
  
  // maybe grow layout framebuffer
  drv_generic_text_resizeFB (row, col+len-1);

  fb1 = LayoutFB  + row*LCOLS;
  fb2 = DisplayFB + row*DCOLS;
  
  // transfer new text into layout buffer
  memcpy (fb1+col, txt, len);
  
  for (; col<=end; col++) {
    int pos1, pos2, equal;
    if (fb1[col]==fb2[col]) continue;
    drv_goto (row, col);
    for (pos1=col, pos2=pos1, col++, equal=0; col<=end; col++) {
      if (fb1[col]==fb2[col]) {
	// If we find just one equal byte, we don't break, because this 
	// would require a goto, which takes several bytes, too.
	if (++equal>goto_len) break;
      } else {
	pos2=col;
	equal=0;
      }
    }
    memcpy    (fb2+pos1, fb1+pos1, pos2-pos1+1);
    drv_write (fb2+pos1,           pos2-pos1+1);
  }
  
  return 0;
}


// initialize text driver
int drv_generic_text_init (char *Driver)
{
  // init display framebuffer
  DisplayFB = (char*)malloc(DCOLS*DROWS*sizeof(char));
  memset (DisplayFB, ' ', DROWS*DCOLS*sizeof(char));
  
  // init layout framebuffer
  LROWS = 0;
  LCOLS = 0;
  LayoutFB=NULL;
  drv_generic_text_resizeFB (DROWS, DCOLS);
  
  // sanity check
  if (LayoutFB==NULL || DisplayFB==NULL) {
    error ("%s: framebuffer could not be allocated: malloc() failed", Driver);
    return -1;
  }
  
  return 0;
}


// close driver
int drv_generic_quit (void) {
  
  if (LayoutFB) {
    free(LayoutFB);
    LayoutFB=NULL;
  }
  
  if (DisplayFB) {
    free(DisplayFB);
    DisplayFB=NULL;
  }
  
  return (0);
}
