/* $Id$
 * $URL$
 *
 * driver for Logic Way GmbH ABP08 (serial) and ABP09 (USB) appliance control panels
 * http://www.logicway.de/pages/mde-hardware.shtml#ABP.AMC
 *
 * Copyright (C) 2009 Arndt Kritzner <kritzner@logicway.de>
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_LW_ABP
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "timer.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "widget_keypad.h"
#include "drv.h"

/* text mode display? */
#include "drv_generic_text.h"
#include "drv_generic_keypad.h"

/* serial port? */
#include "drv_generic_serial.h"

/* i2c bus? */
#ifdef WITH_I2C
#include "drv_generic_i2c.h"
#endif

#define KEY_UP 4
#define KEY_DOWN 3
#define KEY_LEFT 2
#define KEY_RIGHT 1

static char Name[] = "LW_ABP";
static char logicway[] = "www.logicway.de";

/* ring buffer for bytes received from the display */
static unsigned char RingBuffer[256];
static unsigned int RingRPos = 0;
static unsigned int RingWPos = 0;

static int button = -1;
static char *colors[] =
    { "r 0", "r 1", "g 0", "g 1", "b 0", "b 1", "c 0", "c 1", "m 0", "m 1", "y 0", "y 1", "w 0", "w 1", NULL };

#define TIMESYNC_FIRST  600
#define TIMESYNC_INTERVAL 24*3600
static time_t next_timesync;

static char *switchcontact = NULL;
static int statresult = 0;
static time_t switchcontact_lastchange = 0;
static time_t switchcontact_time = 0;
#define SWITCH_ACTION_DURATION 1000

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_LW_ABP_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_LW_ABP_close(void)
{
    drv_generic_serial_close();

    return 0;
}


/* dummy function that sends something to the display */
static void drv_LW_ABP_send(const char *data, const unsigned int len)
{
    /* send data to the serial port is easy... */
    drv_generic_serial_write(data, len);
}


/* text mode displays only */
static void drv_LW_ABP_reset(void)
{
    char cmd[] = "lcd init\r\n";

    /* do whatever is necessary to initialize the display */
    drv_LW_ABP_send(cmd, strlen(cmd));
}

static void drv_LW_ABP_clear(void)
{
    char cmd[] = "lcd clear\r\n";

    /* do whatever is necessary to clear the display */
    drv_LW_ABP_send(cmd, strlen(cmd));
}

static int drv_LW_ABP_time(unsigned char force)
{
    char cmd[] = "date set ";
    char command[50];
    time_t t = time(NULL);

    if (force || (t > next_timesync)) {
	/* do whatever is necessary to set clock on the display */
	sprintf(command, "%s%lu\r\n", cmd, (unsigned long) t);
	drv_LW_ABP_send(command, strlen(command));
	next_timesync = t + TIMESYNC_INTERVAL;
	info("%s: synced time to %lu, next is %lu\n", Name, (unsigned long) t, (unsigned long) next_timesync);
	return 1;
    }

    return 0;
}

static int drv_LW_ABP_switchcontact(void)
{
    char cmd[] = "switch time ";
    char command[50];
    struct stat statinfo;
    FILE *configfile;
    int scanresult;
    int config_entries;
    char inputline[101];
    if ((switchcontact != NULL) && (*switchcontact != '\0')) {
	if (stat(switchcontact, &statinfo) == -1) {
	    if (statresult != -1) {
		error("%s: specified switch contact control file %s not found", Name, switchcontact);
		switchcontact_time = 0;
		sprintf(command, "%s%lu %u\r\n", cmd, (unsigned long) switchcontact_time, SWITCH_ACTION_DURATION);
		drv_LW_ABP_send(command, strlen(command));
		info("%s: switchtime deactivated", Name);
		statresult = -1;
	    }
	} else {
	    if (statresult == -1) {
		info("%s: specified switch contact control file %s appeared", Name, switchcontact);
		statresult = 0;
	    }
	    if ((switchcontact_lastchange == 0) || (switchcontact_lastchange < statinfo.st_mtime)) {
		info("%s: specified switch contact control file %s has changed (%lu < %lu)", Name, switchcontact,
		     (unsigned long) switchcontact_lastchange, (unsigned long) statinfo.st_mtime);
		switchcontact_lastchange = statinfo.st_mtime;
		configfile = fopen(switchcontact, "r");
		if (configfile == NULL) {
		    error("%s: could not open specified switch contact control file %s", Name, switchcontact);
		} else {
		    config_entries = 0;
		    while (!feof(configfile)) {
			if (fgets(inputline, 100, configfile) != NULL) {
			    if ((scanresult = sscanf(inputline, "switchtime=%lu", &switchcontact_time)) == 1) {
				info("%s: switchtime read %lu (file %s)", Name, (unsigned long) switchcontact_time,
				     switchcontact);
				config_entries++;
			    }
			}
		    }
		    fclose(configfile);
		    if (config_entries > 0) {
			if (switchcontact_time < time(NULL)) {
			    switchcontact_time = 0;
			    info("%s: switchtime is in past %lu (now %lu), so deactivated", Name,
				 (unsigned long) switchcontact_time, time(NULL));
			}
		    } else {
			switchcontact_time = 0;
		    }
		    if (switchcontact_time != 0) {
			sprintf(command, "%s%lu %u\r\n", cmd, (unsigned long) switchcontact_time,
				SWITCH_ACTION_DURATION);
			drv_LW_ABP_send(command, strlen(command));
			info("%s: switchtime set to %lu", Name, (unsigned long) switchcontact_time);
		    } else {
			sprintf(command, "%s%lu %u\r\n", cmd, (unsigned long) switchcontact_time,
				SWITCH_ACTION_DURATION);
			drv_LW_ABP_send(command, strlen(command));
			info("%s: switchtime deactivated", Name);
		    }
		}
	    }
	}
    }

    return 0;
}

/* text mode displays only */
static void drv_LW_ABP_write(const int row, const int col, const char *data, int len)
{
    char cmd[] = "lcd set line ";
    char row_[5];
    char col_[5];

    /* do the cursor positioning here */
    drv_LW_ABP_send(cmd, strlen(cmd));
    sprintf(row_, "%d", row + 1);
    drv_LW_ABP_send(row_, strlen(row_));
    if (col > 0) {
	drv_LW_ABP_send(",", 1);
	sprintf(col_, "%d", col + 1);
	drv_LW_ABP_send(col_, strlen(col_));
    }
    drv_LW_ABP_send(" ", 1);

    /* send string to the display */
    drv_LW_ABP_send(data, len);
    drv_LW_ABP_send("\r\n", 2);

}

static unsigned char byte(int pos)
{
    if (pos >= 0) {
	pos += RingRPos;
	if ((unsigned) pos >= sizeof(RingBuffer))
	    pos -= sizeof(RingBuffer);
    } else {
	pos += RingWPos;
	if (pos < 0)
	    pos += sizeof(RingBuffer);
    }
    return RingBuffer[pos];
}

static void drv_LW_ABP_process_button(void)
{
    /* Key Activity */
    debug("%s: Key Activity: %d", Name, button);
    drv_generic_keypad_press(button);
}

static int drv_LW_ABP_poll(void)
{
    char received[100];

    /* read into RingBuffer */
    while (1) {
	char buffer[32];
	int num, n;
	num = drv_generic_serial_poll(buffer, sizeof(buffer));
	if (num <= 0)
	    break;
	/* put result into RingBuffer */
	for (n = 0; n < num; n++) {
	    RingBuffer[RingWPos++] = (unsigned char) buffer[n];
	    if (RingWPos >= sizeof(RingBuffer))
		RingWPos = 0;
	}
    }

    received[0] = '\0';

    /* process RingBuffer */
    while (1) {
	char command[32];
	int n, num;
	/* packet size */
	num = RingWPos - RingRPos;
	if (num < 0)
	    num += sizeof(RingBuffer);
	/* minimum packet size=3 */
	if (num < 1) {
	    if (strlen(received) > 0) {
		debug("%s: received: %s", Name, received);
	    }
	    return 0;
	}
	if (byte(0) != '[') {
	    goto GARBAGE;
	}
	for (n = 0; (n < num) && ((unsigned) n < (sizeof(command) - 1)); n++) {
	    command[n] = byte(n);
	    if (command[n] == ']') {
		n++;
		break;
	    }
	}
	command[n] = '\0';
	if (command[n - 1] != ']') {
	    if (strlen(command) < 4) {
		if (strlen(received) > 0) {
		    debug("%s: received: %s", Name, received);
		}
		return 0;
	    }
	    goto GARBAGE;
	}
	info("%s: command read from keypad: %s\n", Name, command);
	if (sscanf(command, "[T%d]", &button) == 1) {
	    info("%s: button %d pressed\n", Name, button);
	} else {
	    goto GARBAGE;
	}
	/* increment read pointer */
	RingRPos += strlen(command);
	if (RingRPos >= sizeof(RingBuffer))
	    RingRPos -= sizeof(RingBuffer);
	/* a packet arrived */
	if (strlen(received) > 0) {
	    debug("%s: received: %s", Name, received);
	}
	return 1;
      GARBAGE:
	switch (byte(0)) {
	case '\n':
	case '\r':
	case '>':
	    if (strlen(received) > 0) {
		debug("%s: received: %s", Name, received);
		received[0] = '\0';
	    }
	    break;
	default:
	    if (byte(0) < ' ')
		debug("%s: dropping garbage byte %02x", Name, byte(0));
	    else if ((strlen(received) + 2) > sizeof(received)) {
		debug("%s: received: %s", Name, received);
		received[0] = '\0';
	    }
	    sprintf(received, "%s%c", received, byte(0));
	    break;
	}
	RingRPos++;
	if (RingRPos >= sizeof(RingBuffer))
	    RingRPos = 0;
	continue;
    }

    /* not reached */
    return 0;
}

static void drv_LW_ABP_timer(void __attribute__ ((unused)) * notused)
{
    while (drv_LW_ABP_poll()) {
	drv_LW_ABP_process_button();
    }

    drv_LW_ABP_time(0);
    drv_LW_ABP_switchcontact();
}

static int drv_LW_ABP_keypad(const int num)
{
    int val = 0;

    switch (num) {
    case KEY_UP:
	debug("%s: Key Up", Name);
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_UP;
	break;
    case KEY_DOWN:
	debug("%s: Key Down", Name);
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_DOWN;
	break;
    case KEY_LEFT:
	debug("%s: Key Left / Cancel", Name);
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_LEFT;
//      val += WIDGET_KEY_CANCEL;
	break;
    case KEY_RIGHT:
	debug("%s: Key Right / Confirm", Name);
	val += WIDGET_KEY_PRESSED;
	val += WIDGET_KEY_RIGHT;
//      val += WIDGET_KEY_CONFIRM;
	break;
    default:
	debug("%s: Unbound Key '%d'", Name, num);
	break;
    }

    return val;
}

static char *drv_LW_ABP_background(char *color)
{
    static char *Background = NULL;
    char cmd[] = "lcd set color ";
    int idx;
    if (color != NULL) {
	for (idx = 0; colors[idx] != NULL; idx++) {
	    if (strcmp(color, colors[idx]) == 0)
		break;
	}
	if (colors[idx] != NULL) {
	    Background = colors[idx];
	    debug("%s: set background color %s", Name, Background);
	    drv_LW_ABP_send(cmd, strlen(cmd));
	    drv_LW_ABP_send(color, strlen(color));
	    drv_LW_ABP_send("\r\n", 2);
	}
    }
    return Background;
}

static int drv_LW_ABP_contrast(int contrast)
{
    return contrast;
}


/* start text mode display */
static int drv_LW_ABP_start(const char *section)
{
    int contrast;
    int rows = -1, cols = -1;
    char *s;
    char *background;

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

    next_timesync = time(NULL) + TIMESYNC_FIRST;

    /* open communication with the display */
    if (drv_LW_ABP_open(section) < 0) {
	return -1;
    }

    drv_LW_ABP_reset();		/* initialize display */

    background = cfg_get(section, "Background", NULL);
    if ((background != NULL) && (*background != '\0')) {
	if (drv_LW_ABP_background(background) == NULL) {
	    debug("%s: wrong background color specified: %s", Name, background);
	}
    }

    switchcontact = cfg_get(section, "Switchcontact", NULL);

    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_LW_ABP_contrast(contrast);
    }

    drv_LW_ABP_clear();		/* clear display */

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_LW_ABP_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}

static void plugin_background(RESULT * result, const int argc, RESULT * argv[])
{
    char *color;

    switch (argc) {
    case 0:
	color = drv_LW_ABP_background(NULL);
	SetResult(&result, R_STRING, &color);
	break;
    case 1:
	color = drv_LW_ABP_background(R2S(argv[0]));
	SetResult(&result, R_STRING, &color);
	break;
    default:
	error("%s.backlight(): wrong number of parameters (%d)", Name, argc);
	SetResult(&result, R_STRING, "");
    }
}


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_gpio_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_LW_ABP_list(void)
{
    printf("Logic Way ABP driver");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_LW_ABP_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 10;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_LW_ABP_write;
    drv_generic_keypad_real_press = drv_LW_ABP_keypad;

    /* regularly process display answers */
    timer_add(drv_LW_ABP_timer, NULL, 100, 0);

    /* start display */
    if ((ret = drv_LW_ABP_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, logicway)) {
	    sleep(3);
	    drv_LW_ABP_clear();
	}
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic icon driver */
    if ((ret = drv_generic_text_icon_init()) != 0)
	return ret;

    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(0)) != 0)
	return ret;

    /* initialize generic key pad driver */
    if ((ret = drv_generic_keypad_init(section, Name)) != 0)
	return ret;

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);

    /* register plugins */
    AddFunction("LCD::contrast", 1, plugin_contrast);
    AddFunction("LCD::background", -1, plugin_background);

    return 0;
}



/* close driver & display */
/* use this function for a text display */
int drv_LW_ABP_quit(const int quiet)
{
    char switchactive[] = "switch scheduled";
    char timeinfo[17];
    struct tm switchtime_info;

    info("%s: shutting down.", Name);

    drv_generic_text_quit();
    drv_generic_keypad_quit();

    /* clear display */
    drv_LW_ABP_clear();

    /* say goodbye... */
    if (switchcontact_time < time(NULL)) {
	switchcontact_time = 0;
	info("%s: switchtime is in past %lu (now %lu), so deactivated", Name, (unsigned long) switchcontact_time,
	     time(NULL));
    }
    if (switchcontact_time == 0) {
	if (!quiet) {
	    drv_generic_text_greet("goodbye!", NULL);
	}
    } else {
	drv_LW_ABP_write(0, 0, logicway, strlen(logicway));
	drv_LW_ABP_write(1, 0, switchactive, strlen(switchactive));
	localtime_r(&switchcontact_time, &switchtime_info);
	if (strftime(timeinfo, sizeof(timeinfo), "for %a %X", &switchtime_info) > 0) {
	    drv_LW_ABP_write(2, 0, timeinfo, strlen(timeinfo));
	}
    }

    debug("closing connection");
    drv_LW_ABP_close();

    return (0);
}


/* use this one for a text display */
DRIVER drv_LW_ABP = {
    .name = Name,
    .list = drv_LW_ABP_list,
    .init = drv_LW_ABP_init,
    .quit = drv_LW_ABP_quit,
};
