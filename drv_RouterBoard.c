/* $Id: drv_RouterBoard.c,v 1.3 2004/08/30 12:48:52 rjoco77 Exp $
 *
 * driver for the "Router Board LCD port" 
 * see port details at http://www.routerboard.com
 *
 * Copyright (C) 2004  Roman Jozsef <rjoco77@freemail.hu> 
 *
 * based on the HD44780 parallel port driver and RB SDK example 
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
 * $Log: drv_RouterBoard.c,v $
 * Revision 1.3  2004/08/30 12:48:52  rjoco77
 *  * Added backlight update immediatelly
 *
 * Revision 1.2  2004/08/29 20:07:55  reinelt
 *
 * Patch from Joco: Make RouterBoard Backlight configurable
 *
 */


/* This particulary board not have paralell port but have a special LCD header
 * where can connect an HD44780 display.
 * This port called IOSC0 port, and is write only, this port control the
 * 4 leds on board too.
 * Because its a write only port you can't control leds outside lcd driver
 * or inverse, for this added the socket controlled leds. To send led status
 * simply open an UDP socket and send to localhost 127.0.0.1 port 3333 one
 * byte or more anyway only the first byte 4 low bits used the others is
 * cleared and ignored bit0 = Led1 ,bit1 = Led2 ...
 * This socket polled via timer callback, for detail see at drv_RB_start()
 * I add at to end of this file an example!
 * If you don't want coment #define RB_WITH LEDS and this part will be ignored
 *
 * The connection details:
 *    The IOCS0 port lower 16 bits connected as follows:
 *     bit   LCD	LEDS
 *     0..7  data
 *      8    INITX
 *      9    SLINX
 *      10   AFDX
 *      11   backlit
 *      12   		LED1
 *      13   		LED2
 *      14   		LED3
 *      15   		LED4
 *    
 * LCD male header:
 * 1	Vcc +5V
 * 2	GND
 * 3	RS (Register Select,AFDX)
 * 4	Contrast adjust (controlled) but how? if you know tell me not mentioned on User Manual
 * 5	E (enable signal, INITX)
 * 6	R/W (Data read/write or SLINX) not used connect LCD pin to ground
 * 7	Data 1
 * 8	Data 0
 * 9	Data 3
 * 10	Data 2
 * 11	Data 5
 * 12	Data 4
 * 13	Data 7
 * 14	Data 6
 * 15	Backlit GND (controlled) (IOSC0 bit 11)
 * 16   Backlit Vcc +5V
 *
 * If you using this driver and board and you have any fun device connected,
 * program or story :-) ,please share for me. Thanks.
 *    	   
 * Literature
 *    [GEODE] Geode SC1100 Information Appliance On a Chip
 *      (http://www.national.com/ds/SC/SC1100.pdf)	   
 *    [RB User Manual]
 *      (http://www.routerboard.com)
 *
 *
 * Revision 1.2 
 * Added backlight control
 */


/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_RouterBoard
 *
 */





#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/io.h>

#include "debug.h"
#include "cfg.h"
#include "udelay.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"


/* #define RB_WITH_LEDS 1 */

#ifdef RB_WITH_LEDS		   /* Build without socket&led support */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>

#define POLL_TIMEOUT	10	   /* Socket poll timeout */
#define MAXMSG_SIZE	100	   /* Max messagge we can receive */

static int sock_c;	           /* Socket handler */
static struct sockaddr_in *sacl;
char   sock_msg[MAXMSG_SIZE];

#endif


static char Name[]="RouterBoard";

static int Model;
static int Capabilities;

/* RouterBoard control signals */

#define LCD_INITX     0x0100
#define LCD_SLINX     0x0200
#define LCD_AFDX      0x0400
#define LCD_BACKLIGHT 0x0800
#define RB_LEDS	      0xF000

#define CAR 0x0CF8
#define CDR 0x0CFC


/* HD44780 execution timings [microseconds]
 * as these values differ from spec to spec,
 * we use the worst-case values.
 */

#define T_INIT1 4100 /* first init sequence:  4.1 msec */
#define T_INIT2  100 /* second init sequence: 100 usec */
#define T_EXEC    80 /* normal execution time */
#define T_WRCG   120 /* CG RAM Write */
#define T_CLEAR 1680 /* Clear Display */
#define T_AS      60 /* Address setup time */

/* Fixme: GPO's not yet implemented */
static int GPOS;
/* static int GPO=0; */

/* Fixme: This actually ARE the GPO's... */
static unsigned RB_Leds = 0;

static unsigned RB_BackLight = 0;

typedef struct {
  int type;
  char *name;
  int capabilities;
} MODEL;

#define CAP_HD66712    (1<<0)

static MODEL Models[] = {
  { 0x01, "HD44780",  0 },
  { 0x02, "HD66712",  CAP_HD66712 },
  { 0xff, "Unknown",  0 }
};


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

#ifdef RB_WITH_LEDS

static int drv_RB_sock_init() 
{
  
  if ((sacl = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in))) == NULL) {
    return -1;
  }

  memset(sacl, 0, sizeof(struct sockaddr_in));
  sacl->sin_family = AF_INET;
  sacl->sin_port = htons(3333);//Listen Port
  sacl->sin_addr.s_addr = inet_addr("127.0.0.1");//Listen Address
    
  if ((sock_c = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    error ("Socket open failed");
    free(sacl);
    return -1;
  }

  if (bind(sock_c, (struct sockaddr *) sacl, sizeof(struct sockaddr_in)) < 0) {
    error ("Socket bind open failed");
    free(sacl);
    return -1;
  }
  return 0;
}


static void drv_RB_poll_data ( void __attribute__((unused)) *notused) 
{
  int len,size;
  struct pollfd usfd;
  usfd.fd = sock_c;
  usfd.events = POLLIN | POLLPRI;
  while (poll(&usfd, 1, POLL_TIMEOUT) > 0) {
    len = sizeof(struct sockaddr_in);
    if ((size = recvfrom(sock_c, sock_msg, MAXMSG_SIZE, 0, (struct sockaddr *) sacl,(socklen_t*) &len)) < 0);
    else  { 
      RB_Leds = sock_msg[0]&0x0F;
      RB_Leds = RB_Leds << 12;
      /* fprintf(stderr, "Received data %s\n",sock_msg); */
    }
  }    
}

#endif

/* IOCS0 port number can read from PCI Configuration Space Function 0 (F0) */
/* at index 74h as 16 bit value (see [GEODE] 5.3.1 pg.151 and pg.176 Table 5-29 */

static unsigned getIocs0Port (void) 
{
  static unsigned ret = 0;

  /*get IO permission, here you can't use ioperm command */
  iopl(3);

  if (!ret) {
    outl(0x80009074, CAR);
    ret = inw(CDR);
  }
  return ret;
}  


static int drv_RB_backlight ( int backlight)
{
  /* -1 is used to query  the current Backlight */
  if(backlight == -1 ) {
    if (RB_BackLight > 0) return 1;  // because RB_Backlight actually is 0x0800 or 0
    return 0;
  }
  
  if (backlight < 0) backlight = 0;
  if (backlight > 1) backlight = 1;

  RB_BackLight = backlight ? LCD_BACKLIGHT : 0;
  
  /* Set backlight output */
  outw( RB_Leds | RB_BackLight , getIocs0Port());
     
  return backlight;    
}




static void drv_RB_command ( const unsigned char cmd, const int delay)
{

  outw( RB_Leds | LCD_INITX | RB_BackLight | cmd, getIocs0Port());

  ndelay(T_AS);
  
  outw( RB_Leds | RB_BackLight | cmd, getIocs0Port());
  
  // wait for command completion
  udelay(delay);
  
}


static void drv_RB_data ( const char *string, const int len, const int delay)
{
  int l = len;
  unsigned char ch;

  /* sanity check */
  if (len <= 0) return;

  while (l--) {

    ch = *(string++);
      
    outw( RB_Leds | LCD_AFDX | LCD_INITX | RB_BackLight | ch, getIocs0Port());

    ndelay(T_AS);
     
    outw( RB_Leds | LCD_AFDX | RB_BackLight | ch, getIocs0Port());      

    // wait for command completion
    udelay(delay);

  }
}


static void drv_RB_clear (void)
{
  drv_RB_command ( 0x01, T_CLEAR);
}


static void drv_RB_goto (int row, int col)
{
  int pos;
   
  /* 16x1 Displays are organized as 8x2 :-( */
  if (DCOLS==16 && DROWS==1 && col>7) {
    row++;
    col-=8;
  }
  
  if (Capabilities & CAP_HD66712) {
    /* the HD66712 doesn't have a braindamadged RAM layout */
    pos = row*32 + col;
  } else {
    /* 16x4 Displays use a slightly different layout */
    if (DCOLS==16 && DROWS==4) {
      pos = (row%2)*64+(row/2)*16+col;
    } else {  
      pos = (row%2)*64+(row/2)*20+col;
    }
  }
  drv_RB_command ( (0x80|pos), T_EXEC);
}


static void drv_RB_write (const int row, const int col, const char *data, const int len)
{
  drv_RB_goto (row, col);
  drv_RB_data ( data, len, T_EXEC);
}


static void drv_RB_defchar (const int ascii, const unsigned char *matrix)
{
  int i;
  char buffer[8];

  for (i = 0; i < 8; i++) {
    buffer[i] = matrix[i] & 0x1f;
  }
  
  drv_RB_command ( 0x40|8*ascii, T_EXEC);
  drv_RB_data ( buffer, 8, T_WRCG);
}

  
/* Fixme: GPO's */
#if 0
static void drv_RB_setGPO (const int bits)
{}
#endif

static int drv_RB_start (const char *section, const int quiet)
{
  char *model, *strsize;
  int rows=-1, cols=-1, gpos=-1;
  int l;
  
  model=cfg_get(section, "Model", "HD44780");
  if (model!=NULL && *model!='\0') {
    int i;
    for (i=0; Models[i].type!=0xff; i++) {
      if (strcasecmp(Models[i].name, model)==0) break;
    }
    if (Models[i].type==0xff) {
      error ("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
      return -1;
    }
    Model=i;
    Capabilities=Models[Model].capabilities;
    info ("%s: using model '%s'", Name, Models[Model].name);
  } else {
    error ("%s: empty '%s.Model' entry from %s", Name, section, cfg_source());
    return -1;
  }
  free(model);
  
  strsize = cfg_get(section, "Size", NULL);
  if (strsize == NULL || *strsize == '\0') {
    error ("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
    free(strsize);
    return -1;
  }
  if (sscanf(strsize,"%dx%d",&cols,&rows)!=2 || rows<1 || cols<1) {
    error ("%s: bad %s.Size '%s' from %s", Name, section, strsize, cfg_source());
    free(strsize);
    return -1;
  }
  free(strsize);
  
  /*set backlight*/
  if (cfg_number(section, "Backlight", 1, 0, 1, &l) > 0 ) {
    drv_RB_backlight(l);
  }
  
  if (cfg_number(section, "GPOs", 0, 0, 8, &gpos)<0) return -1;
  info ("%s: controlling %d GPO's", Name, gpos);
  
#ifdef RB_WITH_LEDS
  
  if( drv_RB_sock_init() < 0 )
    {
      error ("Sock error");
      return -1;
    } else timer_add (drv_RB_poll_data, NULL, 500, 0);    
  
#endif
  
  DROWS = rows;
  DCOLS = cols;
  GPOS  = gpos;
  
  drv_RB_command (0x30, T_INIT1); /* 8 Bit mode, wait 4.1 ms */
  drv_RB_command (0x30, T_INIT2); /* 8 Bit mode, wait 100 us */
  drv_RB_command (0x38, T_EXEC);  /* 8 Bit mode, 1/16 duty cycle, 5x8 font */
  
  drv_RB_command (0x08, T_EXEC);  /* Display off, cursor off, blink off */
  drv_RB_command (0x0c, T_CLEAR); /* Display on, cursor off, blink off, wait 1.64 ms */
  drv_RB_command (0x06, T_EXEC);  /* curser moves to right, no shift */
  
  if ((Capabilities & CAP_HD66712) && DROWS > 2) {
    drv_RB_command ( 0x3c, T_EXEC); /* set extended register enable bit */
    drv_RB_command ( 0x09, T_EXEC); /* set 4-line mode */
    drv_RB_command ( 0x38, T_EXEC); /* clear extended register enable bit */
  }
  
  drv_RB_clear();
  drv_RB_command (0x03, T_CLEAR); /* return home */
  
  if (!quiet) {
    char buffer[40];
    qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
    if (drv_generic_text_greet (buffer, NULL)) {
      sleep (3);
      drv_RB_clear();
    }
  }
  
  return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/
static void plugin_backlight (RESULT *result , const int argc, RESULT *argv[])
{
  double backlight;
  
  switch (argc) {
  case 0:
    backlight = drv_RB_backlight(-1);
    SetResult(&result, R_NUMBER, &backlight);
    break;
  case 1:
    backlight = drv_RB_backlight(R2N(argv[0]));
    SetResult(&result, R_NUMBER, &backlight);
    break;
  default:
    error ("%s::backlight(): wrong number of parameters", Name);
    SetResult(&result, R_STRING, "");
  }    
}



/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_RB_list (void)
{
  int i;
  
  for (i=0; Models[i].type!=0xff; i++) {
    printf ("%s ", Models[i].name);
  }
  return 0;
}

/* initialize driver & display */
int drv_RB_init (const char *section, const int quiet)
{
  WIDGET_CLASS wc;
  int asc255bug;
  int ret;  
  
  /* display preferences */
  XRES  = 5;      /* pixel width of one char  */
  YRES  = 8;      /* pixel height of one char  */
  CHARS = 8;      /* number of user-defineable characters */
  CHAR0 = 0;      /* ASCII of first user-defineable char */
  GOTO_COST = 2;  /* number of bytes a goto command requires */
  
  /* real worker functions */
  drv_generic_text_real_write   = drv_RB_write;
  drv_generic_text_real_defchar = drv_RB_defchar;


  /* start display */
  if ((ret=drv_RB_start (section, quiet))!=0)
    return ret;
  
  /* initialize generic text driver */
  if ((ret=drv_generic_text_init(section, Name))!=0)
    return ret;

  /* initialize generic icon driver */
  if ((ret=drv_generic_text_icon_init())!=0)
    return ret;
  
  /* initialize generic bar driver */
  if ((ret=drv_generic_text_bar_init(0))!=0)
    return ret;
  
  /* add fixed chars to the bar driver */
  /* most displays have a full block on ascii 255, but some have kind of  */
  /* an 'inverted P'. If you specify 'asc255bug 1 in the config, this */
  /* char will not be used, but rendered by the bar driver */
  cfg_number(section, "asc255bug", 0, 0, 1, &asc255bug);
  drv_generic_text_bar_add_segment (  0,  0,255, 32); /* ASCII  32 = blank */
  if (!asc255bug) 
    drv_generic_text_bar_add_segment (255,255,255,255); /* ASCII 255 = block */
  
  /* register text widget */
  wc=Widget_Text;
  wc.draw=drv_generic_text_draw;
  widget_register(&wc);
  
  /* register icon widget */
  wc=Widget_Icon;
  wc.draw=drv_generic_text_icon_draw;
  widget_register(&wc);
  
  /* register bar widget */
  wc=Widget_Bar;
  wc.draw=drv_generic_text_bar_draw;
  widget_register(&wc);
  
  /* register plugins*/
  AddFunction ("LCD::backlight", -1, plugin_backlight);
  
  return 0;
}


/* close driver & display */
int drv_RB_quit (const int quiet) {

  info("%s: shutting down.", Name);

  drv_generic_text_quit();

  /* clear *both* displays */
  drv_RB_clear();
  
  /* say goodbye... */
  if (!quiet) {
    drv_generic_text_greet ("goodbye!", NULL);
  }

#ifdef RB_WITH_LEDS

  close(sock_c);
  free(sacl);		/*close network socket*/

#endif  
  
  return (0);
}


DRIVER drv_RouterBoard = {
  name: Name,
  list: drv_RB_list,
  init: drv_RB_init,
  quit: drv_RB_quit, 
};



/* 

Simple example to send led status to port 3333
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>

int send_packet (unsigned char leds)
{
    struct sockaddr_in *sas;
    int sock;
    char msg[20];
    msg[0]=leds;
    msg[1]=0;
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	fprintf(stderr, "Socket option failed.\n");
	return -1;
    }
    
    if (( sas = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in))) == NULL) 
            return -1 ;
    memset( sas, 0, sizeof(struct sockaddr_in));
    sas->sin_family = AF_INET;
    sas->sin_port = htons(3333);
    sas->sin_addr.s_addr = inet_addr("127.0.0.1");
    if(sendto(sock,msg,6, 0, (struct sockaddr *) sas, sizeof(struct sockaddr_in)) > 0) 
      { free(sas);
	return 1;
       }  //sent ok to dest

    free(sas);
    return -1;	//Send failed
}
  
int main ()
{
 send_packet(0x03);
 return 0;
} 

*/
