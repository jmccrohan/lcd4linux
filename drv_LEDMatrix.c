/* $Id: drv_LEDMatrix.c,v 1.9 2006/08/14 19:24:22 harbaum Exp $
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
 * Revision 1.9  2006/08/14 19:24:22  harbaum
 * Umlaut support, added KVV HTTP-User-Agent
 *
 * Revision 1.8  2006/08/14 05:54:04  reinelt
 * minor warnings fixed, CFLAGS changed (no-strict-aliasing)
 *
 * Revision 1.7  2006/08/13 18:45:25  harbaum
 * Little cleanup ...
 *
 * Revision 1.6  2006/08/13 18:14:03  harbaum
 * Added KVV plugin
 *
 * Revision 1.5  2006/08/13 09:53:10  reinelt
 * dynamic properties added (used by 'style' of text widget)
 *
 * Revision 1.4  2006/08/13 06:46:51  reinelt
 * T6963 soft-timing & enhancements; indent
 *
 * Revision 1.3  2006/08/09 17:25:34  harbaum
 * Better bar color support and new bold font
 *
 * Revision 1.2  2006/08/08 20:16:28  harbaum
 * Added "extracolor" (used for e.g. bar border) and RGB support for LEDMATRIX
 *
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

#define DSP_DEFAULT_PORT 4711

#define DSP_MEM (80 * 32 * 2 / 8)

#define DEFAULT_X_OFFSET   1	// with a font width of 6

static char Name[] = "LEDMatrix";
static char *IPAddress = NULL;
static int sock = -1;
static struct sockaddr_in dsp_addr;
static unsigned char tx_buffer[DSP_MEM + 1];
static int port = DSP_DEFAULT_PORT;

static void drv_LEDMatrix_blit(const int row, const int col, const int height, const int width)
{
    int r, c, i;
    fd_set rfds;
    struct timeval tv;
    unsigned char reply[256];
    struct sockaddr_in cli_addr;
    socklen_t fromlen;
    int ack = 0;
    int timeout = 10;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    /* LEDMATRIX supports three colors: 10b == green, 01b == red, 11b == amber */

	    unsigned char color = 0;
	    RGBA p = drv_generic_graphic_rgb(r, c);
	    if (p.G >= 128)
		color |= 0x80;
	    if (p.R >= 128)
		color |= 0x40;
	    /* ignore blue ... */

	    tx_buffer[1 + 20 * r + c / 4] &= ~(0xc0 >> (2 * (c & 3)));
	    tx_buffer[1 + 20 * r + c / 4] |= color >> (2 * (c & 3));
	}
    }

    // scan entire display
    tx_buffer[0] = DSP_CMD_IMAGE;

    do {

	if ((sendto(sock, tx_buffer, DSP_MEM + 1, 0, (struct sockaddr *) &dsp_addr, sizeof(dsp_addr))) != DSP_MEM + 1)
	    error("%s: sendto error on socket", Name);

	/* now wait for reply */

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

	// wait 1 sec for ack
	if ((i = select(FD_SETSIZE, &rfds, NULL, NULL, &tv)) < 0) {
	    info("%s: Select error: %s", Name, strerror(errno));
	}

	if (FD_ISSET(sock, &rfds)) {
	    // wait for ack
	    fromlen = sizeof(dsp_addr);
	    i = recvfrom(sock, reply, sizeof(reply), 0, (struct sockaddr *) &cli_addr, &fromlen);
	    if (i < 0) {
		info("%s: Receive error: %s", Name, strerror(errno));
	    } else {
		if ((i == 2) && (reply[0] == DSP_CMD_ACK) && (reply[1] == DSP_CMD_IMAGE)) {
		    ack = 1;
		} else if ((i > 1) && (reply[0] == DSP_CMD_IR)) {
// maybe used later:
//        ir_receive(reply+1, i-1);
		} else {
		    info("%s: Unexpected reply message", Name);
		}
	    }
	}
	timeout--;
    } while ((!ack) && (timeout > 0));

    if (timeout == 0) {
	error("%s: display reply timeout", Name);
    }
}

static int drv_LEDMatrix_start(const char *section)
{
    char *s;
    struct sockaddr_in cli_addr;
    struct hostent *hp;
    int val;

    IPAddress = cfg_get(section, "IPAddress", NULL);
    if (IPAddress == NULL || *IPAddress == '\0') {
	error("%s: no '%s.IPAddress' entry from %s", Name, section, cfg_source());
	return -1;
    }

    if (cfg_number(section, "Port", 0, 0, 65535, &val) > 0) {
	info("%s: port set to %d", Name, val);
	port = val;
    } else {
	info("%s: using default port", Name, port);
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
    info("%s: contacting %s", Name, IPAddress);

    /* try to resolve as a hostname */
    if ((hp = gethostbyname(IPAddress)) == NULL) {
	error("%s: unable to resolve hostname %s: %s", Name, IPAddress, strerror(errno));
	return -1;
    }

    /* open datagram socket */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	error("%s: could not create socket: %s", Name, strerror(errno));
	return -1;
    }

    memset((char *) &dsp_addr, 0, sizeof(dsp_addr));
    dsp_addr.sin_family = AF_INET;
    dsp_addr.sin_addr.s_addr = *(int *) hp->h_addr;
    dsp_addr.sin_port = htons(port);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htons(INADDR_ANY);
    cli_addr.sin_port = htons(port);

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

    if (sock != -1)
	close(sock);

    return (0);
}


DRIVER drv_LEDMatrix = {
  name:Name,
  list:drv_LEDMatrix_list,
  init:drv_LEDMatrix_init,
  quit:drv_LEDMatrix_quit,
};
