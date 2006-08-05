/* $Id: drv_LEDMatrix.c,v 1.1 2006/08/05 21:08:01 harbaum Exp $
 *
 * LED matrix driver for LCD4Linux 
 * (see http://www.harbaum.org/till/ledmatrix for hardware)
 *
 * Copyright (C) 2006 Till Harbaum <till@harbaum.org>
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
 * $Log: drv_LEDMatrix.c,v $
 * Revision 1.1  2006/08/05 21:08:01  harbaum
 * New LEDMATRIX driver (see http://www.harbaum.org/till/ledmatrix)
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_LEDMatrix
 *
 */

/*
 * Options:
 * IPAddress
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

/* include network specific headers */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h> 

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_graphic.h"

// display command bytes
#define DSP_CMD_ECHO  0
#define DSP_CMD_NOP   1
#define DSP_CMD_IMAGE 2
#define DSP_CMD_ACK   3
#define DSP_CMD_IR    4
#define DSP_CMD_BEEP  5

#define DSP_PORT 4711

#define DSP_MEM (80 * 32 * 2 / 8)

static char Name[] = "LEDMatrix";
static char *IPAddress = NULL;
static int sock = -1;
static struct sockaddr_in dsp_addr;
static unsigned char tx_buffer[DSP_MEM+1];

#if 0
typedef enum { RED, GREEN, AMBER } col_t;

static col_t fg_col, bg_col, hg_col;
#endif


static void drv_LEDMatrix_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    /* drv_generic_graphic_black() returns 1 if pixel is black */
	    /* drv_generic_graphic_gray() returns a gray value 0..255 */
	    /* drv_generic_graphic_rgb() returns a RGB color */
	    if (drv_generic_graphic_black(r, c)) {
	        tx_buffer[1 + 20*r + c/4] |=    0xc0>>(2*(c&3));
	    } else {
	        tx_buffer[1 + 20*r + c/4] &=  ~(0xc0>>(2*(c&3)));
	    }
	}
    }

    // scan entire display
    tx_buffer[0] = DSP_CMD_IMAGE;
    if((sendto(sock, tx_buffer, DSP_MEM+1, 0, 
	       (struct sockaddr *)&dsp_addr, 
	       sizeof(dsp_addr))) != DSP_MEM+1)
      error("%s: sendto error on socket", Name);
}

static int drv_LEDMatrix_start(const char *section)
{
    char *s;
    struct sockaddr_in cli_addr;
    struct hostent *hp;

    IPAddress = cfg_get(section, "IPAddress", NULL);
    if (IPAddress == NULL || *IPAddress == '\0') {
	error("%s: no '%s.IPAddress' entry from %s", Name, section, cfg_source());
	return -1;
    }

    /* display size is hard coded */
    DCOLS = 80;
    DROWS = 32;

    if (sscanf(s = cfg_get(section, "font", "6x8"), "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad %s.Font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    /* contact display */
    info("%s: contacting %s\n", Name, IPAddress);

    /* try to resolve as a hostname */
    if((hp = gethostbyname(IPAddress)) == NULL) {
        error("%s: unable to resolve hostname %s: %s", Name, IPAddress, strerror(errno));
	return -1;
    }

    /* open datagram socket */
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("%s: could not create socket: %s", Name, strerror(errno));
	return -1;
    } 
    
    memset((char *) &dsp_addr, 0, sizeof(dsp_addr));
    dsp_addr.sin_family = AF_INET;
    dsp_addr.sin_addr.s_addr = *(int*)hp->h_addr;
    dsp_addr.sin_port = htons(DSP_PORT);
    
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htons(INADDR_ANY);
    cli_addr.sin_port = htons(DSP_PORT);
    
    if (bind(sock, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
        error("%s: can't bind local address: %s", Name, strerror(errno));
	return -1;
    }
    
    memset(tx_buffer, 0, sizeof(tx_buffer));

    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_graphic_draw(W) */
/* using drv_generic_graphic_icon_draw(W) */
/* using drv_generic_graphic_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_LEDMatrix_list(void)
{
    return 0;
}


/* initialize driver & display */
int drv_LEDMatrix_init(const char *section, const __attribute__ ((unused))
		 int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_LEDMatrix_blit;

    /* start display */
    if ((ret = drv_LEDMatrix_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_graphic_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_graphic_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_graphic_bar_draw;
    widget_register(&wc);

    /* register plugins */
    /* none at the moment... */


    return 0;
}


/* close driver & display */
int drv_LEDMatrix_quit(const __attribute__ ((unused))
		 int quiet)
{

    info("%s: shutting down.", Name);
    drv_generic_graphic_quit();

    if(sock != -1)
        close(sock);

    return (0);
}


DRIVER drv_LEDMatrix = {
  name:Name,
  list:drv_LEDMatrix_list,
  init:drv_LEDMatrix_init,
  quit:drv_LEDMatrix_quit,
};
