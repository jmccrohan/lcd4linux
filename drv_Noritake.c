/* $Id: drv_Noritake.c,v 1.3 2005/05/05 08:36:12 reinelt Exp $
 * 
 * Driver for a Noritake GU128x32-311 graphical display.
 * 
 * Copyright (C) 2005 Julien Aube <ob@obconseil.net>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_Noritake.c,v $
 * Revision 1.3  2005/05/05 08:36:12  reinelt
 * changed SELECT to SLCTIN
 *
 * Revision 1.2  2005/05/04 07:18:44  obconseil
 * Driver modified according to Michels's recommendations :
 *
 * - Suppressed linux/parport.h depandancy. It was not needed anyway.
 * - Compile-time disable the wait_busy polling function, replaced with a time wait.
 * - Replaced the hardwire_* calls by their wire_* equivalent, to adapt other wirings.
 * - Created a "Models" structure, containing parameters for the display.
 * - Other cleanups, to remove compile-time warnings.
 *
 * Revision 1.1  2005/05/04 05:42:38  reinelt
 * Noritake driver added
 *
 */

/*
 * *** Noritake Itron GU128x32-311 ***
 * A 128x32 VFD (Vacuum Fluorescent Display).
 * It is driver by a Hitachi microcontroller, with a specific 
 * firmware. 
 * The datasheet can be easily found on the internet by searching for the 
 * the name of the display, it's a PDF file that describe the timing, and 
 * the protocol to communicate with the Hitachi microcontroller.
 * 
 * The display support 2 modes (that can be mutiplexed), one text mode
 * thanks to an integrated character generator, and provide 4 lines of 
 * 21 caracters.
 * There is also a graphical mode that can be used to switch on or off
 * each one if the 128x32 pixels. (monochrome).
 * 
 * The protocol include the possibility to clear the display memory quickly,
 * change the luminosity, swich the display on or off (without affecting the 
 * content of the memory) and finally change the "page" or the caracter 
 * generator. Two pages are available in the ROM, all the characters are 
 * listed in the documentation.
 *
 * This driver support only the character mode at the moment.
 * A future release will support the graphical mode as an option.
 * 
 * This driver is released under the GPL.
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Noritake
 *
 */

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "udelay.h"
#include "drv_generic_text.h"
#include "drv_generic_parport.h"


static char Name[]="Noritake";

typedef struct {
  int type;
  char *name;
  int rows;
  int cols;
  int xres;
  int yrex;
  int goto_cost;
  int protocol;
} MODEL;

static int Model,Protocol;
static MODEL Models[] = {
  { 0x01, "GU311",            4, 21,  6,  8, 5, 1 },
  { 0x02, "GU311_Graphic",    4, 21,  6,  8, 6, 1 },
  { 0xff, "Unknown", -1, -1, -1, -1, -1, -1 }
};

static unsigned char SIGNAL_CS;    /* Chip select, OUTPUT, negative logic, pport AUTOFEED */
static unsigned char SIGNAL_WR;    /* Write        OUTPUT, negative logic, pport STOBE */
static unsigned char SIGNAL_RESET; /* Reset,       OUTPUT, negative logic, pport INIT */
static unsigned char SIGNAL_BLANK; /* Blank,       OUTPUT , negative logic, pport SELECT-IN */
/* static unsigned char SIGNAL_BUSY;*/  /* Busy,        INPUT , positive logic, pport BUSY, not used */
/* static unsigned char SIGNAL_FRP;*/   /* Frame Pulse, INPUT , positive logic, pport ACK, not used */ 
void (*drv_Noritake_clear) (void) ;

/* Data port is positive logic */


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/* Low-level parport driving functions */

/* This function was used to pool the BUSY line on the parallel port, which 
can be linked to the BUSY line on the display. But since it works 
with a timed wait, this function is not necessary, and is kept just in case.*/
#if 0
static void drv_GU311_wait_busy(void)
{
    char c;
    
    c = drv_generic_parport_status();
    while ( (c & SIGNAL_BUSY) == 0 ) 
    {
       ndelay(200); /* Wait 100ns before next consultation of BUSY line 
                    if the first one was not successful */
       c = drv_generic_parport_status();
    }
}
#endif 


static void drv_GU311_send_char(char c)
{
   //drv_GU311_wait_busy(); /* ensuite the display is ready to take the command */
                            /* Disabled because all the cables does not have the busy line linked. */
   drv_generic_parport_data(c);
   ndelay(30); /* delay to ensure data line stabilisation on long cables */
   drv_generic_parport_control(SIGNAL_WR,0); /* write line to enable */
   ndelay(150); /* data hold time */
   drv_generic_parport_control(SIGNAL_WR,0xff);  /* write line to disable */
   ndelay(75); /* From spec : minimum time before next command */
}

static void drv_GU311_send_string(char * str, int size)
{
  int i;
  for (i=0;i<size;i++)
       drv_GU311_send_char(str[i]);
  
}

/* Command-string elaboration functions */
static void drv_GU311_make_text_string(const int row, const int col, const char *data, int len)
{
   static char cmd[96] = { 0x01,'C',0,0,'S',0 };
   unsigned char start_addr;
   
   /* Cols are 0x00..0x15, on 4 lines. */
   start_addr = ( 0x16 * row ) + col; 
   if ( start_addr > 0x57 ) return;
   if ( len > 0x57 ) return ;
   
   cmd[2] = start_addr;
   cmd[3] = len;
   
   memcpy(cmd+5,data,len);
   
   drv_GU311_send_string(cmd,len+5);
   
}

/* API functions */

static void drv_GU311_clear (void)
{
  static char clear_cmd[] = { 0x01, 'O', 'P' };
  drv_GU311_send_string(  clear_cmd, sizeof(clear_cmd) );
  ndelay(500); /* Delay for execution - this command is the longuest */
}


static void drv_GU311_write (const int row, const int col, const char *data, int len)
{
    drv_GU311_make_text_string(row,col,data, len);
}


static void drv_GU311_reset (void)
{
  drv_generic_parport_control(SIGNAL_RESET,0); /* initiate reset */
  ndelay(1000); /* reset hold time 1ms */
  drv_generic_parport_control(SIGNAL_RESET,0xff);
  ndelay(200000); /* reset ready time 200ms */

}


static int drv_GU311_start(const char *section)
{
  char   cmd[3] = { 0x01, 'O' };
  
  /* Parallel port opening and association */
  if (drv_generic_parport_open(section, Name) < 0) return -1;
  if ((SIGNAL_CS=drv_generic_parport_wire_ctrl    ("CS", "AUTOFD"))==0xff) return -1;
  if ((SIGNAL_WR=drv_generic_parport_wire_ctrl    ("WR", "STROBE"))==0xff) return -1;
  if ((SIGNAL_RESET=drv_generic_parport_wire_ctrl ("RESET", "INIT"))==0xff) return -1;
  if ((SIGNAL_BLANK=drv_generic_parport_wire_ctrl ("BLANK", "SLCTIN")  )==0xff) return -1;
  /* SIGNAL_BUSY=PARPORT_STATUS_BUSY; */ /* Not currently needed */ 
  /* SIGNAL_FRP=PARPORT_STATUS_ACK;   */ /* Not currently needed */
  
  /* Signals configuration */
  drv_generic_parport_direction(0); /* parallel port in output mode */
  drv_generic_parport_control(SIGNAL_CS|SIGNAL_WR|SIGNAL_RESET|SIGNAL_BLANK, 0xff);
                             /* All lines to "deactivate", -> 1 level on the wire */
  drv_generic_parport_control(SIGNAL_CS,0); /* CS to 0 all the time, write done by WR */
  drv_GU311_reset();
  
  /* Ready for commands from now on. */
  
  /* Display configuration */
  cmd[2] = '0' ; drv_GU311_send_string(cmd, sizeof(cmd) ); /* Select char. page 0 */
  cmd[2] = 'Q' ; drv_GU311_send_string(cmd, sizeof(cmd) ); /* Select 'Quick Mode' */ 
  cmd[2] = 'a' ; drv_GU311_send_string(cmd, sizeof(cmd) ); /* Brightness at 100% */
  cmd[2] = 'T' ; drv_GU311_send_string(cmd, sizeof(cmd) ); /* Ensure display ON */
  
  drv_Noritake_clear();
  return 0;
}



static int drv_Noritake_start (const char *section)
{
   char * model=0;
   int i;
   model=cfg_get(section, "Model", NULL);
   if (model!=NULL && *model!='\0') {
      for (i=0; Models[i].type!=0xff; i++) {
         if (strcasecmp(Models[i].name, model)==0) break;
      }
      if (Models[i].type==0xff) {
         error ("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
         return -1;
       }
       Model=i;
       info ("%s: using model '%s'", Name, Models[Model].name);
   } else {
       error ("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
       return -1;
  }
  
  DROWS     = Models[Model].rows;
  DCOLS     = Models[Model].cols;
  XRES      = Models[Model].xres;
  YRES      = Models[Model].xres;
  GOTO_COST = Models[Model].goto_cost;
  Protocol  = Models[Model].protocol;
    /* display preferences */
  CHARS = 0;      /* number of user-defineable characters */
  CHAR0 = 0;      /* ASCII of first user-defineable char */


 
  /* real worker functions */
  drv_Noritake_clear = drv_GU311_clear;
  if ( Models[Model].type == 0x01 ) {
     drv_generic_text_real_write = drv_GU311_write;    
  } else {
     error("%s: Unsupported display. Currently supported are : GU311.",Name);
     return -1;
  }
  return drv_GU311_start(section);
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_Noritake_list (void)
{
  printf ("GU311 GU311_Graphic");
  return 0;
}


/* initialize driver & display */
int drv_Noritake_init (const char *section, const int quiet)
{
  WIDGET_CLASS wc;
  int ret;  
  
  /* start display */
  if ((ret=drv_Noritake_start (section))!=0)
    return ret;
    
  /* initialize generic text driver */
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  /* register text widget */
  wc=Widget_Text;
  wc.draw=drv_generic_text_draw;
  widget_register(&wc);
  
  /* register plugins */
  /* none */

  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer, NULL)) {
      sleep (3);
      drv_Noritake_clear();
    }
  }

  return 0;
}


/* close driver & display */
int drv_Noritake_quit (const int quiet)
{

  info("%s: shutting down.", Name);
  
  /* clear display */
  drv_Noritake_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }
  
  drv_generic_parport_close();
  drv_generic_text_quit();
  return (0);
}


DRIVER drv_Noritake = {
  name: Name,
  list: drv_Noritake_list,
  init: drv_Noritake_init,
  quit: drv_Noritake_quit, 
};

