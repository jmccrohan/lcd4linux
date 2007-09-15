/* $Id$
 * $URL$
 *
 * gps plugin (nmea), code by michu / www.neophob.com, based on nmeap, http://sourceforge.net/projects/nmeap/
 *
 * Copyright (C) 2007 Michu - http://www.neophob.com
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
 * 	v0.1 -initial release
 *	v0.2 -fix include files, include <termios.h> and <asm/fcntl.h> (embedded devices)
 *           -fixed emulation mode (read only x bytes instead of the whole emulated nmea data)
 *  	     -improved error handling (if there are no parameters defined, lcd4linx will display "GPS ARG ERR")
 *	     -display raw nmea string from the gps receiver
 *	     -added support for gps device with 9600 baud (env. variable)
 *	     -added the option that the widget graps data from the buffer and not from the gps device (usefull for multible widgets)
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
 *	#define OPTION_NO_PREFIX 	0x000000001	disable prefix (example, instead of "alt:500" it displays "500"
 *	#define OPTION_SPEED_IN_KNOTS 	0x000000010	when use the SHOW_SPEED option, display speed in knots instead in km/h
 *	#define OPTION_RAW_NMEA 	0x000000100	outputs the parsed nmea string, only valid when EMULATE is not defined!
 *	#define OPTION_GET_BUFFERDATA	0x000001000	when you define more than 1 gps widget
 *							each widget will get updates and cause some ugly side effects, specially when you display the time
 *							by enabling this option, the widget will not read any nmea data from the serial port.
 *							KEEP IN MIND that there must be ONE widget which does NOT get buffered data (means read data from the port)
 *
 *	Examples:  
 *		- gps::parse('0x011','0') will display the altitude and speed -> alt:500 spd:43
 *		- gps::parse('0x01100','0') will display the course and the numbers of satellites -> dir:NO sat:3
 *		- gps::parse('0x01','0x01') will display the speed without prefix -> 50
 *		- gps::parse('0x01','0x01') will display the speed in knots without prefix -> 27
 *
 * ENV VARIABLES
 *	To finetune plugin_gps you may define some env variables:
 *		GPS_PORT	gps device, default value is /dev/usb/tts/1
 *		GPS_9600	serial port speed, default is 4800 baud, if you define this var speed will be 9600 (export GPS_9600=dummy)
 */


/* define the include files you need */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>		//used for serial port flags
#include <asm/fcntl.h>


/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "nmeap.h"

#define EMULATE			//remove comment to enable gps data emulation...
#define EMU_BUFFER_READ_SIZE 32	//how many bytes are read each loop aka emulation speed

#define SHOW_ALTITUDE 		0x000000001
#define SHOW_SPEED 		0x000000010
#define SHOW_COURSE 		0x000000100
#define SHOW_SATELLITES 	0x000001000
#define SHOW_QUALITY 		0x000010000
#define SHOW_STATUS 		0x000100000
#define SHOW_TIME_UTC 		0x001000000
#define SHOW_DATE 		0x010000000

#define OPTION_NO_PREFIX 	0x000000001
#define OPTION_SPEED_IN_KNOTS 	0x000000010
#define OPTION_RAW_NMEA 	0x000000100	//outputs the parsed nmea string, only valid when EMULATE is not defined!
#define OPTION_GET_BUFFERDATA	0x000001000	//when you define more than 1 gps widget
						//each widget will get updates and cause some ugly side effects, specially when you display the time
						//by enabling this option, the widget will not read any nmea data from the serial port.
						//KEEP IN MIND that there must be ONE widget which does not get buffered data (means read data from the port)
#define OPTION_DEBUG		0x000010000


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
static unsigned int emu_read_ofs = 0;
static int debug = 0;		//debug flag

static char Name[] = "plugin_gps.c";

#ifdef EMULATE
char test_vector[] = {
/*    "$GPGGA,123519,3929.946667,N,11946.086667,E,1,08,0.9,545.4,M,46.9,M,,*4A\r\n"	// good
	"$xyz,1234,asdfadfasdfasdfljsadfkjasdfk\r\n"	// junk 
	"$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68\r\n"	// good 
	"$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*48\r\n"	// checksum error 
	"$GPRMC,165110.000,A,5601.0318,N,01211.3503,E,0.09,28.11,190706,,*35\r\n"	//some more data
    "$GPGGA,165111.000,5601.0318,N,01211.3503,E,1,07,1.2,22.3,M,41.6,M,,0000*65\r\n"
	"$GPRMC,165111.000,A,5601.0318,N,01211.3503,E,0.08,43.53,190706,,*3E\r\n"
	"$GPGGA,165112.000,5601.0318,N,01211.3503,E,1,07,1.2,22.3,M,41.6,M,,0000*66\r\n"
	"$GPRMC,165112.000,A,5601.0318,N,01211.3503,E,0.08,37.38,190706,,*33\r\n"
	"$GPGGA,165113.000,5601.0318,N,01211.3503,E,1,07,1.2,22.4,M,41.6,M,,0000*60\r\n"
	"$GPRMC,165113.000,A,5601.0318,N,01211.3503,E,0.11,21.03,190706,,*35\r\n"
	"$GPGGA,165114.000,5601.0318,N,01211.3504,E,1,07,1.2,22.6,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165114.000,A,5601.0318,N,01211.3504,E,0.08,45.67,190706,,*3D\r\n"
	"$GPGGA,165115.000,5601.0318,N,01211.3504,E,1,07,1.2,22.7,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165115.000,A,5601.0318,N,01211.3504,E,0.10,59.41,190706,,*3C\r\n"
	"$GPGGA,165116.000,5601.0318,N,01211.3504,E,1,07,1.2,22.8,M,41.6,M,,0000*6E\r\n"
	"$GPRMC,165116.000,A,5601.0318,N,01211.3504,E,0.06,50.22,190706,,*34\r\n"
	"$GPGGA,165117.000,5601.0318,N,01211.3505,E,1,07,1.2,22.9,M,41.6,M,,0000*6F\r\n"
	"$GPRMC,165117.000,A,5601.0318,N,01211.3505,E,0.08,45.32,190706,,*3F\r\n"
	"$GPGGA,165118.000,5601.0318,N,01211.3505,E,1,07,1.2,23.0,M,41.6,M,,0000*68\r\n"
	"$GPRMC,165118.000,A,5601.0318,N,01211.3505,E,0.10,37.49,190706,,*30\r\n"
	"$GPGGA,165119.000,5601.0318,N,01211.3504,E,1,06,1.2,23.0,M,41.6,M,,0000*69\r\n"
	"$GPRMC,165119.000,A,5601.0318,N,01211.3504,E,0.08,27.23,190706,,*34\r\n"
	"$GPGGA,165120.000,5601.0318,N,01211.3504,E,1,07,1.2,23.0,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165120.000,A,5601.0318,N,01211.3504,E,0.08,41.52,190706,,*38\r\n"
	"$GPGGA,165121.000,5601.0319,N,01211.3505,E,1,07,1.2,23.1,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165121.000,A,5601.0319,N,01211.3505,E,0.09,57.19,190706,,*30\r\n"
	"$GPGGA,165122.000,5601.0319,N,01211.3505,E,1,07,1.2,23.2,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165122.000,A,5601.0319,N,01211.3505,E,0.10,30.60,190706,,*34\r\n"
	"$GPGGA,165123.000,5601.0319,N,01211.3505,E,1,07,1.2,23.3,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165123.000,A,5601.0319,N,01211.3505,E,0.07,45.49,190706,,*3A\r\n"
	"$GPGGA,165124.000,5601.0319,N,01211.3505,E,1,07,1.2,23.4,M,41.6,M,,0000*62\r\n"
	"$GPRMC,165124.000,A,5601.0319,N,01211.3505,E,0.09,34.85,190706,,*35\r\n"
	"$GPGGA,165125.000,5601.0319,N,01211.3506,E,1,07,1.2,23.4,M,41.6,M,,0000*60\r\n"
	"$GPRMC,165125.000,A,5601.0319,N,01211.3506,E,0.06,43.06,190706,,*33\r\n"
	"$GPGGA,165126.000,5601.0319,N,01211.3506,E,1,07,1.2,23.4,M,41.6,M,,0000*63\r\n"
	"$GPRMC,165126.000,A,5601.0319,N,01211.3506,E,0.10,37.63,190706,,*37\r\n"
	"$GPGGA,165127.000,5601.0319,N,01211.3505,E,1,07,1.2,23.4,M,41.6,M,,0000*61\r\n"
	"$GPRMC,165127.000,A,5601.0319,N,01211.3505,E,0.07,42.23,190706,,*35\r\n"
	"$GPGGA,165128.000,5601.0319,N,01211.3505,E,1,07,1.2,23.4,M,41.6,M,,0000*6E\r\n"
	"$GPRMC,165128.000,A,5601.0319,N,01211.3505,E,0.09,27.98,190706,,*37\r\n"
	"$GPGGA,165129.000,5601.0319,N,01211.3505,E,1,07,1.2,23.3,M,41.6,M,,0000*68\r\n"
	"$GPRMC,165129.000,A,5601.0319,N,01211.3505,E,0.08,51.89,190706,,*36\r\n"
	"$GPGGA,165130.000,5601.0319,N,01211.3505,E,1,07,1.2,23.2,M,41.6,M,,0000*61\r\n"
	"$GPRMC,094055.000,A,5409.998645,N,00859.370546,E,0.044,0.00,301206,,,A*55\r\n"
	"$GPGGA,094056.000,5409.998657,N,00859.370528,E,1,12,0.82,-5.168,M,45.414,M,,*47\r\n"
	"$GPRMC,094056.000,A,5409.998657,N,00859.370528,E,0.263,0.00,301206,,,A*5A\r\n"
	"$GPGGA,094057.000,5409.998726,N,00859.370512,E,1,12,0.82,-5.171,M,45.414,M,,*40\r\n"
	"$GPRMC,094057.000,A,5409.998726,N,00859.370512,E,0.311,0.00,301206,,,A*51\r\n"
	"$GPGGA,094058.000,5409.998812,N,00859.370518,E,1,12,0.82,-5.172,M,45.414,M,,*4E\r\n"
	"$GPRMC,094058.000,A,5409.998812,N,00859.370518,E,0.423,0.00,301206,,,A*5A\r\n"
	"$GPGGA,094059.000,5409.998934,N,00859.370505,E,1,12,0.82,-5.177,M,45.414,M,,*43\r\n"
	"$GPRMC,094059.000,A,5409.998934,N,00859.370505,E,0.576,0.00,301206,,,A*53\r\n"
	"$GPGGA,094100.000,5409.999097,N,00859.370542,E,1,12,0.82,-5.177,M,45.414,M,,*4C\r\n"
	"$GPRMC,094100.000,A,5409.999097,N,00859.370542,E,0.705,0.00,301206,,,A\r\n" */
	"$GPGGA,004037.851,0000.0000,N,00000.0000,E,0,00,50.0,0.0,M,0.0,M,0.0,0000*7A\r\n"
	"$GPGGA,175218.255,4657.3391,N,00726.2666,E,1,04,9.0,568.6,M,48.0,M,0.0,0000*7B\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.08\r\n"
	"$GPRMC,175218.255,A,4657.3391,N,00726.2666,E,0.$GPGGA,175219.255,4657.3391,N,00726.2666,E,1,04,9.0,568.5,M,48.0,M,0.0,0000*79\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.7*38\r\n"
	"$7,,*06\r\n"
	"$GPGGA,175220.255,4657.3392,N,00726.2667,E,1,04,9.0,568.4,M,48.0,M,0.0,0000*70\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,$GPGGA,175221.255,4657.3392,N,00726.2667,E,1,04,9.0,568.3,M,48.0,M,0.0,0000*76\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.7*38\r\n"
	"$GPRMC,175221.255,A,4657.3392,N,00726.2667,E,0.14,150.24,230507,,*0A\r\n"
	"$GPGGA,175222.255,4657.3393,N,00726.2668,E,1,04,9.0,568,29,17,12,,,,,,,,,9.5,9.0,2.7*38\r\n"
	"$GPGSV,2,1,08,26,71,153,45,29,60,14$GPGGA,175223.255,4657.3393,N,00726.2669,E,1,04,9.0,568.2,M,48.0,M,7.3393,N,00726.2669,E,0.11,141.32,230507,,*05\r\n"
	"$GPGGA,175224.255,4657.3394,N,00726.2670,E,1,04,9.0,568.0,M,48.0,M,0.0,0000*70\r\n"
	"$GPGSA,$GPGGA,175225.255,4657.3395,N,00726.2670,E,1,04,9.0,567.8,M,48.0,M,0.0,0000*77\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.7*38\r\n"
	"$GPRMC,175225.255,A,4657.3395,N,00726.2670,E,0.10,146.99,230507,,*0A\r\n"
	"$GPGGA,175226.255,465567.5,M,48.0,M,0.0,0000*7B\r\n"
	"$GPGSA,A,3,26,29,17,12$GPGGA,175227.255,4657.3397,N,00726.2671,E,1,04,9.0,567.1,M,48.0,M,0.0,0000*7F\r\n"
	"$.7*38\r\n"
	"$GPGSV,2,1,08,26,71,153,46,29,60,149,46,09,55,286,00,28,32,051$GPGGA,175228.254,4657.3399,N,00726.2672,E,1,04,9.0,566.8,M,48.0,M,0.0,0000*74\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.72,E,0.12,146.43,230507,,*0D\r\n"
	"$GPGGA,175229.254,4657.3400,N,00726.2673,E,1,04,9.0,566.4,M,48.0,M,0.0,0000*7F\r\n"
	"$GPGSA,A,3,26,29,17,1$GPGGA,175230.254,4657.3401,N,00726.2675,E,1,04,9.0,566.0,M,48.0,M,0.0,0000*74\r\n"
	"$GPGSA,A,3,26,29,17,12$GPGGA,175231.254,4657.3402,N,00726.2676,E,1,04,9.0,565.7,M,48.0,M,0.0,0000*71\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.7*38\r\n"
	"$GPRMC,175231.2$GPGGA,175232.254,4657.3404,N,00726.2677,E,1,04,9.,153,45,29,60,149,46,09,55,286,00,28,32,051,00*7F\r\n"
	"$GPGSV,2,2,08,17,32,102,38,18,28,298,00,12,12,21$GPGGA,175233.254,4657.3405,N,00726.2678,E,1,04,9.0,565.1,M,48.0,M,0.0,0000*7C\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$,*09\r\n"
	"$GPGGA,175234.254,4657.3406,N,00726.2679,E,1,04,9.0,564.9,M,48.0,M,0.0,0000*70\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,175234.254,A,4657.3$GPGGA,175235.254,4657.3408,N,00726.2680,E,1,04,9.00,M,0.0,0000*76\r\n"
	"$GPGSA,A,3,26,$GPGGA,175236.254,439\r\n"
	"$GPRMC,175236.254,A,4657.3409,N,00726.2681,E,0.14,142.56,230507,,*06\r\n"
	"$GPGGA,175237.254,4657.3410,N,00726.2682,E,1,04,9.0,564.5,M,48.0,M,0.0,0000*7C\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,00,28,32,051,00*7F\r\n"
	"$GPGSV,2,2,08,17,32,102,39,18,28,298,00,12,12,218,35,22,09,3$GPGGA,175238.254,4657.3411,N,00726.2682,E,1,04$GPGGA,175239.254,4657.3412,N,00726.2683,E,1,04,9.0,564.7,M,48.0,M,0.0,0000*73\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,175239.254,A,4657.3412,N$GPGGA,175240.254,4657.3412,N,00726.2684,E,1,04,9.0,564.8,M,48.0,M,0.0,0000*75\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,$GPGGA,175241.254,4657.3413,N,00726.2684,E,1,04,9.0,565.1,M,48.0,M,0.0,0000*7D\r\n"
	",,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,175241.254,A,4657.3413,N,00726.2684,E,0.$GPGGA,175242.254,4657.3413,N,00726.2684,E,1,04,9.0,565.3,M,48.0,M,0.0,0000*7C\r\n"
	"$GPGSA,A,3,26,29,179,60,149,46,09,55,286,00,28,32,051,00*7C\r\n"
	"$GPGS$GPGGA,175243.254,4657.3414,N,00726.2685,E,1,04,9.0,565.7,M,48.0,M,0.0,0000*7F\r\n"
	"$GPGSA,A,3$GPGGA,175244.253,4657.3414,N,00726.2685,E,1,04,9.0,566.0,M,48.0,M,0.0,0000*7B\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPGGA,175245.253,4657.3415,N,00726.2685,E,1,04,9.0,566.3,M,48.0,M,0.0,0000*78\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,175245.253,A,4657.3415,N,00726.2685,E,0.2$GPGGA,175246.253,4657.3415,N,00726.2686,E,1,04,9.0,566.5,M,48.0,M,0.0,0000*7E\r\n"
	",175246.253,A,4657.3415,N,00726.2686,E,0.13,145$GPGGA,175247.253,4657.3416,N,00726.2686,E,1,04,9.0,566.8,M,48.0,M,0.0,0000*71\r\n"
	"$GPGSA,A,3,26,17,32,101,39*7D\r\n"
	"$GPGSV,2,2,08,28,32,051,00,18,$GPGGA,175248.253,4657.3417,N,00726.2686,E,1,04,9.0,567.0,M,48.0,M,0.0,0000*76\r\n"
	"$GPGSA,A,3,26,29$GPGGA,175249.253,4657.3417,N,00726.2686,E,1,04.3,M,48.0,M,0.0,0000*7$GPGGA,175250.253,4657.3418,N,00726.2686,E,1,04,9.0,567.4,M,48.0,M,0.0,0000*74\r\n"
	".5,9.0,2.6*39\r\n"
	"$GPRMC,175250.253,A,4657.3$GPGGA,175251.253,4657.3418,N,00726.2687,E,1,04,9.0,567.5,M,48.0,M,0.0,0000*75\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	".2687,E,0.14,146.26,230507,,*05\r\n"
	"$GPGGA,175252.253,4657.,00,18,28,298,00,12,12,218,36,22,09,326,00*7E\r\n"
	"$GPRMC,175252.253,A,4657.3419,N,04,9.0,567.9,M,48.0,M,0.0,0000*7A\r\n"
	"$GPGSA,A,3,26,29,17,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,175253.253,A,4657.$GPGGA,175254.253,4657.3420,N,00726.2687,E,1,0417,12,,,,,,,,,9.5,9.0,2.6*39\r\n"
	"$GPRMC,175254.253,A,4657.3420,N,00726.26$GPGGA,175255.253,4657.3421,N,00726.2688,E,1,04,9.0,568.4,M,48.0,M,0.0,0000*7A\r\n"
	"$GPGSA,A,,E,0.16,151.85,230507,,*09\r\n"
};
#endif


/*************************************************************************/

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
	error("openPort: error open");
	return fd;
    }

    /* flush serial port */
    status = tcflush(fd, TCIFLUSH);
    if (status < 0) {
	error("openPort: error tcflush");
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
	//error("tcsetattr() failed: %s", strerror(errno));
	error("tcsetattr() failed: __ERRNO removed due lazy coder");
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
    
    if (debug==1)
        debug("gps:debug: get gga callout, msg nr: %d\n",msgCounter);
    
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
    
    if (debug==1)
        debug("gps:debug: get rmc callout, msg nr: %d\n",msgCounter);
    
}


static int prepare_gps_parser()
{
    int status;
    char *port = "/dev/usb/tts/1";
    char *test;
    int speed = 0;		// 0 = default 4800 baud, 1 is 9600 baud

    if ((test = getenv("GPS_PORT"))) {	/* define your port via env variable */
	port = test;
    }

    if ((test = getenv("GPS_9600"))) {	/* define your port via env variable */
	speed = 1;
    }

    /* --------------------------------------- */
    /* open the serial port device             */
    /* using default 4800 baud for most GPS    */
    /* --------------------------------------- */
#ifndef EMULATE
    if (speed == 0)
	fd_g = openPort(port, B4800);
    else
	fd_g = openPort(port, B9600);

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
    int rem;
    int offset;
    int status;
    int len;

    long options;
    long dispOptions;
#define BUFFER_SIZE 128    
    char buffer[BUFFER_SIZE];

    options = R2N(theOptions);
    dispOptions = R2N(displayOptions);
    //error("options: %x\n",options);
    
    if (dispOptions & OPTION_DEBUG)
	debug=1;	    
    
    if ((dispOptions & OPTION_GET_BUFFERDATA) == 0) {

	/* ---------------------------------------- */
	/* STEP 6 : get a buffer of input          */
	/* --------------------------------------- */
#ifdef EMULATE
	len = rem = EMU_BUFFER_READ_SIZE;
	emu_read_ofs += EMU_BUFFER_READ_SIZE;
	if (emu_read_ofs > (sizeof(test_vector)-BUFFER_SIZE))
	    emu_read_ofs = 0;
	memcpy(buffer, &test_vector[emu_read_ofs], BUFFER_SIZE);
#else
	memset(buffer, 0, sizeof(buffer));
	len = rem = read(fd_g, buffer, sizeof(buffer));
	if (len <= 0) {
	    error("GPS Plugin, Error read from port, try using the GPS_PORT env variable (export GPS_PORT=/dev/mydev)");
	    //break;
	}
	if (debug==1)
	    debug("gps:debug: read %d bytes\n",len);
	
#endif
	if (dispOptions & OPTION_RAW_NMEA)
	    printf("\n__[%s]", buffer + '\0');

	/* ---------------------------------------------- */
	/* STEP 7 : process input until buffer is used up */
	/* ---------------------------------------------- */
//#ifdef EMULATE
//	offset = emu_read_ofs;
//#else
	offset = 0;
//#endif

	while (rem > 0) {
//#ifdef EMULATE
//	    status = nmeap_parseBuffer(&nmea, &test_vector[offset], &rem);
//#else
	    status = nmeap_parseBuffer(&nmea, &buffer[offset], &rem);
	    debug("\nGPS::debug: remaining: %d bytes\n",rem);
	    //error("loop, remaining=%d, read bytes=%d\n",rem,offset);        
//#endif
	    offset += (len - rem);
	}
    }				// end of OPTION get bufferdata


    /* --------------------------------------- */
    /* DISPLAY stuff comes here...             */
    /* --------------------------------------- */
    char *value = " ";
    char outputStr[80];
    memset(outputStr, 0, 80);

    if (options & SHOW_ALTITUDE) {
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%.0f ", outputStr, altitude);
	else
	    sprintf(outputStr, "%salt:%.0f ", outputStr, altitude);
    }
    if (options & SHOW_SPEED) {
	float knotsConvert = 1.852f;	//default speed display=km/h
	if (dispOptions & OPTION_SPEED_IN_KNOTS)
	    knotsConvert = 1.0f;	//use knots

	if (dispOptions & OPTION_NO_PREFIX)
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
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%s ", outputStr, courses[selectedDegree]);
	else
	    sprintf(outputStr, "%sdir:%s ", outputStr, courses[selectedDegree]);
    }
    if (options & SHOW_SATELLITES) {
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%d ", outputStr, satellites);
	else
	    sprintf(outputStr, "%ssat:%d ", outputStr, satellites);
    }
    if (options & SHOW_QUALITY) {
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%d ", outputStr, quality);
	else
	    sprintf(outputStr, "%squa:%d ", outputStr, quality);
    }
    if (options & SHOW_STATUS) {
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%c ", outputStr, gpsStatus);
	else
	    sprintf(outputStr, "%ssta:%c ", outputStr, gpsStatus);
    }
    if (options & SHOW_TIME_UTC) {
	char digitizer[9];	//01:34:67
	sprintf(digitizer, "%.6ld", gpsTime);	//<012345>
	digitizer[7] = digitizer[5];
	digitizer[6] = digitizer[4];
	digitizer[4] = digitizer[3];
	digitizer[3] = digitizer[2];
	digitizer[2] = ':';
	digitizer[5] = ':';
	digitizer[8] = '\0';
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%s ", outputStr, digitizer);
	else
	    sprintf(outputStr, "%sutc:%s ", outputStr, digitizer);
    }
    if (options & SHOW_DATE) {
	char digitizer[9];	//01:34:67
	sprintf(digitizer, "%.6ld", gpsDate);	//<012345>
	digitizer[7] = digitizer[5];
	digitizer[6] = digitizer[4];
	digitizer[4] = digitizer[3];
	digitizer[3] = digitizer[2];
	digitizer[2] = '/';
	digitizer[5] = '/';
	digitizer[8] = '\0';
	if (dispOptions & OPTION_NO_PREFIX)
	    sprintf(outputStr, "%s%s ", outputStr, digitizer);
	else
	    sprintf(outputStr, "%sdat:%s ", outputStr, digitizer);
    }

    if (options == 0) {		//error, no parameter defined!
	error("gps::parse() ERROR, no parameter specified!");
	value = strdup("GPS ARG ERR");
    } else {
	value = strdup(outputStr);
    }

    SetResult(&result, R_STRING, value);
    free(value);
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_gps(void)
{
    info("%s: v%s", Name, "0.2");
    prepare_gps_parser();

    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("gps::parse", 2, parse);

    return 0;
}

void plugin_exit_gps(void)
{
    info("%s: shutting down plugin.", Name);
#ifndef EMULATE
    close(fd_g);
#endif
}
