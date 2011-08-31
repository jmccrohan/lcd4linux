/* $Id: drv_Sample.c 773 2007-02-25 12:39:09Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/branches/0.10.1/drv_Sample.c $
 *
 * lcd4linux driver for EFN LED modules (manufacturer: COMFILE/South Korea)
 * connected to the ethernet to serial converter EUG 100 (manufacturer ELV/
 * Germany)
 *
 * Up to 256 EFN LED modules listen on a serial bus (9600 BAUD, 1 stop bit,
 * no parity) and are controlled by a 3 byte protocol:
 * Byte 1: 0xff
 * Byte 2: <module address>
 * Byte 3: <Ascii value of character to be diplayed>
 * The module address of each EFN LED module is set hy solder bridges.
 * The driver expects the EFN LED modules to be configured starting 
 * from address 1. The EFN LED module set to address 1 is the right
 * most module.
 *
 * The EUG 100 is an ethernet to seriell converter. The IP address
 * can be set by dhcp. It listens on ports 1000 (data) and 1001 
 * (configuration). Data received on port 1000 is sent onto the serial bus, 
 * while data sent to configuration port sets format of the EUG 100's 
 * serial interface.
 * Due to a bug in the firmware, the transmission is interrupted if '\0' (0x00)
 * is received on port 1000. Consequently, none of the EFN LED modules shall
 * be set to address 0.
 *
 * Copyright (C) 2010 Tilman Glötzner <tilmanglotzner@hotmail.com>
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
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_EFN
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"

/* text mode display */
#include "drv_generic_text.h"



static char Name[] = "EFN";

char *Host;
int Port;
int DataSocket;

static void drv_EFN_clear(void);

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_EFN_open(const char __attribute__ ((unused)) * section)
{
    int sockfd_conf, portno_conf, n;
    struct sockaddr_in serv_addr;
    struct sockaddr_in conf_addr;
    struct hostent *server;
    char buffer[5];

    /* open tcp sockets to config port to EUG 100 t0 set serial parameter */
    /* 9600 BAUD; no parity; 1 stop bit */
    // socket to config EUG
    portno_conf = Port + 1;
    sockfd_conf = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_conf < 0) {
	error("ERROR opening config socket");
	return -1;
    }
    // resolve DNS name of EFN server (= EUG 100)
    server = gethostbyname(Host);
    if (server == NULL) {
	error("ERROR, no such host\n");
	return -1;
    }
    // socket addr for config socket
    bzero((char *) &conf_addr, sizeof(struct sockaddr_in));
    conf_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &conf_addr.sin_addr.s_addr, server->h_length);
    conf_addr.sin_port = htons(portno_conf);

    // open config socket
    if (connect(sockfd_conf, (struct sockaddr *) &conf_addr, sizeof(struct sockaddr_in)) < 0) {
	error("ERROR connecting to config port");
	return -1;
    }
    // sent config message

    bzero(buffer, 5);
    buffer[0] = '4';
    buffer[1] = '0';
    buffer[2] = '1';


    n = write(sockfd_conf, buffer, 3);
    if (n < 0) {
	error("ERROR writing to config socket");
	close(sockfd_conf);
	return -1;
    }
    close(sockfd_conf);

    // open data socket

    DataSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (DataSocket < 0) {
	error("ERROR opening data socket");
	return -1;
    }
    // socket addr for data socket
    bzero((char *) &serv_addr, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(Port);

    if (connect(DataSocket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) < 0) {
	error("ERROR connecting to data socket");
	return -1;
    }
    return 0;
}


static int drv_EFN_close(void)
{
    /* close whatever port you've opened */
    drv_EFN_clear();
    close(DataSocket);
    return 0;
}


/* dummy function that sends something to the display */
static void drv_EFN_send(const char *data, const unsigned int len)
{
    int n;

    // transport command string to EUG 100
    n = write(DataSocket, data, len);

    if (n < 0) {
	error("%s:drv_EFN_send: Failed to write to data socket\n", Name);
    }
}


/* text mode displays only */
static void drv_EFN_clear(void)
{
    char *cmd;
    int max_char, max_cmd, k;

    max_char = DROWS * DCOLS;
    max_cmd = 3 * max_char;	// each EFN module expects 3 bytes

    if ((cmd = malloc(max_cmd)) == NULL) {
	error("%s : Failed to allocate memory in drv_Sample_write\n", Name);
	// return -1;
    } else {
	/* do whatever is necessary to clear the display */
	for (k = 0; k < max_char; k++) {
	    cmd[(3 * k) + 0] = 0xff;
	    cmd[(3 * k) + 1] = k + 1;
	    cmd[(3 * k) + 2] = ' ';
	}
	drv_EFN_send(cmd, max_cmd);
	drv_EFN_send(cmd, max_cmd);
	free(cmd);
	//return 0;
    }
}


/* text mode displays only */
static void drv_EFN_write(const int row, const int col, const char *data, int len)
{
    char *cmd;
    int offset, i, k, max_char, max_cmd;

    max_char = DROWS * DCOLS;
    max_cmd = 3 * max_char;	// each LED blocks expects a 3 byte sequence


    if ((cmd = (char *) malloc(max_cmd)) == NULL) {
	error("%s : Failed to allocate memory in drv_Sample_write\n", Name);
	//return -1;
    } else {
	/* do the cursor positioning here */

	offset = ((row) * DCOLS) + col;

	for (i = max_char - offset, k = 0; ((i > 0) && (k < len)); i--, k++) {
	    cmd[(3 * k) + 0] = 0xff;
	    cmd[(3 * k) + 1] = i;
	    cmd[(3 * k) + 2] = data[k];
	}

	/* send string to the display twice (to make transmission
	 * reliable) */
	drv_EFN_send(cmd, 3 * (k));
	drv_EFN_send(cmd, 3 * (k));

	free(cmd);
	// return 0;
    }
}

static void drv_EFN_defchar(const int __attribute__ ((unused)) ascii, const unsigned char
			    __attribute__ ((unused)) * matrix)
{
    error("%s:drv_EFN_defchar: Function not supported by EFN modules\n", Name);
}

/* start text mode display */
static int drv_EFN_start(const char *section)
{
    int rows = -1, cols = -1;
    char *s;

    Host = cfg_get(section, "Host", NULL);

    if (Host == NULL || *Host == '\0') {
	error("%s: no '%s.Host'  entry from %s", Name, section, cfg_source());
	return -1;
    }

    if (cfg_number(section, "Port", 1000, 0, 65535, &Port) < 0)
	return -1;


    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }

    DROWS = rows;
    DCOLS = cols;

    /* open communication with the display */
    if (drv_EFN_open(section) < 0) {
	return -1;
    }

    /*  initialize display */
    drv_EFN_clear();		/* clear display */

    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/


/****************************************/
/***        widget callbacks          ***/
/****************************************/




/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_EFN_list(void)
{
    printf("EFN LED modules + EUG100 Ethernet to serial converter");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_EFN_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev: 773 $");

    /* display preferences */

    /* real worker functions */
    drv_generic_text_real_write = drv_EFN_write;
    drv_generic_text_real_defchar = drv_EFN_defchar;


    /* start display */
    if ((ret = drv_EFN_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	sleep(3);
	drv_EFN_clear();

    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;


    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    return 0;
}




/* close driver & display */
/* use this function for a text display */
int drv_EFN_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_EFN_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_EFN_close();

    return (0);
}

/* close driver & display */
/* use this one for a text display */
DRIVER drv_EFN = {
    .name = Name,
    .list = drv_EFN_list,
    .init = drv_EFN_init,
    .quit = drv_EFN_quit,
};
