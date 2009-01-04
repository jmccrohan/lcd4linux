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
#include "cfg.h"
#include "thread.h"


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


// replaces all null chars in a buffer with a given character.
static void replace_nulls(char *buf, size_t len, char repl)
{
    char *end = buf + len;

    while (buf < end) {
	if (*buf == '\0') {
	    *buf = repl;
	}

	buf += 1;
    }
}


static size_t hddtemp_read(int socket, char *buf, size_t bufsiz)
{
    size_t count = 0;
    ssize_t len;

    while (count < bufsiz) {
	len = read(socket, buf + count, bufsiz - count);
	if (len > 0) {
	    replace_nulls(buf + count, len, '@');
	    count += len;
	} else if (len < 0) {
	    // an error!
	    error("[hddtemp] Couldn't read: %s", strerror(errno));
	    return -1;
	} else {
	    // remote socket has closed
	    break;
	}
    }

    // null-terminate the string
    if (count < bufsiz) {
	buf[count] = '\0';
    } else {
	buf[bufsiz - 1] = '\0';
    }

    return count;
}


static int hddtemp_connect(const char *host, int port, char *buf, size_t bufsiz)
{
    int sock, ret;

    sock = socket_open(host, port);
    if (sock < 0) {
	error("[hddtemp] Error accessing %s:%d: %s", host, port, strerror(errno));
	return -1;
    }

    ret = hddtemp_read(sock, buf, bufsiz);
    close(sock);

    return ret;
}


static char *hddtemp_fetch(char *buf, size_t bufsiz, const char *host, int port, const char *arg)
{
    char *line, *end;

    // fetch a buffer of all hddtemps
    if (!hddtemp_connect(host, port, buf, bufsiz)) {
	return "err";
    }
    // find the line containing the user's specification
    line = strstr(buf, arg);
    if (!line) {
	return "--";
    }
    // back up to the beginning of the line
    while (line > buf && line[-1] != '\n') {
	line--;
    }

    // quick sanity check -- each line begins with the separator
    if (*line != '|') {
	return "fmt1";
    }
    // |device|identifier|temp|units|
    // skip 3 bars to find the start of the temperature
    line = strchr(line + 1, '|');
    if (line && line[0])
	line = strchr(line + 1, '|');
    if (!line || !line[0]) {
	return "fmt2";
    }
    // set the next bar to '\0' to null-terminate temp
    line += 1;
    end = strchr(line, '|');
    if (!end) {
	return "fmt3";
    }
    *end = '\0';

    return line;
}


#ifndef TEST

static void hddtemp_call(RESULT * result, int argc, RESULT * argv[])
{
    char buf[4096];
    const char *search = "";
    const char *host = "localhost";
    int port = 7634;
    char *out;

    switch (argc) {
    case 0:
	break;
    case 1:
	search = R2S(argv[0]);
	break;
    case 2:
	host = R2S(argv[0]);
	search = R2S(argv[1]);
    default:
	if (argc > 3) {
	    error("hddtemp(): too many parameters");
	}
	host = R2S(argv[0]);
	port = (int) R2N(argv[1]);
	search = R2S(argv[2]);
    }

    out = hddtemp_fetch(buf, sizeof(buf), host, port, search);
    SetResult(&result, R_STRING, out);
}


int plugin_init_hddtemp(void)
{
    info("Adding hddtemp functions!");

    AddFunction("hddtemp", -1, hddtemp_call);

    return 0;
}


void plugin_exit_hddtemp(void)
{
}
#endif



#ifdef TEST
// Add this to your makefile to make the test_hddtemp app:
// test_hddtemp: plugin_hddtemp.c
//      gcc -Wall -Werror -g -DTEST plugin_hddtemp.c -o $@

#include <stdarg.h>

void message(const int level, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    putchar('\n');
    va_end(ap);
}

int main(int argc, char **argv)
{
    char buf[4096];
    char *out = hddtemp_fetch(buf, sizeof(buf), argv[1] ? argv[1] : "/dev/hda");
    printf("OUTPUT: %s\n", out);
    return 0;
}
#endif
