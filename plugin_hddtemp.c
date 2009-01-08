/* $Id$
 * $URL$
 *
 * plugin hddtemp
 *
 * Copyright (C) 2007 Scott Bronson <brons_lcd4linux@rinspin.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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

/* Example:
 * (I tried to put this info on the wiki but Akismet claimed it was spam)

	Widget SmallHDDTemp {
		class 'Text'
		expression hddtemp('/dev/hda')
		width 4
		precision 1
		align 'R'
		update tick
	}

	hddtemp home page: http://www.guzu.net/linux/hddtemp.php

	Quick Examples:
	 * hddtemp() -- return temperature of first drive (order is defined by the hddtemp daemon).
	 * hddtemp('/dev/hda') -- return temperature of /dev/hda on localhost.
	 * hddtemp('ST3400832A') -- return temperature of the drive with the given ID.
	 * hddtemp('burly', '') -- return temperature of the first drive on host burly.
	 * hddtemp('burly', 17634, '/dev/hda') -- return temperature of /dev/hda on burly connecting to the hddtemp daemon listening on port 17634.

You can find out what drives are being monitored by running
		telnet HOST PORT

i.e.

	$ telnet localhost 7634
	Trying 127.0.0.1...
	Connected to localhost.
	Escape character is '^]'.
	|/dev/hda|ST3400832A|53|C|Connection closed by foreign host.

This tells me that /dev/hda is a Seagate ST3400832A running at an awfully
toasty 53 degC.

 */


/* 
 * exported functions:
 *
 * int plugin_init_hddtemp (void)
 *  adds various functions
 * void plugin_exit_hddtemp (void)
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* network specific includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"



static int socket_open(const char *name, int port)
{
    struct sockaddr_in server;
    struct hostent *host_info;
    unsigned long addr;
    int sock;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	error("[hddtemp] failed to create socket: %s", strerror(errno));
	return -1;
    }

    memset(&server, 0, sizeof(server));
    if ((addr = inet_addr(name)) != INADDR_NONE) {
	memcpy((char *) &server.sin_addr, &addr, sizeof(addr));
    } else {
	host_info = gethostbyname(name);
	if (NULL == host_info) {
	    error("[hddtemp] Unknown server: %s", name);
	    return -1;
	}
	memcpy((char *) &server.sin_addr, host_info->h_addr, host_info->h_length);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
	error("[hddtemp] can't connect to server %s: %s", name, strerror(errno));
	return -1;
    }

    return sock;
}


static size_t hddtemp_read(int socket, char *buffer, size_t size)
{
    size_t count = 0;
    ssize_t len;

    while (count < size) {
	len = read(socket, buffer + count, size - count);
	if (len > 0) {
	    count += len;
	} else if (len < 0) {
	    error("[hddtemp] couldn't read from socket: %s", strerror(errno));
	    return -1;
	} else {
	    break;		/* remote socket has closed */
	}
    }

    // null-terminate the string
    if (count < size) {
	buffer[count] = '\0';
    } else {
	buffer[size - 1] = '\0';
    }

    return count;
}


static int hddtemp_connect(const char *host, int port, char *buffer, size_t size)
{
    int socket, ret;

    socket = socket_open(host, port);
    if (socket < 0) {
	error("[hddtemp] Error accessing %s:%d: %s", host, port, strerror(errno));
	return -1;
    }

    ret = hddtemp_read(socket, buffer, size);
    close(socket);

    return ret;
}


/* split a value into columns and return the nth column */
/* WARNING: does return a pointer to a static string!! */
static char *split(const char *value, const int column)
{
    static char buffer[256];
    const char *p, *q, *v;
    size_t l;
    int c;

    c = 0;
    p = value;
    q = NULL;
    for (v = value; v && *v; v++) {
	if (*v == '|') {
	    if (c == column) {
		q = v;
		break;
	    } else {
		p = v + 1;
	    }
	    c++;
	}
    }

    if (c < column)
	return NULL;

    l = q ? (size_t) (q - p) : strlen(p);
    if (l >= sizeof(buffer))
	l = sizeof(buffer) - 1;
    strncpy(buffer, p, l);

    buffer[l] = '\0';
    return buffer;
}

static char *hddtemp_fetch(const char *host, int port, const char *device)
{
    char buffer[4096];
    char *key;
    int i;

    /* fetch a buffer of all hddtemps */
    if (!hddtemp_connect(host, port, buffer, sizeof(buffer))) {
	return "err";
    }

    i = 1;
    while (1) {
	key = split(buffer, i);
	if (key == NULL)
	    break;
	if (strcmp(device, key) == 0) {
	    return split(buffer, i + 2);
	}
	i += 5;
    }

    return "n/a";
}


static void my_hddtemp(RESULT * result, int argc, RESULT * argv[])
{
    char *device = "";
    int port = 7634;
    char *host = "localhost";
    char *value;

    switch (argc) {
    case 0:
	break;
    case 1:
	device = R2S(argv[0]);
	break;
    case 2:
	host = R2S(argv[0]);
	device = R2S(argv[1]);
	break;
    case 3:
	host = R2S(argv[0]);
	port = (int) R2N(argv[1]);
	device = R2S(argv[2]);
	break;
    default:
	error("hddtemp(): too many parameters");
	SetResult(&result, R_STRING, "");
	return;
    }

    value = hddtemp_fetch(host, port, device);
    SetResult(&result, R_STRING, value);
}


int plugin_init_hddtemp(void)
{
    AddFunction("hddtemp", -1, my_hddtemp);

    return 0;
}


void plugin_exit_hddtemp(void)
{
}
