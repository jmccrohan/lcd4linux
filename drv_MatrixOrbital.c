/* $Id: drv_MatrixOrbital.c,v 1.5 2004/01/11 09:26:15 reinelt Exp $
 *
 * new style driver for Matrix Orbital serial display modules
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
 * $Log: drv_MatrixOrbital.c,v $
 * Revision 1.5  2004/01/11 09:26:15  reinelt
 * layout starts to exist...
 *
 * Revision 1.4  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.3  2004/01/10 17:34:40  reinelt
 * further matrixOrbital changes
 * widgets initialized
 *
 * Revision 1.2  2004/01/10 10:20:22  reinelt
 * new MatrixOrbital changes
 *
 * Revision 1.1  2004/01/09 17:03:07  reinelt
 * initiated transfer to new driver architecture
 * new file 'drv.c' will someday replace 'display.c'
 * new file 'drv_MatrixOrbital.c' will replace 'MatrixOrbital.c'
 * due to this 'soft' transfer lcd4linux should stay usable during the switch
 * (at least I hope so)
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_MatrixOrbital
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
#include "drv.h"
#include "bar.h"
#include "icon.h"
#include "widget.h"


// these values are hardcoded
#define XRES 5
#define YRES 8
#define CHARS 8

static char *Port=NULL;
static speed_t Speed;
static int Device=-1;
static int Model;

static int ROWS, COLS, GPOS;
static int ICONS, PROTOCOL;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;
static int GPO[8];


typedef struct {
  int type;
  char *name;
  int rows;
  int cols;
  int gpos;
  int protocol;
} MODEL;

// Fixme #1: number of gpo's should be verified
// Fixme #2: protocol should be verified

static MODEL Models[] = {
  { 0x01, "LCD0821",      2,  8, 0, 1 },
  { 0x03, "LCD2021",      2, 20, 0, 1 },
  { 0x04, "LCD1641",      4, 16, 0, 1 },
  { 0x05, "LCD2041",      4, 20, 0, 1 },
  { 0x06, "LCD4021",      2, 40, 0, 1 },
  { 0x07, "LCD4041",      4, 40, 0, 1 },
  { 0x08, "LK202-25",     2, 20, 0, 2 },
  { 0x09, "LK204-25",     4, 20, 0, 2 },
  { 0x0a, "LK404-55",     4, 40, 0, 2 },
  { 0x0b, "VFD2021",      2, 20, 0, 1 },
  { 0x0c, "VFD2041",      4, 20, 0, 1 },
  { 0x0d, "VFD4021",      2, 40, 0, 1 },
  { 0x0e, "VK202-25",     2, 20, 0, 1 },
  { 0x0f, "VK204-25",     4, 20, 0, 1 },
  { 0x10, "GLC12232",    -1, -1, 0, 1 },
  { 0x13, "GLC24064",    -1, -1, 0, 1 },
  { 0x15, "GLK24064-25", -1, -1, 0, 1 },
  { 0x22, "GLK12232-25", -1, -1, 0, 1 },
  { 0x31, "LK404-AT",     4, 40, 0, 2 },
  { 0x32, "VFD1621",      2, 16, 0, 1 },
  { 0x33, "LK402-12",     2, 40, 0, 2 },
  { 0x34, "LK162-12",     2, 16, 0, 2 },
  { 0x35, "LK204-25PC",   4, 20, 0, 2 },
  { 0x36, "LK202-24-USB", 2, 20, 0, 2 },
  { 0x38, "LK204-24-USB", 4, 20, 0, 2 },
  { 0xff, "Unknown",     -1, -1, 0, 0 }
};


static int drv_MO_open (void)
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


static int drv_MO_read (char *string, int len)
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
    error("MatrixOrbital: read(%s, %d) failed: %s", Port, len, strerror(errno));
  }
  
  return ret;
}


static void drv_MO_write (char *string, int len)
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
  }

  // Fixme
  if (ret!=len) {
    error ("MatrixOrbital: partial write: len=%d ret=%d", len, ret);
  }

  return;
}


static int drv_MO_contrast (int contrast)
{
  char buffer[4];
  
  if (contrast<0  ) contrast=0;
  if (contrast>255) contrast=255;
  snprintf (buffer, 4, "\376P%c", contrast);
  drv_MO_write (buffer, 3);
  return contrast;
}


static void drv_MO_define_char (int ascii, char *buffer)
{
  char cmd[3]="\376N";

  cmd[2]=(char)ascii;
  drv_MO_write (cmd, 3);
  drv_MO_write (buffer, 8);
}


static int drv_MO_clear (int full)
{
  int gpo;
  
  memset (FrameBuffer1, ' ', ROWS*COLS*sizeof(char));
  
  icon_clear();
  bar_clear();
  memset(GPO, 0, sizeof(GPO));

  if (full) {
    memset (FrameBuffer2, ' ', ROWS*COLS*sizeof(char));
    switch (PROTOCOL) {
    case 1:
      drv_MO_write ("\014",  1);  // Clear Screen
      drv_MO_write ("\376V", 2);  // GPO off
      break;
    case 2:
      drv_MO_write ("\376\130",  2);  // Clear Screen
      for (gpo=1; gpo<=GPOS; gpo++) {
	char cmd1[3]="\376V";
	char cmd2[4]="\376\300x\377";
	cmd1[2]=(char)gpo;
	cmd2[2]=(char)gpo;
	drv_MO_write (cmd1, 3);  // GPO off
	drv_MO_write (cmd2, 4);  // PWM full power
      }
      break;
    }
  }
  
  return 0;
}

static void drv_MO_goto (int row, int col)
{
  char cmd[5]="\376Gyx";
  cmd[2]=(char)col+1;
  cmd[3]=(char)row+1;
  drv_MO_write(cmd,4);
}


static int drv_MO_put (int row, int col, char *text)
{
  char *p=FrameBuffer1+row*COLS+col;
  char *t=text;
  
  while (*t && col++<=COLS) {
    *p++=*t++;
  }
  return 0;
}


static int drv_MO_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


static int drv_MO_icon (int num, int seq, int row, int col)
{
  return icon_draw (num, seq, row, col);
}


static int drv_MO_gpo (int num, int val)
{
  if (num>=GPOS) 
    return -1;

  GPO[num]=val;
  
  // Fixme
  GPO[num]=255;

  return 0;
}


// start display
static int drv_MO_start (char *section)
{
  int i;  
  char *port, buffer[256];
  char *model;
  
  if (Port) {
    free (Port);
    Port=NULL;
  }

  model=cfg_get(section, "Model", NULL);
  if (model!=NULL && *model!='\0') {
    for (i=0; Models[i].type!=0xff; i++) {
      if (strcasecmp(Models[i].name, model)==0) break;
    }
    if (Models[i].type==0xff) {
      error ("MatrixOrbital: %s.Model '%s' is unknown from %s", section, model, cfg_source());
      return -1;
    }
    Model=i;
    info ("MatrixOrbital: using model '%s'", Models[Model].name);
  } else {
    info ("MatrixOrbital: no '%s.Model' entry from %s, auto-dedecting", section, cfg_source());
    Model=-1;
  }
  
  
  port=cfg_get(section, "Port", NULL);
  if (port==NULL || *port=='\0') {
    error ("MatrixOrbital: no '%s.Port' entry from %s", section, cfg_source());
    return -1;
  }
  Port=strdup(port);

  if (cfg_number(section, "Speed", 19200, 1200, 19200, &i)<0) return -1;
  switch (i) {
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
    error ("MatrixOrbital: unsupported speed '%d' from %s", i, cfg_source());
    return -1;
  }    
  
  info ("MatrixOrbital: using port '%s' at %d baud", Port, i);
  
  Device=drv_MO_open();
  if (Device==-1) return -1;

  // read module type
  drv_MO_write ("\3767", 2);
  usleep(1000);
  drv_MO_read (buffer, 1);
  for (i=0; Models[i].type!=0xff; i++) {
    if (Models[i].type == (int)*buffer) break;
  }
  info ("MatrixOrbital: display identifies itself as a '%s' (type 0x%02x)", 
	Models[i].name, Models[i].type);
  
  // auto-dedection
  if (Model==-1) Model=i;
  
  // auto-dedection matches specified model?
  if (Model!=i) {
    error ("MatrixOrbital: %s.Model '%s' from %s does not match dedected Model '%s'", 
	   section, model, cfg_source(), Models[i].name);
    return -1;
  }

  // read serial number
  drv_MO_write ("\3765", 2);
  usleep(100000);
  drv_MO_read (buffer, 2);
  info ("MatrixOrbital: display reports serial number 0x%x", *(short*)buffer);
  
  // read version number
  drv_MO_write ("\3766", 2);
  usleep(100000);
  drv_MO_read (buffer, 1);
  info ("MatrixOrbital: display reports firmware version 0x%x", *buffer);

  
  // initialize global variables
  ROWS     = Models[Model].rows;
  COLS     = Models[Model].cols;
  GPOS     = Models[Model].gpos;
  PROTOCOL = Models[Model].protocol;


  // init the framebuffers
  FrameBuffer1 = (char*)malloc(COLS*ROWS*sizeof(char));
  FrameBuffer2 = (char*)malloc(COLS*ROWS*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("MatrixOrbital: framebuffer could not be allocated: malloc() failed");
    return -1;
  }
  
  // init Icons
  if (cfg_number(NULL, "Icons", 0, 0, CHARS, &ICONS)<0) return -1;
  if (ICONS>0) {
    debug ("reserving %d of %d user-defined characters for icons", ICONS, CHARS);
    icon_init(ROWS, COLS, XRES, YRES, CHARS, ICONS, drv_MO_define_char);
  }
  
  // init Bars
  bar_init(ROWS, COLS, XRES, YRES, CHARS-ICONS);
  bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
  bar_add_segment(255,255,255,255); // ASCII 255 = block
  
  
  drv_MO_clear(1);
  // Fixme: where to init contrast?
  drv_MO_contrast(160);

  drv_MO_write ("\376B", 3);  // backlight on
  drv_MO_write ("\376K", 2);  // cursor off
  drv_MO_write ("\376T", 2);  // blink off
  drv_MO_write ("\376D", 2);  // line wrapping off
  drv_MO_write ("\376R", 2);  // auto scroll off

  return 0;
}


static int drv_MO_flush (void)
{
  int row, col, pos1, pos2;
  int c, equal;
  int gpo;
  
  bar_process(drv_MO_define_char);
  
  for (row=0; row<ROWS; row++) {
    for (col=0; col<COLS; col++) {
      c=bar_peek(row, col);
      if (c==-1) c=icon_peek(row, col);
      if (c!=-1) {
	FrameBuffer1[row*COLS+col]=(char)c;
      }
    }
    for (col=0; col<COLS; col++) {
      if (FrameBuffer1[row*COLS+col]==FrameBuffer2[row*COLS+col]) continue;
      drv_MO_goto (row, col);
      for (pos1=col++, pos2=pos1, equal=0; col<COLS; col++) {
	if (FrameBuffer1[row*COLS+col]==FrameBuffer2[row*COLS+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes one byte, too.
	  if (++equal>5) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      drv_MO_write (FrameBuffer1+row*COLS+pos1, pos2-pos1+1);
    }
  }
  
  memcpy (FrameBuffer2, FrameBuffer1, ROWS*COLS*sizeof(char));
  
  switch (PROTOCOL) {
  case 1:
    if (GPO[0]) {
      drv_MO_write ("\376W", 2);  // GPO on
    } else {
      drv_MO_write ("\376V", 2);  // GPO off
    }
    break;
  case 2:
    for (gpo=1; gpo<=GPOS; gpo++) {
      char cmd[3]="\376";
      cmd[1]=GPO[gpo]? 'W':'V';
      cmd[2]=(char)gpo;
      drv_MO_write (cmd, 3);
    }
    break;
  }
  
  return 0;
}



// ****************************************
// ***            plugins               ***
// ****************************************


static void plugin_contrast (RESULT *result, RESULT *arg1)
{
  char buffer[4];
  double contrast;
  
  contrast=R2N(arg1);
  if (contrast<0  ) contrast=0;
  if (contrast>255) contrast=255;
  snprintf (buffer, 4, "\376P%c", (int)contrast);
  drv_MO_write (buffer, 3);
  
  SetResult(&result, R_NUMBER, &contrast); 
}


static void plugin_backlight (RESULT *result, RESULT *arg1)
{
  char buffer[4];
  double backlight;

  backlight=R2N(arg1);
  if (backlight<-1  ) backlight=-1;
  if (backlight>255) backlight=255;
  if (backlight<0) {
    // backlight off
    snprintf (buffer, 3, "\376F");
    drv_MO_write (buffer, 2);
  } else {
    // backlight on for n minutes
    snprintf (buffer, 4, "\376B%c", (int)backlight);
    drv_MO_write (buffer, 3);
  }
  SetResult(&result, R_NUMBER, &backlight); 
}


static void plugin_gpo (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  int num;
  double val;
  char cmd[3]="\376";
  
  num=R2N(arg1);
  val=R2N(arg2);
  
  if (num<1) num=1;
  if (num>6) num=6;
  
  if (val>=1.0) {
    val=1.0;
  } else {
    val=0.0;
  }
  
  switch (PROTOCOL) {
  case 1:
    if (num==0) {
      if (val>=1.0) {
	drv_MO_write ("\376W", 2);  // GPO on
      } else {
	drv_MO_write ("\376V", 2);  // GPO off
      }
    } else {
      error("Fixme");
      val=-1.0;
    }
    break;
    
  case 2:
    if (val>=1.0) {
      cmd[1]='W';  // GPO on
    } else {
      cmd[1]='V';  // GPO off
    }
    cmd[2]=(char)num;
    drv_MO_write (cmd, 3);
    break;
  }
  
  SetResult(&result, R_NUMBER, &val); 
}


static void plugin_pwm (RESULT *result, RESULT *arg1, RESULT *arg2)
{
  int    num;
  double val;
  char   cmd[4]="\376\300";
  
  num=R2N(arg1);
  if (num<1) num=1;
  if (num>6) num=6;
  cmd[2]=(char)num;
  
  val=R2N(arg2);
  if (val<  0.0) val=  0.0;
  if (val>255.0) val=255.0;
  cmd[3]=(char)val;

  drv_MO_write (cmd, 4);

  SetResult(&result, R_NUMBER, &val); 
}


static void plugin_rpm (RESULT *result, RESULT *arg1)
{
  int    num;
  double val;
  char   cmd[3]="\376\301";
  char   buffer[7];
  
  num=R2N(arg1);
  if (num<1) num=1;
  if (num>6) num=6;
  cmd[2]=(char)num;
  
  drv_MO_write (cmd, 3);
  usleep(100000);
  drv_MO_read (buffer, 7);
  
  debug ("rpm: buffer[0]=0x%01x", buffer[0]);
  debug ("rpm: buffer[1]=0x%01x", buffer[1]);
  debug ("rpm: buffer[2]=0x%01x", buffer[2]);
  debug ("rpm: buffer[3]=0x%01x", buffer[3]);
  debug ("rpm: buffer[4]=0x%01x", buffer[4]);
  debug ("rpm: buffer[5]=0x%01x", buffer[5]);
  debug ("rpm: buffer[6]=0x%01x", buffer[6]);

  SetResult(&result, R_NUMBER, &val); 
}



// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_MO_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_MO_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // start display
  if ((ret=drv_MO_start (section))!=0)
    return ret;
  
  // register text widget
  wc=Widget_Text;
  wc.render=NULL;
  widget_register(&wc);
  
  // register plugins
  AddFunction ("contrast",  1, plugin_contrast);
  AddFunction ("backlight", 1, plugin_backlight);
  AddFunction ("gpo",       2, plugin_gpo);
  AddFunction ("pwm",       2, plugin_pwm);
  AddFunction ("rpm",       1, plugin_rpm);
  
  return 0;
}


// close driver & display
int drv_MO_quit (void) {
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


DRIVER drv_MatrixOrbital = {
  name: "MatrixOrbital",
  list:  drv_MO_list,
  init:  drv_MO_init,
  quit:  drv_MO_quit, 
};
