/* $Id: M50530.c,v 1.11 2003/08/16 07:31:35 reinelt Exp $
 *
 * driver for display modules based on the M50530 chip
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: M50530.c,v $
 * Revision 1.11  2003/08/16 07:31:35  reinelt
 * double buffering in all drivers
 *
 * Revision 1.10  2003/08/15 07:54:07  reinelt
 * HD44780 4 bit mode implemented
 *
 * Revision 1.9  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.8  2003/04/07 06:02:59  reinelt
 * further parallel port abstraction
 *
 * Revision 1.7  2003/04/04 06:01:59  reinelt
 * new parallel port abstraction scheme
 *
 * Revision 1.6  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.5  2002/08/19 10:51:06  reinelt
 * M50530 driver using new generic bar functions
 *
 * Revision 1.4  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.3  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.2  2002/04/30 07:20:15  reinelt
 *
 * implemented the new ndelay(nanoseconds) in all parallel port drivers
 *
 * Revision 1.1  2001/09/11 05:31:37  reinelt
 * M50530 driver
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD M50530[]
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"
#include "parport.h"
#include "udelay.h"

#define XRES 5
#define YRES 8
#define CHARS 8

static LCD Lcd;
static int  GPO=0;

static char *FrameBuffer1=NULL;
static char *FrameBuffer2=NULL;


static unsigned char SIGNAL_EX;
static unsigned char SIGNAL_IOC1;
static unsigned char SIGNAL_IOC2;
static unsigned char SIGNAL_GPO;

static void M5_command (unsigned int cmd, int delay)
{
    
  // put data on DB1..DB8
  parport_data (cmd&0xff);
    
  // set I/OC1 (Select inverted)
  // set I/OC2 (AutoFeed inverted)
  parport_control (SIGNAL_IOC1|SIGNAL_IOC2, 
		   (cmd&0x200?SIGNAL_IOC1:0) | 
		   (cmd&0x100?SIGNAL_IOC2:0));

  // Control data setup time
  ndelay(200);

  // send command
  // EX signal pulse width = 500ns
  // Fixme: why 500 ns? Datasheet says 200ns
  parport_toggle (SIGNAL_EX, 1, 500);

  // wait
  udelay(delay);

}


static void M5_write (unsigned char *string, int len)
{
  unsigned int cmd;

  while (len--) {
    cmd=*string++;
    M5_command (0x100|cmd, 20);
  }
}


static void M5_setGPO (int bits)
{
  if (Lcd.gpos>0) {

    // put data on DB1..DB8
    parport_data (bits);

    // 74HCT573 set-up time
    ndelay(20);
    
    // send data
    // 74HCT573 enable pulse width = 24ns
    parport_toggle (SIGNAL_GPO, 1, 24);
  }
}


static void M5_define_char (int ascii, char *buffer)
{
  M5_command (0x300+192+8*ascii, 20);
  M5_write (buffer, 8);
}


int M5_clear (int full)
{

  memset (FrameBuffer1, ' ', Lcd.rows*Lcd.cols*sizeof(char));
  bar_clear();
  GPO=0;

  if (full) {
    memset (FrameBuffer2, ' ', Lcd.rows*Lcd.cols*sizeof(char));
    M5_command (0x0001, 1250); // clear display
    M5_setGPO (GPO);           // All GPO's off
  }
  
  return 0;
}


int M5_init (LCD *Self)
{
  int rows=-1, cols=-1, gpos=-1;
  char *s, *e;
  
  s=cfg_get("Size",NULL);
  if (s==NULL || *s=='\0') {
    error ("M50530: no 'Size' entry in %s", cfg_file());
    return -1;
  }
  if (sscanf(s,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("M50530: bad size '%s'",s);
    return -1;
  }

  s=cfg_get ("GPOs",NULL);
  if (s==NULL) {
    gpos=0;
  } else {
    gpos=strtol(s, &e, 0);
    if (*e!='\0' || gpos<0 || gpos>8) {
      error ("M50530: bad GPOs '%s' in %s", s, cfg_file());
      return -1;
    }    
  }
  
  Self->rows=rows;
  Self->cols=cols;
  Self->gpos=gpos;
  Lcd=*Self;
  
  // Init the framebuffers
  FrameBuffer1 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  FrameBuffer2 = (char*)malloc(Lcd.cols*Lcd.rows*sizeof(char));
  if (FrameBuffer1==NULL || FrameBuffer2==NULL) {
    error ("HD44780: framebuffer could not be allocated: malloc() failed");
    return -1;
  }

  if ((SIGNAL_EX   = parport_wire_ctrl ("EX",   "STROBE"))==0xff) return -1;
  if ((SIGNAL_IOC1 = parport_wire_ctrl ("IOC1", "SELECT"))==0xff) return -1;
  if ((SIGNAL_IOC2 = parport_wire_ctrl ("IOC2", "AUTOFD"))==0xff) return -1;
  if ((SIGNAL_GPO  = parport_wire_ctrl ("GPO",  "INIT")  )==0xff) return -1;
  
  if (parport_open() != 0) {
    error ("M50530: could not initialize parallel port!");
    return -1;
  }
  
  // set direction: write
  parport_direction (0);

  // initialize display
  M5_command (0x00FA, 20); // set function mode
  M5_command (0x0020, 20); // set display mode
  M5_command (0x0050, 20); // set entry mode
  M5_command (0x0030, 20); // set display mode
  
  bar_init(rows, cols, XRES, YRES, CHARS);
  bar_add_segment(0,0,255,32); // ASCII 32 = blank
  
  M5_clear(1);
  
  return 0;
}


void M5_goto (int row, int col)
{
  int pos;

  pos=row*48+col;
  if (row>3) pos-=168;
  M5_command (0x300|pos, 20);
}


int M5_put (int row, int col, char *text)
{
  char *p=FrameBuffer1+row*Lcd.cols+col;
  char *t=text;
  
  while (*t && col++<=Lcd.cols) {
    *p++=*t++;
  }
  return 0;
}


int M5_bar (int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}


int M5_gpo (int num, int val)
{
  if (num>=Lcd.gpos) 
    return -1;

  if (val) {
    GPO |= 1<<num;     // set bit
  } else {
    GPO &= ~(1<<num);  // clear bit
  }
  return 0;
}


int M5_flush (void)
{
  int row, col, pos1, pos2;
  int c, equal;
  
  bar_process(M5_define_char);

  for (row=0; row<Lcd.rows; row++) {
    for (col=0; col<Lcd.cols; col++) {
      c=bar_peek(row, col);
      if (c!=-1) {
	if (c!=32) c+=248; //blank
	FrameBuffer1[row*Lcd.cols+col]=(char)c;
      }
    }
    for (col=0; col<Lcd.cols; col++) {
      if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) continue;
      M5_goto (row, col);
      for (pos1=col++, pos2=pos1, equal=0; col<Lcd.cols; col++) {
	if (FrameBuffer1[row*Lcd.cols+col]==FrameBuffer2[row*Lcd.cols+col]) {
	  // If we find just one equal byte, we don't break, because this 
	  // would require a goto, which takes one byte, too.
	  if (++equal>2) break;
	} else {
	  pos2=col;
	  equal=0;
	}
      }
      M5_write (FrameBuffer1+row*Lcd.cols+pos1, pos2-pos1+1);
    }
  }
  
  memcpy (FrameBuffer2, FrameBuffer1, Lcd.rows*Lcd.cols*sizeof(char));

  M5_setGPO(GPO);

  return 0;
}


int M5_quit (void)
{
  info("M50530: shutting down.");

  if (FrameBuffer1) {
    free(FrameBuffer1);
    FrameBuffer1=NULL;
  }

  if (FrameBuffer2) {
    free(FrameBuffer2);
    FrameBuffer2=NULL;
  }

  return parport_close();
}


LCD M50530[] = {
  { name: "M50530",
    rows:  0,
    cols:  0,
    xres:  XRES,
    yres:  YRES,
    bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
    gpos:  0,
    init:  M5_init,
    clear: M5_clear,
    put:   M5_put,
    bar:   M5_bar,
    gpo:   M5_gpo,
    flush: M5_flush,
    quit:  M5_quit 
  },
  { NULL }
};
