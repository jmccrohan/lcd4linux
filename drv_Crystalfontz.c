/* $Id: drv_Crystalfontz.c,v 1.9 2004/02/04 19:10:51 reinelt Exp $
 *
 * new style driver for Crystalfontz display modules
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
 * $Log: drv_Crystalfontz.c,v $
 * Revision 1.9  2004/02/04 19:10:51  reinelt
 * Crystalfontz driver nearly finished
 *
 * Revision 1.8  2004/02/01 08:05:12  reinelt
 * Crystalfontz 633 extensions (CRC checking and stuff)
 * Models table for HD44780
 * Noritake VFD BVrightness patch from Bill Paxton
 *
 * Revision 1.7  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.6  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.5  2004/01/25 05:30:09  reinelt
 * plugin_netdev for parsing /proc/net/dev added
 *
 * Revision 1.4  2004/01/23 07:04:03  reinelt
 * icons finished!
 *
 * Revision 1.3  2004/01/23 04:53:34  reinelt
 * icon widget added (not finished yet!)
 *
 * Revision 1.2  2004/01/22 07:57:45  reinelt
 * several bugs fixed where segfaulting on layout>display
 * Crystalfontz driver optimized, 632 display already works
 *
 * Revision 1.1  2004/01/21 12:36:19  reinelt
 * Crystalfontz NextGeneration driver added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Crystalfontz
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "cfg.h"
#include "timer.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_serial.h"


static char Name[]="Crystalfontz";

static int Model;
static int Protocol;
static int Row, Col;

// ring buffer for bytes received from the display
static unsigned char RingBuffer[256];
static int  RingRPos = 0;
static int  RingWPos = 0;

// packet from the display
struct {
  unsigned char type;
  unsigned char size;
  unsigned char data[16+1]; // trailing '\0'
} Packet;

// Line Buffer for 633 displays
static char Line[2*16];

// Fixme:
// static int GPO[8];
static int GPOS;


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
  { 626, "626",      2, 16, 0, 1 },
  { 631, "631",      2, 20, 0, 3 },
  { 632, "632",      2, 16, 0, 1 },
  { 633, "633",      2, 16, 0, 2 },
  { 634, "634",      4, 20, 0, 1 },
  { 636, "636",      2, 16, 0, 1 },
  {  -1, "Unknown", -1, -1, 0, 0 }
};


// ****************************************
// ***  hardware dependant functions    ***
// ****************************************

// x^0 + x^5 + x^12
#define CRCPOLY 0x8408 

static unsigned short CRC (unsigned char *p, size_t len, unsigned short seed)
{
  int i;
  while (len--) {
    seed ^= *p++;
    for (i = 0; i < 8; i++)
      seed = (seed >> 1) ^ ((seed & 1) ? CRCPOLY : 0);
  }
  return ~seed;
}

static unsigned char LSB (unsigned short word)
{
  return word & 0xff;
}

static unsigned char MSB (unsigned short word)
{
  return word >> 8;
}


static unsigned char byte (int pos)
{
  pos+=RingRPos;
  if (pos>=sizeof(RingBuffer))
    pos-=sizeof(RingBuffer);
  return RingBuffer[pos];
}


static void drv_CF_process_packet (void)
{
  // debugging only
  if (0) debug ("Packet: type=%d (%d) len=%d data=<%s>", 
	 Packet.type, Packet.type & 0x3f, Packet.size, Packet.data);
}


static int drv_CF_poll (void)
{
  char buffer[32];
  unsigned short crc;
  int n, num, size;
  
  // read into RingBuffer
  while (1) {
    num=drv_generic_serial_poll(buffer, 32);
    if (num<=0) break;
    // put result into RingBuffer
    for (n=0; n<num; n++) {
      RingBuffer[RingWPos++]=buffer[n];
      if (RingWPos>=sizeof(RingBuffer)) RingWPos=0;
    }
  }
  
  // process RingBuffer
  while (1) {
    // packet size
    num=RingWPos-RingRPos;
    if (num < 0) num+=10;
    // minimum packet size=4
    if (num < 4) return 0;
    // valid response types: 01xxxxx 10.. 11..
    // therefore: 00xxxxxx is invalid
    if (byte(0)>>6 == 0) goto GARBAGE;
    // valid command length is 0 to 16
    if (byte(1) > 16) goto GARBAGE;
    // all bytes available?
    size=byte(1);
    if (num < size+4) return 0;
    // check CRC
    for (n=0; n<size+4; n++) buffer[n]=byte(n);
    crc = CRC(buffer, size+2, 0xffff);
    if (LSB(crc) != byte(size+2)) goto GARBAGE;
    if (MSB(crc) != byte(size+3)) goto GARBAGE;
    // process packet
    Packet.type = buffer[0];
    Packet.size = size;
    memcpy(Packet.data, buffer+2, size);
    Packet.data[size]='\0'; // trailing zero
    // increment read pointer
    RingRPos += size+4;
    if (RingRPos >= sizeof(RingBuffer)) RingRPos -= sizeof(RingBuffer);
    // a packet arrived
    return 1;
  GARBAGE:
    debug ("dropping garbage byte %d", byte(0));
    RingRPos++;
    if (RingRPos>=sizeof(RingBuffer)) RingRPos=0;
    continue;
  }
  
  // not reached
  return 0;
}


static void drv_CF_timer (void *notused)
{
  while (drv_CF_poll()) {
    drv_CF_process_packet();
  }
}


static void drv_CF_send (int cmd, int len, char *data)
{
  unsigned char buffer[16+2];
  unsigned short crc;
  
  if (len>sizeof(buffer)-2) {
    error ("%s: internal error: packet length %d exceeds buffer size %d", Name, len, sizeof(buffer)-2);
    len=sizeof(buffer)-1;
  }
  
  buffer[0]=cmd;
  buffer[1]=len;
  memcpy (buffer+2, data, len);
  crc = CRC(buffer, len+2, 0xffff);
  buffer[len+2] = LSB(crc);
  buffer[len+3] = MSB(crc);
  
  if (0) debug ("Tx Packet %d len=%d", buffer[0], buffer[1]);
  
  drv_generic_serial_write (buffer, len+4);
  
}


static void drv_CF_write1 (char *string, int len)
{
  drv_generic_serial_write (string, len);
}


static void drv_CF_write2 (char *string, int len)
{
  // limit length
  if (Col+len>16) len=16-Col;
  
  // sanity check
  if (Row>=2 || Col+len>16) {
    error ("%s: internal error: write outside linebuffer bounds!", Name);
    return;
  }
  
  memcpy (Line+16*Row+Col, string, len);
  drv_CF_send (7+Row, 16, Line+16*Row);
}


static void drv_CF_write3 (char *string, int len)
{
  debug ("write3(<%.*s>,%d)", len, string, len);
}


static void drv_CF_goto1 (int row, int col)
{
  char cmd[3]="\021xy"; // set cursor position
  
  if (row==0 && col==0) {
    drv_CF_write1("\001", 1); // cursor home
  } else {
    cmd[1]=(char)col;
    cmd[2]=(char)row;
    drv_CF_write1(cmd, 3);
  }
}

static void drv_CF_goto23 (int row, int col)
{
  // as the 633 does not have random access to the display content,
  // and the 631 needs coordinates with random access, 
  // we just store the needed cursor position
  Row=row;
  Col=col;
}


static void drv_CF_defchar1 (int ascii, char *buffer)
{
  char cmd[2]="\031n"; // set custom char bitmap
  
  // user-defineable chars start at 128, but are defined at 0
  cmd[1]=(char)(ascii-CHAR0); 
  drv_CF_write1 (cmd, 2);
  drv_CF_write1 (buffer, 8);
}


static void drv_CF_defchar23 (int ascii, char *matrix)
{
  char buffer[9];
  
  // user-defineable chars start at 128, but are defined at 0
  buffer[0] = (char)(ascii-CHAR0); 
  memcpy(buffer+1, matrix, 8);
  drv_CF_send (9, 9, buffer);
}


static int drv_CF_contrast (int contrast)
{
  char buffer[2];

  if (contrast<0  ) contrast=0;
  if (contrast>100) contrast=100;

  switch (Protocol) {

  case 1:
    buffer[0] = 15; // Set LCD Contrast
    buffer[1] = contrast;
    drv_CF_write1 (buffer, 2);
    break;

  case 2:
  case 3:
    // contrast goes from 0 to 50 only
    if (contrast>50) contrast=50;
    buffer[0] = contrast;
    drv_CF_send (13, 1, buffer);
    break;
  }

  return contrast;
}


static int drv_CF_backlight (int backlight)
{
  char buffer[3];

  if (backlight<0  ) backlight=0;
  if (backlight>100) backlight=100;
  
  switch (Protocol) {
    
  case 1:
    buffer[0] = 14; // Set LCD backlight
    buffer[1] = backlight;
    drv_CF_write1 (buffer, 2);
    break;

  case 2:
  case 3:
    buffer[0] = backlight;
    drv_CF_send (14, 1, buffer);
    break;
  }
  
  return backlight;
}


static int drv_CF_autodetect (void)
{
  int i, m;
  
  // only autodetect newer displays
  if (Protocol<2) return -1;
    
  // read display type
  drv_CF_send (1, 0, NULL);
  
  i=0;
  while (1) {
    // wait 10 msec
    usleep(10*1000);
    // packet available?
    if (drv_CF_poll()) {
      // display type
      if (Packet.type==0x41) {
	char t[7]; float h, v;
	info ("%s: display identifies itself as '%s'", Name, Packet.data); 
	if (sscanf(Packet.data, "%6s:h%f,v%f", t, &h, &v)!=3) {
	  error ("%s: error parsing display identification string", Name);
	  return -1;
	}
	info ("%s: display type '%s', hardware version %3.1f, firmware version %3.1f", Name, t, h, v);
	if (strncmp(t, "CFA", 3)==0) {
	  for (m=0; Models[m].type!=-1; m++) {
	    // omit the 'CFA'
	    if (strcasecmp(Models[m].name, t+3)==0)
	      return m;
	  }
	}
	error ("%s: display type '%s' may be not supported!", Name, t);
	return -1;
      }
      drv_CF_process_packet();
    }
    // wait no longer than 300 msec
    if (++i > 30) {
      error ("%s: display detection timed out", Name);
      return -1;
    }
  }
  
  // not reached
  return -1;
}
  

static int drv_CF_start (char *section)
{
  int i;  
  char *model;
  char buffer;
  
  model=cfg_get(section, "Model", NULL);
  if (model!=NULL && *model!='\0') {
    for (i=0; Models[i].type!=-1; i++) {
      if (strcasecmp(Models[i].name, model)==0) break;
    }
    if (Models[i].type==-1) {
      error ("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
      return -1;
    }
    Model    = i;
    Protocol = Models[Model].protocol;
    info ("%s: using model '%s'", Name, Models[Model].name);
  } else {
    Model    = -1;
    Protocol =  2; //auto-detect only newer displays
    info ("%s: no '%s.Model' entry from %s, auto-detecting", Name, section, cfg_source());
  }
  
  // open serial port
  if (drv_generic_serial_open(section, Name)<0) return -1;
  
  // Fixme: why such a large delay?
  usleep(350*1000);
  
  // display autodetection
  i=drv_CF_autodetect();
  if (Model==-1) Model=i;
  if (Model==-1) {
    error ("%s: auto-detection failed, please specify a '%s.Model' entry in %s", Name, section, cfg_source());
    return -1;
  }
  if (i!=-1 && Model!=i) {
    error ("%s: %s.Model '%s' from %s does not match detected model '%s'", 
	   Name, section, model, cfg_source(), Models[i].name);
    return -1;
  }
  
  // initialize global variables
  DROWS    = Models[Model].rows;
  DCOLS    = Models[Model].cols;
  GPOS     = Models[Model].gpos;
  Protocol = Models[Model].protocol;

  // regularly process display answers
  // Fixme: make 20msec configurable
  timer_add (drv_CF_timer, NULL, 100, 0);

  switch (Protocol) {
  case 1:
    drv_CF_write1 ("\014", 1);  // Form Feed (Clear Display)
    drv_CF_write1 ("\004", 1);  // hide cursor
    drv_CF_write1 ("\024", 1);  // scroll off
    drv_CF_write1 ("\030", 1);  // wrap off
    break;
  case 2:
  case 3:
    drv_CF_send ( 6, 0, NULL);    // Clear Display
    buffer=0;
    drv_CF_send (12, 1, &buffer); // Set LCD Cursor Style
    break;
  }
  
  // clear 633 linebuffer
  memset (Line, ' ', sizeof(Line));
  
  // set contrast
  if (cfg_number(section, "Contrast", 0, 0, 100, &i)==0) {
    drv_CF_contrast(i);
  }

  // set backlight
  if (cfg_number(section, "Backlight", 0, 0, 100, &i)==0) {
    drv_CF_backlight(i);
  }

  return 0;
}


// ****************************************
// ***            plugins               ***
// ****************************************


static void plugin_contrast (RESULT *result, RESULT *arg1)
{
  double contrast;
  
  contrast=drv_CF_contrast(R2N(arg1));
  SetResult(&result, R_NUMBER, &contrast); 
}


static void plugin_backlight (RESULT *result, RESULT *arg1)
{
  double backlight;
  
  backlight=drv_CF_backlight(R2N(arg1));
  SetResult(&result, R_NUMBER, &backlight); 
}

// Fixme: other plugins for Fans, Temmperature sensors, ...



// ****************************************
// ***        widget callbacks          ***
// ****************************************

// using drv_generic_text_draw(W)
// using drv_generic_text_icon_draw(W)
// using drv_generic_text_bar_draw(W)


// ****************************************
// ***        exported functions        ***
// ****************************************


// list models
int drv_CF_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=-1; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}


// initialize driver & display
int drv_CF_init (char *section)
{
  WIDGET_CLASS wc;
  int ret;  
  
  // start display
  if ((ret=drv_CF_start (section))!=0)
    return ret;
  
  // display preferences
  XRES  = 6;      // pixel width of one char 
  YRES  = 8;      // pixel height of one char 
  CHARS = 8;      // number of user-defineable characters
  GOTO_COST = 3;  // number of bytes a goto command requires

  // real worker functions
  switch (Protocol) {
  case 1:
    CHAR0 = 128; // ASCII of first user-defineable char
    drv_generic_text_real_goto    = drv_CF_goto1;
    drv_generic_text_real_write   = drv_CF_write1;
    drv_generic_text_real_defchar = drv_CF_defchar1;
    break;
  case 2:
    CHAR0 = 0; // ASCII of first user-defineable char
    drv_generic_text_real_goto    = drv_CF_goto23;
    drv_generic_text_real_write   = drv_CF_write2;
    drv_generic_text_real_defchar = drv_CF_defchar23;
    break;
  case 3:
    CHAR0 = 0; // ASCII of first user-defineable char
    drv_generic_text_real_goto    = drv_CF_goto23;
    drv_generic_text_real_write   = drv_CF_write2;
    drv_generic_text_real_defchar = drv_CF_defchar23;
    break;
  }
  
  // initialize generic text driver
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  // initialize generic icon driver
  if ((ret=drv_generic_text_icon_init())!=0)
    return ret;
  
  // initialize generic bar driver
  if ((ret=drv_generic_text_bar_init())!=0)
    return ret;
  
  // add fixed chars to the bar driver
  drv_generic_text_bar_add_segment (0, 0, 255, 32); // ASCII 32 = blank
  if (Protocol==2)
    drv_generic_text_bar_add_segment (255,255,255,255); // ASCII 255 = block
  
  // register text widget
  wc=Widget_Text;
  wc.draw=drv_generic_text_draw;
  widget_register(&wc);
  
  // register icon widget
  wc=Widget_Icon;
  wc.draw=drv_generic_text_icon_draw;
  widget_register(&wc);
  
  // register bar widget
  wc=Widget_Bar;
  wc.draw=drv_generic_text_bar_draw;
  widget_register(&wc);
  
  // register plugins
  AddFunction ("contrast",  1, plugin_contrast);
  AddFunction ("backlight", 1, plugin_backlight);
  
  return 0;
}


// close driver & display
int drv_CF_quit (void) {

  info("%s: shutting down.", Name);
  drv_generic_serial_close();
  drv_generic_text_quit();
  
  return (0);
}


DRIVER drv_Crystalfontz = {
  name: Name,
  list: drv_CF_list,
  init: drv_CF_init,
  quit: drv_CF_quit, 
};

