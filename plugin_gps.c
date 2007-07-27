/* $Id$
 * $URL$
 *
 * gps plugin (nmea), code by michu / www.neophob.com, based on nmeap, http://sourceforge.net/projects/nmeap/
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
 * GPS Plugin for lcd4linux, by michael vogt / http://www.neophob.com
 * contact: michu@neophob.com
 *
 * based on nmeap by daveh, http://www.dmh2000.com/ or http://sourceforge.net/projects/nmeap/
 * you need a compiled libnmeap to compile this plugin.
 *
 * History:
 * 	v0.1 initial release
 *
 * TODO:
 *	-update direction only when speed > 5 kmh
 *	-add more data fields
 *	-dump nmea string to external file
 *
 * Exported functions:
 * int plugin_init_gps (void) - default plugin initialisation
 * void parse(SHOW-OPTIONS, DISPLAY-OPTIONS)
 * 	display option define, what the plugin should display
 *
 *	OPTION:						EXAMPLE:
 *	-------						--------
 *
 * PARAMETER 1:
 * 	#define SHOW_ALTITUDE 		0x000000001	alt:500
 * 	#define SHOW_SPEED 		0x000000010	spd:30
 * 	#define SHOW_COURSE 		0x000000100	dir:NO
 * 	#define SHOW_SATELLITES 	0x000001000	sat:4
 * 	#define SHOW_QUALITY 		0x000010000	qua:1
 * 	#define SHOW_STATUS 		0x000100000	sta:V
 *	#define SHOW_TIME_UTC   	0x001000000	utc:113459
 *	#define SHOW_DATE 		0x010000000	dat:190204 (19.02.2004)
 *
 * PARAMETER 2: 
 *	#define DISPLAY_NO_PREFIX 	0x000000001	disable prefix (example, instead of "alt:500" it displays "500"
 *	#define DISPLAY_SPEED_IN_KNOTS 	0x000000010	when use the SHOW_SPEED option, display speed in knots instead in km/h
 *
 *	Examples:  
 *		- gps::parse('0x011','0') will display the altitude and speed -> alt:500 spd:43
 *		- gps::parse('0x01100','0') will display the course and the numbers of satellites -> dir:NO sat:3
 *		- gps::parse('0x01','0x01') will display the speed without prefix -> 50
 *		- gps::parse('0x01','0x01') will display the speed in knots without prefix -> 27
 */


/* define the include files you need */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "nmeap.h"

//#define EMULATE                       //remove comment to enable gps data emulation...

#define SHOW_ALTITUDE 		0x000000001
#define SHOW_SPEED 		0x000000010
#define SHOW_COURSE 		0x000000100
#define SHOW_SATELLITES 	0x000001000
#define SHOW_QUALITY 		0x000010000
#define SHOW_STATUS 		0x000100000
#define SHOW_TIME_UTC 		0x001000000
#define SHOW_DATE 		0x010000000

#define DISPLAY_NO_PREFIX 	0x000000001
#define DISPLAY_SPEED_IN_KNOTS 	0x000000010


static float course = 0.f;	//degrees
static float altitude = 0.f;
static float speed = 0.f;	//Speed over ground in KNOTS!!
static int satellites = 0;	//Number of satellites in use (00-12)
static int quality = 0;		//GPS quality indicator (0 - fix not valid, 1 - GPS fix, 2 - DGPS fix) 
static char gpsStatus = 'V';	//A=active or V=Void 
static unsigned long gpsTime = 0;	//UTC of position fix in hhmmss format 
static unsigned long gpsDate = 0;	//Date in ddmmyy format

static int msgCounter = 0;	//debug counter
/* ---------------------------------------------------------------------------------------*/
/* STEP 1 : allocate the data structures. be careful if you put them on the stack because */
/*          they need to be live for the duration of the parser                           */
/* ---------------------------------------------------------------------------------------*/
static nmeap_context_t nmea;	/* parser context */
static nmeap_gga_t gga;		/* this is where the data from GGA messages will show up */
static nmeap_rmc_t rmc;		/* this is where the data from RMC messages will show up */
static int user_data;		/* user can pass in anything. typically it will be a pointer to some user data */

static int fd_g;		/* port handler */


#ifdef EMULATE
char test_vector[] = {
    "$GPGGA,123519,3929.946667,N,11946.086667,E,1,08,0.9,545.4,M,46.9,M,,*4A\r\n"	/* good */
	"$xyz,1234,asdfadfasdfasdfljsadfkjasdfk\r\n"	/* junk */
	"$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68\r\n"	/* good */
	"$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*48\r\n"	/* checksum error */
};
#endif


/*************************************************************************

/*
 * open the specified serial port for read/write
 * @return port file descriptor or -1
 */

#ifndef EMULATE
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
#endif

/** called when a gpgga message is received and parsed */
static void gpgga_callout( __attribute__ ((unused)) nmeap_context_t * context, void *data, __attribute__ ((unused))
			  void *user_data)
{
    nmeap_gga_t *gga = (nmeap_gga_t *) data;

    altitude = gga->altitude;
    satellites = gga->satellites;
    quality = gga->quality;
    msgCounter++;
}

/** called when a gprmc message is received and parsed */
static void gprmc_callout( __attribute__ ((unused)) nmeap_context_t * context, void *data, __attribute__ ((unused))
			  void *user_data)
{
    nmeap_rmc_t *rmc = (nmeap_rmc_t *) data;

    speed = rmc->speed;
    gpsStatus = rmc->warn;
    gpsTime = rmc->time;
    gpsDate = rmc->date;
    msgCounter++;
}


static int prepare_gps_parser()
{
    int status;
    char *port = "/dev/usb/tts/1";
    char *test;

    if ((test = getenv("GPS_PORT"))) {	/* define your port via env variable */
	port = test;
    }

    /* --------------------------------------- */
    /* open the serial port device             */
    /* using default 4800 baud for most GPS    */
    /* --------------------------------------- */
#ifndef EMULATE
    fd_g = openPort(port, B4800);
    if (fd_g < 0) {
	/* open failed */
	error("GPS PLUGIN, Error: openPort %d", fd_g);
	return fd_g;
    }
#endif

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
    status = nmeap_addParser(&nmea, "GPGGA", nmeap_gpgga, gpgga_callout, &gga);
    if (status != 0) {
	error("GPS PLUGIN, Error: nmeap_add GPGGA parser, error:%d", status);
	return -1;
    }

    /* --------------------------------------- */
    /*STEP 4 : add standard GPRMC parser      */
    /* -------------------------------------- */
    /* status = nmeap_addParser(&nmea,"GPRMC",nmeap_gprmc,gprmc_callout,&rmc); */
    status = nmeap_addParser(&nmea, "GPRMC", nmeap_gprmc, gprmc_callout, &rmc);
    if (status != 0) {
	error("GPS PLUGIN, Error: nmeap_add GPRMC parser, error:%d", status);
	return -1;
    }

    return fd_g;
}



static void parse(RESULT * result, RESULT * theOptions, RESULT * displayOptions)
{
//    char *value = " ";
//    int finito = 0;

    int rem;
    int offset;
    int status;
    int len;
    long options;
    long dispOptions;
#ifndef EMULATE
    char buffer[128];
#endif

    options = R2N(theOptions);
    dispOptions = R2N(displayOptions);
    //error("options: %x\n",options);

    /* ---------------------------------------- */
    /* STEP 6 : get a buffer of input          */
    /* --------------------------------------- */
#ifdef EMULATE
    len = rem = sizeof(test_vector);
#else
    len = rem = read(fd_g, buffer, sizeof(buffer));

    /* --------------------------------------- */
    /*STEP 7 : pass it to the parser           */
    /* status indicates whether a complete msg */
    /* arrived for this byte                   */
    /* NOTE : in addition to the return status */
    /* the message callout will be fired when  */
    /* a complete message is processed         */
    /* --------------------------------------- */
    if (len <= 0) {
	error("GPS Plugin, Error read from port, try using the GPS_PORT env variable (export GPS_PORT=/dev/mydev)");
	//break;
    }
#endif

    /* ---------------------------------------------- */
    /* STEP 7 : process input until buffer is used up */
    /* ---------------------------------------------- */
    offset = 0;
    while (rem > 0) {
#ifdef EMULATE
	status = nmeap_parseBuffer(&nmea, &test_vector[offset], &rem);
#else
	status = nmeap_parseBuffer(&nmea, &buffer[offset], &rem);
//      error("loop, remaining=%d, read bytes=%d\n",rem,offset);        
#endif
	offset += (len - rem);
    }

    /* --------------------------------------- */
    /* DISPLAY stuff comes here...             */
    /* --------------------------------------- */
    char *value = " ";
    char outputStr[80];
    memset(outputStr, 0, 80);

    if (options & SHOW_ALTITUDE) {
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%.0f ", outputStr, altitude);
	else
	    sprintf(outputStr, "%salt:%.0f ", outputStr, altitude);
    }
    if (options & SHOW_SPEED) {
	float knotsConvert = 1.852f;	//default speed display=km/h
	if (dispOptions & DISPLAY_SPEED_IN_KNOTS)
	    knotsConvert = 1.0f;	//use knots

	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%.0f ", outputStr, speed * knotsConvert);
	else
	    sprintf(outputStr, "%sspd:%.0f ", outputStr, speed * knotsConvert);
    }
    if (options & SHOW_COURSE) {
	char courses[8][3] = { "N ", "NO", "O ", "SO", "S ", "SW", "W ", "NW" };
	float degrees[8] = { 22.5f, 67.5f, 112.5f, 157.5f, 202.5f, 247.5f, 292.5f, 337.5f };
	int selectedDegree = 0;
	int n;

	for (n = 0; n < 8; n++) {
	    if (course < degrees[n]) {
		selectedDegree = n;
		break;
	    }
	}
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%s ", outputStr, courses[selectedDegree]);
	else
	    sprintf(outputStr, "%sdir:%s ", outputStr, courses[selectedDegree]);
    }
    if (options & SHOW_SATELLITES) {
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%d ", outputStr, satellites);
	else
	    sprintf(outputStr, "%ssat:%d ", outputStr, satellites);
    }
    if (options & SHOW_QUALITY) {
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%d ", outputStr, quality);
	else
	    sprintf(outputStr, "%squa:%d ", outputStr, quality);
    }
    if (options & SHOW_STATUS) {
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%c ", outputStr, gpsStatus);
	else
	    sprintf(outputStr, "%ssta:%c ", outputStr, gpsStatus);
    }
    if (options & SHOW_TIME_UTC) {
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%ld ", outputStr, gpsTime);
	else
	    sprintf(outputStr, "%sutc:%ld ", outputStr, gpsTime);
    }
    if (options & SHOW_DATE) {
	if (dispOptions & DISPLAY_NO_PREFIX)
	    sprintf(outputStr, "%s%ld ", outputStr, gpsDate);
	else
	    sprintf(outputStr, "%sdat:%ld ", outputStr, gpsDate);
    }

    value = strdup(outputStr);
    SetResult(&result, R_STRING, value);
    free(value);
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_gps(void)
{
    printf("PLUGIN INIT\n");
    prepare_gps_parser();

    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("gps::parse", 2, parse);

    return 0;
}

void plugin_exit_gps(void)
{
#ifndef EMULATE
    close(fd_g);
#endif
}
