/* $Id$
 * $URL$
 *
 * gps plugin, code by michu / www.neophob.com, based on nmeap, http://sourceforge.net/projects/nmeap/
 *
 * Copyright (C) 2007 michu
 * Copyright (C) 2004, 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * exported functions:
 *
 * int plugin_init_gps (void)
 *  adds various functions
 *
 */


/* define the include files you need */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/io.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "nmeap.h"


#define SHOW_ALTITUDE 	0x0000001
#define SHOW_SPEED 	0x0000010
#define SHOW_COURSE 	0x0000100
#define SHOW_SATELLITES 0x0001000
#define SHOW_QUALITY 	0x0010000
#define SHOW_WARN 	0x0100000

nmeap_gga_t g_gga;


/* ---------------------------------------------------------------------------------------*/
/* STEP 1 : allocate the data structures. be careful if you put them on the stack because */
/*          they need to be live for the duration of the parser                           */
/* ---------------------------------------------------------------------------------------*/

static nmeap_context_t nmea;	/* parser context */
static nmeap_gga_t gga;		/* this is where the data from GGA messages will show up */
static nmeap_rmc_t rmc;		/* this is where the data from RMC messages will show up */
static int user_data;		/* user can pass in anything. typically it will be a pointer to some user data */
static int fd;



/*************************************************************************
 * LINUX IO
 */

/*
 * open the specified serial port for read/write
 * @return port file descriptor or -1
 */

static int openPort(const char *tty, int baud)
{
    int status;
    int fd;
    struct termios newtio;

    /* open the tty */
    fd = open(tty, O_RDWR | O_NOCTTY);
    if (fd < 0) {
	error("open");
	return fd;
    }

    /* flush serial port */
    status = tcflush(fd, TCIFLUSH);
    if (status < 0) {
	error("tcflush");
	close(fd);
	return -1;
    }

    /* get current terminal state */
    tcgetattr(fd, &newtio);

    /* set to raw terminal type */
    newtio.c_cflag = baud | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNBRK | IGNPAR;
    newtio.c_oflag = 0;

    /* control parameters */
    newtio.c_cc[VMIN] = 1;	/* block for at least one charater */

    /* set its new attrigutes */
    status = tcsetattr(fd, TCSANOW, &newtio);
    if (status < 0) {
	error("tcsetattr() failed: %s", strerror(errno));
	close(fd);
	fd = -1;
	return fd;
    }
    return fd;
}



static void printGps(nmeap_gga_t * gga, nmeap_rmc_t * rmc)
{
    float myCourse = rmc->course;	/* I assume 0 degree = North */

    char courses[8][3] = { "N ", "NO", "O ", "SO", "S ", "SW", "W ", "NW" };
    float degrees[8] = { 22.5f, 67.5f, 112.5f, 157.5f, 202.5f, 247.5f, 292.5f, 337.5f };
    int selectedDegree = 0;
    int n;

    for (n = 0; n < 8; n++) {
	if (myCourse < degrees[n]) {
	    selectedDegree = n;
	    break;
	}
    }

    debug("alt:%.0f speed:%.0f course:%s sat:%d quali:%d, warn:%c\n", gga->altitude, rmc->speed * 1.852f,	/* knots -> km/h */
	  courses[selectedDegree], gga->satellites, gga->quality, rmc->warn	/* A=active or V=Void */
	);

}


/*
#define SHOW_ALTITUDE 	0x0000001
#define SHOW_SPEED 	0x0000010
#define SHOW_COURSE 	0x0000100
#define SHOW_SATELLITES 0x0001000
#define SHOW_QUALITY 	0x0010000
#define SHOW_WARN 	0x0100000
*/

static void parse(RESULT * result, RESULT * theOptions)
{
    char *value = " ";
    int finito = 0;

    int rem;
    int offset;
    int status;
    int len;
    int lame;
    long options;
    char buffer[32];

    options = R2N(theOptions);
    if (options & SHOW_ALTITUDE)

	while (finito == 0) {

	    /* ---------------------------------------- */
	    /* STEP 6 : get a buffer of input          */
	    /* --------------------------------------- */
	    len = rem = read(fd, buffer, sizeof(buffer));
	    if (len <= 0) {
		error("GPS Plugin, Error read from port");
		break;
	    }

	    /* ---------------------------------------------- */
	    /* STEP 7 : process input until buffer is used up */
	    /* ---------------------------------------------- */

	    offset = 0;
	    lame = 0;

	    while (rem > 0) {
		status = nmeap_parseBuffer(&nmea, &buffer[offset], &rem);
		offset += (len - rem);

		switch (status) {
		case NMEAP_GPGGA:
		    /* GOT A GPGGA MESSAGE */
		    lame++;
		    break;
		case NMEAP_GPRMC:
		    /* GOT A GPRMC MESSAGE */
		    lame++;
		    break;
		default:
		    debug("unhandled status: %d", status);
		    break;
		}
		if (lame > 1) {
		    finito = 1;
		    printGps(&gga, &rmc);
		}
	    }

	}


    /* store result */
    SetResult(&result, R_STRING, value);

    free(value);
}



/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_gps(void)
{
    int fd;
    int status;
    char *port = "/dev/usb/tty/1";
    char *test;

    if ((test = getenv("GPS_PORT"))) {	/* define your port via env variable */
	port = test;
    }


    /* --------------------------------------- */
    /* open the serial port device             */
    /* using default 4800 baud for most GPS    */
    /* --------------------------------------- */
    fd = openPort(port, B4800);
    if (fd < 0) {
	/* open failed */
	error("GPS PLUGIN, Error: openPort %d", fd);
	return fd;
    }

    /* --------------------------------------- */
    /*STEP 2 : initialize the nmea context    */
    /* --------------------------------------- */
    status = nmeap_init(&nmea, (void *) &user_data);
    if (status != 0) {
	error("GPS PLUGIN, Error: nmeap_init %d", status);
	exit(1);
    }

    /* --------------------------------------- */
    /*STEP 3 : add standard GPGGA parser      */
    /* -------------------------------------- */
    status = nmeap_addParser(&nmea, "GPGGA", nmeap_gpgga, 0, &gga);
    if (status != 0) {
	error("GPS PLUGIN, Error: nmeap_add GPGGA parser, error:%d", status);
	return -1;
    }

    /* --------------------------------------- */
    /*STEP 4 : add standard GPRMC parser      */
    /* -------------------------------------- */
    /* status = nmeap_addParser(&nmea,"GPRMC",nmeap_gprmc,gprmc_callout,&rmc); */
    status = nmeap_addParser(&nmea, "GPRMC", nmeap_gprmc, 0, &rmc);
    if (status != 0) {
	error("GPS PLUGIN, Error: nmeap_add GPRMC parser, error:%d", status);
	return -1;
    }


    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("gps::parse", 0, parse);

    return 0;
}

void plugin_exit_gps(void)
{
    /* close the serial port */
    close(fd);

}
