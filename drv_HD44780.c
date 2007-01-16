/* $Id$
 * $URL$
 *
 * new style driver for HD44780-based displays
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Support for I2C bus
 * Copyright (C) 2005 Luis Correia <lfcorreia@users.sf.net>
 *
 * Modification for 4-Bit mode
 * Copyright (C) 2003 Martin Hejl (martin@hejl.de)
 *
 * Modification for 2nd controller support
 * Copyright (C) 2003 Jesse Brook Kovach <jkovach@wam.umd.edu>
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
 * struct DRIVER drv_HD44780
 *
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
#include <sys/ioctl.h>

#include "debug.h"
#include "cfg.h"
#include "udelay.h"
#include "qprintf.h"
#include "timer.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_gpio.h"

#ifdef WITH_PARPORT
#include "drv_generic_parport.h"
#endif

#ifdef WITH_I2C
#include "drv_generic_i2c.h"
#endif

static char Name[] = "HD44780";

static int Bus;
static int Model;
static int Capabilities;


/* Timings */
#ifdef WITH_PARPORT
static int T_CY, T_PW, T_AS, T_AH;
#endif
static int T_INIT1, T_INIT2, T_EXEC, T_WRCG, T_CLEAR, T_HOME, T_ONOFF;
#ifdef WITH_PARPORT
static int T_POWER, T_GPO_ST, T_GPO_PW;
#endif

static int Bits = 0;
static int numControllers = 0;
static int allControllers = 0;
static int currController = 0;

/* size of every single controller */
static int CROWS[4];
static int CCOLS[4];

static unsigned char SIGNAL_RW;
static unsigned char SIGNAL_RS;
static unsigned char SIGNAL_ENABLE;
static unsigned char SIGNAL_ENABLE2;
static unsigned char SIGNAL_ENABLE3;
static unsigned char SIGNAL_ENABLE4;

static unsigned char SIGNAL_GPO;
#ifdef WITH_PARPORT
static unsigned char SIGNAL_BACKLIGHT;
static unsigned char SIGNAL_POWER;
#endif

/* maximum time to wait for the busy-flag (in usec) */
#define MAX_BUSYFLAG_WAIT 10000

/* maximum busy flag errors before falling back to busy-waiting */
#define MAX_BUSYFLAG_ERRORS 20

/* flag for busy-waiting vs. busy flag checking */
#ifdef WITH_PARPORT
static int UseBusy = 0;
#endif

/* buffer holding the GPO state */
#ifdef WITH_PARPORT
static unsigned char GPO = 0;
#endif

typedef struct {
    int type;
    char *name;
    int capabilities;
} MODEL;

#define CAP_PARPORT    (1<<0)
#define CAP_I2C        (1<<1)
#define CAP_GPO        (1<<2)
#define CAP_BACKLIGHT  (1<<3)
#define CAP_BRIGHTNESS (1<<4)
#define CAP_BUSY4BIT   (1<<5)
#define CAP_HD66712    (1<<6)
#define CAP_LCM162     (1<<7)

#define BUS_PP  CAP_PARPORT
#define BUS_I2C CAP_I2C


static MODEL Models[] = {
    {0x01, "generic", CAP_PARPORT | CAP_I2C | CAP_GPO | CAP_BACKLIGHT},
    {0x02, "Noritake", CAP_PARPORT | CAP_I2C | CAP_GPO | CAP_BRIGHTNESS},
    {0x03, "Soekris", CAP_PARPORT | CAP_BUSY4BIT},
    {0x04, "HD66712", CAP_PARPORT | CAP_I2C | CAP_GPO | CAP_BACKLIGHT | CAP_HD66712},
    {0x05, "LCM-162", CAP_PARPORT | CAP_LCM162},
    {0xff, "Unknown", 0}
};


/****************************************/
/***  generic functions               ***/
/****************************************/

static int (*drv_HD_load) (const char *section);
static void (*drv_HD_command) (const unsigned char controller, const unsigned char cmd, const unsigned long delay);
static void (*drv_HD_data) (const unsigned char controller, const char *string, const int len,
			    const unsigned long delay);
static void (*drv_HD_stop) (void);



/****************************************/
/***  parport dependant functions     ***/
/****************************************/

#ifdef WITH_PARPORT

static void drv_HD_PP_busy(const int controller)
{
    static unsigned int errors = 0;

    unsigned char enable;
    unsigned char data = 0xFF;
    unsigned char busymask;
    unsigned char ctrlmask;
    unsigned int counter;

    if (Bits == 8) {
	busymask = 0x80;
    } else {
	/* Since in 4-Bit mode DB0 on the parport is mapped to DB4 on the LCD
	 * (and consequently, DB3 on the partport is mapped to DB7 on the LCD)
	 * we need to listen for DB3 on the parport to go low
	 */
	busymask = 0x08;
    }

    ctrlmask = 0x08;
    while (ctrlmask > 0) {

	if (controller & ctrlmask) {

	    enable = 0;
	    if (ctrlmask & 0x01)
		enable = SIGNAL_ENABLE;
	    else if (ctrlmask & 0x02)
		enable = SIGNAL_ENABLE2;
	    else if (ctrlmask & 0x04)
		enable = SIGNAL_ENABLE3;
	    else if (ctrlmask & 0x08)
		enable = SIGNAL_ENABLE4;

	    /* set data-lines to input */
	    drv_generic_parport_direction(1);

	    if (Bits == 8) {
		/* Set RW, clear RS */
		drv_generic_parport_control(SIGNAL_RW | SIGNAL_RS, SIGNAL_RW);
	    } else {
		drv_generic_parport_data(SIGNAL_RW);
	    }

	    /* Address set-up time */
	    ndelay(T_AS);

	    /* rise ENABLE */
	    if (Bits == 8) {
		drv_generic_parport_control(enable, enable);
	    } else {
		drv_generic_parport_data(SIGNAL_RW | enable);
	    }

	    counter = 0;
	    while (1) {
		/* read the busy flag */
		data = drv_generic_parport_read();
		if ((data & busymask) == 0) {
		    errors = 0;
		    break;
		}

		/* make sure we don't wait forever
		 * - but only check after 5 iterations
		 * that way, we won't slow down normal mode
		 * (where we don't need the timeout anyway) 
		 */
		counter++;

		if (counter >= 5) {
		    struct timeval now, end;

		    if (counter == 5) {
			/* determine the time when the timeout has expired */
			gettimeofday(&end, NULL);
			end.tv_usec += MAX_BUSYFLAG_WAIT;
			while (end.tv_usec > 1000000) {
			    end.tv_usec -= 1000000;
			    end.tv_sec++;
			}
		    }

		    /* get the current time */
		    gettimeofday(&now, NULL);
		    if (now.tv_sec == end.tv_sec ? now.tv_usec >= end.tv_usec : now.tv_sec >= end.tv_sec) {
			error("%s: timeout waiting for busy flag on controller %x (0x%02x)", Name, ctrlmask, data);
			if (++errors >= MAX_BUSYFLAG_ERRORS) {
			    error("%s: too many busy flag failures, turning off busy flag checking.", Name);
			    UseBusy = 0;
			}
			break;
		    }
		}
	    }

	    /* RS=low, RW=low, EN=low */
	    if (Bits == 8) {
		/* Lower EN */
		drv_generic_parport_control(enable, 0);

		/* Address hold time */
		ndelay(T_AH);

		drv_generic_parport_control(SIGNAL_RW | SIGNAL_RS, 0);
	    } else {
		/* Lower EN */
		drv_generic_parport_data(SIGNAL_RW);
		ndelay(T_AH);
		drv_generic_parport_data(0);
	    }

	    /* set data-lines to output */
	    drv_generic_parport_direction(0);
	}
	ctrlmask >>= 1;
    }
}


static void drv_HD_PP_nibble(const unsigned char controller, const unsigned char nibble)
{
    unsigned char enable;

    /* enable signal: 'controller' is a bitmask */
    /* bit n .. send to controller #n */
    /* so we can send a byte to more controllers at the same time! */
    enable = 0;
    if (controller & 0x01)
	enable |= SIGNAL_ENABLE;
    if (controller & 0x02)
	enable |= SIGNAL_ENABLE2;
    if (controller & 0x04)
	enable |= SIGNAL_ENABLE3;
    if (controller & 0x08)
	enable |= SIGNAL_ENABLE4;

    /* clear ENABLE */
    /* put data on DB1..DB4 */
    /* nibble already contains RS bit! */
    drv_generic_parport_data(nibble);

    /* Address set-up time */
    ndelay(T_AS);

    /* rise ENABLE */
    drv_generic_parport_data(nibble | enable);

    /* Enable pulse width */
    ndelay(T_PW);

    /* lower ENABLE */
    drv_generic_parport_data(nibble);
}


static void drv_HD_PP_byte(const unsigned char controller, const unsigned char data, const unsigned char RS)
{
    /* send high nibble of the data */
    drv_HD_PP_nibble(controller, ((data >> 4) & 0x0f) | RS);

    /* Make sure we honour T_CY */
    ndelay(T_CY - T_AS - T_PW);

    /* send low nibble of the data */
    drv_HD_PP_nibble(controller, (data & 0x0f) | RS);
}


static void drv_HD_PP_command(const unsigned char controller, const unsigned char cmd, const unsigned long delay)
{
    unsigned char enable;

    if (UseBusy)
	drv_HD_PP_busy(controller);

    if (Bits == 8) {

	/* enable signal: 'controller' is a bitmask */
	/* bit n .. send to controller #n */
	/* so we can send a byte to more controllers at the same time! */
	enable = 0;
	if (controller & 0x01)
	    enable |= SIGNAL_ENABLE;
	if (controller & 0x02)
	    enable |= SIGNAL_ENABLE2;
	if (controller & 0x04)
	    enable |= SIGNAL_ENABLE3;
	if (controller & 0x08)
	    enable |= SIGNAL_ENABLE4;

	/* put data on DB1..DB8 */
	drv_generic_parport_data(cmd);

	/* clear RW and RS */
	drv_generic_parport_control(SIGNAL_RW | SIGNAL_RS, 0);

	/* Address set-up time */
	ndelay(T_AS);

	/* send command */
	drv_generic_parport_toggle(enable, 1, T_PW);

    } else {

	drv_HD_PP_byte(controller, cmd, 0);

    }

    /* wait for command completion */
    if (!UseBusy)
	udelay(delay);

}


static void drv_HD_PP_data(const unsigned char controller, const char *string, const int len, const unsigned long delay)
{
    int l = len;
    unsigned char enable;

    /* sanity check */
    if (len <= 0)
	return;

    if (Bits == 8) {

	/* enable signal: 'controller' is a bitmask */
	/* bit n .. send to controller #n */
	/* so we can send a byte to more controllers at the same time! */
	enable = 0;
	if (controller & 0x01)
	    enable |= SIGNAL_ENABLE;
	if (controller & 0x02)
	    enable |= SIGNAL_ENABLE2;
	if (controller & 0x04)
	    enable |= SIGNAL_ENABLE3;
	if (controller & 0x08)
	    enable |= SIGNAL_ENABLE4;

	if (!UseBusy) {
	    /* clear RW, set RS */
	    drv_generic_parport_control(SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);
	    /* Address set-up time */
	    ndelay(T_AS);
	}

	while (l--) {

	    if (UseBusy) {
		drv_HD_PP_busy(controller);
		/* clear RW, set RS */
		drv_generic_parport_control(SIGNAL_RW | SIGNAL_RS, SIGNAL_RS);
		/* Address set-up time */
		ndelay(T_AS);
	    }

	    /* put data on DB1..DB8 */
	    drv_generic_parport_data(*(string++));

	    /* send command */
	    drv_generic_parport_toggle(enable, 1, T_PW);

	    /* wait for command completion */
	    if (!UseBusy)
		udelay(delay);
	}

    } else {			/* 4 bit mode */

	while (l--) {
	    if (UseBusy)
		drv_HD_PP_busy(controller);

	    /* send data with RS enabled */
	    drv_HD_PP_byte(controller, *(string++), SIGNAL_RS);

	    /* wait for command completion */
	    if (!UseBusy)
		udelay(delay);
	}
    }
}


static int drv_HD_PP_load(const char *section)
{
    if (cfg_number(section, "Bits", 8, 4, 8, &Bits) < 0)
	return -1;

    if (Bits != 4 && Bits != 8) {
	error("%s: bad %s.Bits '%d' from %s, should be '4' or '8'", Name, section, Bits, cfg_source());
	return -1;
    }

    /* LCM-162 only supports 8-bit-mode */
    if (Capabilities & CAP_LCM162 && Bits != 8) {
	error("%s: Model '%s' does not support %d bit mode!", Name, Models[Model].name, Bits);
	Bits = 8;
    }
    info("%s: using %d bit mode", Name, Bits);

    if (drv_generic_parport_open(section, Name) != 0) {
	error("%s: could not initialize parallel port!", Name);
	return -1;
    }

    /* Soft-Wiring */
    if (Capabilities & CAP_LCM162) {
	/* the LCM-162 is hardwired */
	if ((SIGNAL_RS = drv_generic_parport_hardwire_ctrl("RS", "SLCTIN")) == 0xff)
	    return -1;
	if ((SIGNAL_RW = drv_generic_parport_hardwire_ctrl("RW", "INIT")) == 0xff)
	    return -1;
	if ((SIGNAL_ENABLE = drv_generic_parport_hardwire_ctrl("ENABLE", "AUTOFD")) == 0xff)
	    return -1;
	if ((SIGNAL_ENABLE2 = drv_generic_parport_hardwire_ctrl("ENABLE2", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_ENABLE3 = drv_generic_parport_hardwire_ctrl("ENABLE3", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_ENABLE4 = drv_generic_parport_hardwire_ctrl("ENABLE4", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_BACKLIGHT = drv_generic_parport_hardwire_ctrl("BACKLIGHT", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_GPO = drv_generic_parport_hardwire_ctrl("GPO", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_POWER = drv_generic_parport_hardwire_ctrl("POWER", "GND")) == 0xff)
	    return -1;
    } else {
	if (Bits == 8) {
	    if ((SIGNAL_RS = drv_generic_parport_wire_ctrl("RS", "AUTOFD")) == 0xff)
		return -1;
	    if ((SIGNAL_RW = drv_generic_parport_wire_ctrl("RW", "GND")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE = drv_generic_parport_wire_ctrl("ENABLE", "STROBE")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE2 = drv_generic_parport_wire_ctrl("ENABLE2", "GND")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE3 = drv_generic_parport_wire_ctrl("ENABLE3", "GND")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE4 = drv_generic_parport_wire_ctrl("ENABLE4", "GND")) == 0xff)
		return -1;
	} else {
	    if ((SIGNAL_RS = drv_generic_parport_wire_data("RS", "DB4")) == 0xff)
		return -1;
	    if ((SIGNAL_RW = drv_generic_parport_wire_data("RW", "DB5")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE = drv_generic_parport_wire_data("ENABLE", "DB6")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE2 = drv_generic_parport_wire_data("ENABLE2", "GND")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE3 = drv_generic_parport_wire_data("ENABLE3", "GND")) == 0xff)
		return -1;
	    if ((SIGNAL_ENABLE4 = drv_generic_parport_wire_data("ENABLE4", "GND")) == 0xff)
		return -1;
	}
	/* backlight GPO and power are always control signals */
	if ((SIGNAL_BACKLIGHT = drv_generic_parport_wire_ctrl("BACKLIGHT", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_GPO = drv_generic_parport_wire_ctrl("GPO", "GND")) == 0xff)
	    return -1;
	if ((SIGNAL_POWER = drv_generic_parport_wire_ctrl("POWER", "GND")) == 0xff)
	    return -1;
    }

    /* clear capabilities if corresponding signal is GND */
    if (SIGNAL_BACKLIGHT == 0) {
	Capabilities &= ~CAP_BACKLIGHT;
    }
    if (SIGNAL_GPO == 0) {
	Capabilities &= ~CAP_GPO;
    }

    /* Timings */

    /* low level communication timings [nanoseconds]
     * as these values differ from spec to spec,
     * we use the worst-case default values, but allow
     * modification from the config file.
     */
    T_CY = timing(Name, section, "CY", 1000, "ns");	/* Enable cycle time */
    T_PW = timing(Name, section, "PW", 450, "ns");	/* Enable pulse width */
    T_AS = timing(Name, section, "AS", 140, "ns");	/* Address setup time */
    T_AH = timing(Name, section, "AH", 20, "ns");	/* Address hold time */

    /* GPO timing */
    if (SIGNAL_GPO != 0) {
	T_GPO_ST = timing(Name, section, "GPO_ST", 20, "ns");	/* 74HCT573 set-up time */
	T_GPO_PW = timing(Name, section, "GPO_PW", 230, "ns");	/* 74HCT573 enable pulse width */
    } else {
	T_GPO_ST = 0;
	T_GPO_PW = 0;
    }

    /* HD44780 execution timings [microseconds]
     * as these values differ from spec to spec,
     * we use the worst-case default values, but allow
     * modification from the config file.
     */
    T_INIT1 = timing(Name, section, "INIT1", 4100, "us");	/* first init sequence: 4.1 msec */
    T_INIT2 = timing(Name, section, "INIT2", 100, "us");	/* second init sequence: 100 usec */
    T_EXEC = timing(Name, section, "EXEC", 80, "us");	/* normal execution time */
    T_WRCG = timing(Name, section, "WRCG", 120, "us");	/* CG RAM Write */
    T_CLEAR = timing(Name, section, "CLEAR", 2250, "us");	/* Clear Display */
    T_HOME = timing(Name, section, "HOME", 2250, "us");	/* Return Cursor Home */
    T_ONOFF = timing(Name, section, "ONOFF", 2250, "us");	/* Display On/Off Control */

    /* Power-on delay */
    if (SIGNAL_POWER != 0) {
	T_POWER = timing(Name, section, "POWER", 500, "ms");
    } else {
	T_POWER = 0;
    }

    /* clear all signals */
    if (Bits == 8) {
	drv_generic_parport_control(SIGNAL_RS | SIGNAL_RW |
				    SIGNAL_ENABLE | SIGNAL_ENABLE2 | SIGNAL_ENABLE3 | SIGNAL_ENABLE4 |
				    SIGNAL_BACKLIGHT | SIGNAL_GPO | SIGNAL_POWER, 0);
    } else {
	drv_generic_parport_control(SIGNAL_BACKLIGHT | SIGNAL_GPO | SIGNAL_POWER, 0);
	drv_generic_parport_data(0);
    }

    /* set direction: write */
    drv_generic_parport_direction(0);

    /* raise power pin */
    if (SIGNAL_POWER != 0) {
	drv_generic_parport_control(SIGNAL_POWER, SIGNAL_POWER);
	udelay(1000 * T_POWER);
    }

    /* initialize *all* controllers */
    if (Bits == 8) {
	drv_HD_PP_command(allControllers, 0x30, T_INIT1);	/* 8 Bit mode, wait 4.1 ms */
	drv_HD_PP_command(allControllers, 0x30, T_INIT2);	/* 8 Bit mode, wait 100 us */
	drv_HD_PP_command(allControllers, 0x38, T_EXEC);	/* 8 Bit mode, 1/16 duty cycle, 5x8 font */
    } else {
	drv_HD_PP_nibble(allControllers, 0x03);
	udelay(T_INIT1);	/* 4 Bit mode, wait 4.1 ms */
	drv_HD_PP_nibble(allControllers, 0x03);
	udelay(T_INIT2);	/* 4 Bit mode, wait 100 us */
	drv_HD_PP_nibble(allControllers, 0x03);
	udelay(T_INIT1);	/* 4 Bit mode, wait 4.1 ms */
	drv_HD_PP_nibble(allControllers, 0x02);
	udelay(T_INIT2);	/* 4 Bit mode, wait 100 us */
	drv_HD_PP_command(allControllers, 0x28, T_EXEC);	/* 4 Bit mode, 1/16 duty cycle, 5x8 font */
    }

    /* maybe use busy-flag from now on  */
    /* (we can't use the busy flag during the init sequence) */
    cfg_number(section, "UseBusy", 0, 0, 1, &UseBusy);

    /* make sure we don't use the busy flag with RW wired to GND */
    if (UseBusy && !SIGNAL_RW) {
	error("%s: busy-flag checking is impossible with RW wired to GND!", Name);
	UseBusy = 0;
    }

    /* make sure the display supports busy-flag checking in 4-Bit-Mode */
    /* at the moment this is inly possible with Martin Hejl's gpio driver, */
    /* which allows to use 4 bits as input and 4 bits as output */
    if (UseBusy && Bits == 4 && !(Capabilities & CAP_BUSY4BIT)) {
	error("%s: Model '%s' does not support busy-flag checking in 4 bit mode", Name, Models[Model].name);
	UseBusy = 0;
    }

    info("%s: %susing busy-flag checking", Name, UseBusy ? "" : "not ");

    /* The LCM-162 should really use BusyFlag checking */
    if (!UseBusy && (Capabilities & CAP_LCM162)) {
	error("%s: Model '%s' should definitely use busy-flag checking!", Name, Models[Model].name);
    }

    return 0;
}


static void drv_HD_PP_stop(void)
{
    /* clear all signals */
    if (Bits == 8) {
	drv_generic_parport_control(SIGNAL_RS |
				    SIGNAL_RW | SIGNAL_ENABLE | SIGNAL_ENABLE2 | SIGNAL_ENABLE3 | SIGNAL_ENABLE4 |
				    SIGNAL_BACKLIGHT | SIGNAL_GPO, 0);
    } else {
	drv_generic_parport_data(0);
	drv_generic_parport_control(SIGNAL_BACKLIGHT | SIGNAL_GPO | SIGNAL_POWER, 0);
    }

    drv_generic_parport_close();

}

#endif


#ifdef WITH_I2C

/****************************************/
/***  i2c dependant functions         ***/
/****************************************/

    /*
       DISCLAIMER!!!!

       The following code is WORK IN PROGRESS, since it basicly 'works for us...'

       (C) 2005 Paul Kamphuis & Luis Correia

       We have removed all of the delays from this code, as the I2C bus is slow enough...
       (maximum possible speed is 100KHz only)

     */

static void drv_HD_I2C_nibble(unsigned char controller, unsigned char nibble)
{
    unsigned char enable;
    unsigned char command;	/* this is actually the first data byte on the PCF8574 */
    unsigned char data_block[2];
    /* enable signal: 'controller' is a bitmask */
    /* bit n .. send to controller #n */
    /* so we can send a byte to more controllers at the same time! */
    enable = 0;
    if (controller & 0x01)
	enable |= SIGNAL_ENABLE;
    if (controller & 0x02)
	enable |= SIGNAL_ENABLE2;
    if (controller & 0x04)
	enable |= SIGNAL_ENABLE3;
    if (controller & 0x08)
	enable |= SIGNAL_ENABLE4;

    /*
       The new method Paul Kamphuis has concocted places the 3 needed writes to the I2C device
       as a single operation, using the 'i2c_smbus_write_block_data' function.
       These actual writes are performed by putting the nibble along with the 'EN' signal.

       command = first byte to be written, which contains the nibble (DB0..DB3)
       data [0]   = second byte to be written, which contains the nibble plus the EN signal
       data [1]   = third byte to be written, which contains the nibble (DB0..DB3)

       Then we write the block as a whole.

       The main advantage we see is that we do 2 less IOCTL's from our driver.
     */

    command = nibble;
    data_block[0] = nibble | enable;
    data_block[1] = nibble;

    drv_generic_i2c_command(command, data_block, 2);
}


static void drv_HD_I2C_byte(const unsigned char controller, const unsigned char data)
{
    /* send data with RS enabled */
    drv_HD_I2C_nibble(controller, ((data >> 4) & 0x0f) | SIGNAL_RS);
    drv_HD_I2C_nibble(controller, (data & 0x0f) | SIGNAL_RS);
}


static void drv_HD_I2C_command(const unsigned char controller, const unsigned char cmd, __attribute__ ((unused))
			       const unsigned long delay)
{
    /* send data with RS disabled */
    drv_HD_I2C_nibble(controller, ((cmd >> 4) & 0x0f));
    drv_HD_I2C_nibble(controller, ((cmd) & 0x0f));
}

static void drv_HD_I2C_data(const unsigned char controller, const char *string, const int len, __attribute__ ((unused))
			    const unsigned long delay)
{
    int l = len;

    /* sanity check */
    if (len <= 0)
	return;

    while (l--) {
	drv_HD_I2C_byte(controller, *(string++));
    }
}


static int drv_HD_I2C_load(const char *section)
{
    if (cfg_number(section, "Bits", 8, 4, 8, &Bits) < 0)
	return -1;
    if (Bits != 4) {
	error("%s: bad %s.Bits '%d' from %s, should be '4'", Name, section, Bits, cfg_source());
	return -1;
    }

    info("%s: using %d bit mode", Name, Bits);

    if (drv_generic_i2c_open(section, Name) != 0) {
	error("%s: could not initialize i2c attached device!", Name);
	return -1;
    }

    if ((SIGNAL_RS = drv_generic_i2c_wire("RS", "DB4")) == 0xff)
	return -1;
    if ((SIGNAL_RW = drv_generic_i2c_wire("RW", "DB5")) == 0xff)
	return -1;
    if ((SIGNAL_ENABLE = drv_generic_i2c_wire("ENABLE", "DB6")) == 0xff)
	return -1;
    if ((SIGNAL_ENABLE2 = drv_generic_i2c_wire("ENABLE2", "GND")) == 0xff)
	return -1;
    if ((SIGNAL_GPO = drv_generic_i2c_wire("GPO", "GND")) == 0xff)
	return -1;

    /* initialize display */
    drv_HD_I2C_nibble(allControllers, 0x03);
    udelay(T_INIT1);		/* 4 Bit mode, wait 4.1 ms */
    drv_HD_I2C_nibble(allControllers, 0x03);
    udelay(T_INIT2);		/* 4 Bit mode, wait 100 us */
    drv_HD_I2C_nibble(allControllers, 0x03);
    udelay(T_INIT1);		/* 4 Bit mode, wait 4.1 ms */
    drv_HD_I2C_nibble(allControllers, 0x02);
    udelay(T_INIT2);		/* 4 Bit mode, wait 100 us */
    drv_HD_I2C_command(allControllers, 0x28, T_EXEC);	/* 4 Bit mode, 1/16 duty cycle, 5x8 font */

    info("%s: I2C initialization done", Name);

    return 0;
}


static void drv_HD_I2C_stop(void)
{
    /* clear all signals */
    drv_generic_i2c_data(0);

    /* close port */
    drv_generic_i2c_close();
}

/* END OF DISCLAIMER */

#endif				/* WITH_I2C */


/****************************************/
/***  display dependant functions     ***/
/****************************************/

static void drv_HD_clear(void)
{
    drv_HD_command(allControllers, 0x01, T_CLEAR);	/* clear *all* displays */
}


static int drv_HD_goto(int row, int col)
{
    int pos, controller;

    /* handle multiple controllers */
    for (pos = 0, controller = 0; controller < numControllers; controller++) {
	pos += CROWS[controller];
	if (row < pos) {
	    currController = (1 << controller);
	    break;
	}
	row -= CROWS[controller];
    }

    /* column outside of current display's width */
    if (col >= CCOLS[controller])
	return -1;

    if (0) {
	debug("goto: [%d,%d] mask=%d, controller=%d, size:%dx%d", row, col, currController, controller,
	      CROWS[controller], CCOLS[controller]);
    }

    /* 16x1 Displays are organized as 8x2 :-( */
    if (CCOLS[controller] == 16 && CROWS[controller] == 1 && col > 7) {
	row++;
	col -= 8;
    }

    if (Capabilities & CAP_HD66712) {
	/* the HD66712 doesn't have a braindamadged RAM layout */
	pos = row * 32 + col;
    } else {
	/* 16x4 Controllers use a slightly different layout */
	if (CCOLS[controller] == 16 && CROWS[controller] == 4) {
	    pos = (row % 2) * 64 + (row / 2) * 16 + col;
	} else {
	    pos = (row % 2) * 64 + (row / 2) * 20 + col;
	}
    }
    drv_HD_command(currController, (0x80 | pos), T_EXEC);

    /* return columns left on current display */
    return CCOLS[controller] - col;

}


static void drv_HD_write(const int row, const int col, const char *data, const int len)
{
    int space = drv_HD_goto(row, col);
    if (space > 0) {
	drv_HD_data(currController, data, len > space ? space : len, T_EXEC);
    }
}


static void drv_HD_defchar(const int ascii, const unsigned char *matrix)
{
    int i;
    char buffer[8];

    for (i = 0; i < 8; i++) {
	buffer[i] = matrix[i] & 0x1f;
    }

    /* define chars on *all* controllers! */
    drv_HD_command(allControllers, 0x40 | 8 * ascii, T_EXEC);
    drv_HD_data(allControllers, buffer, 8, T_WRCG);
}


#ifdef WITH_PARPORT

static int drv_HD_backlight(int backlight)
{
    if (!(Capabilities & CAP_BACKLIGHT))
	return -1;

    if (backlight < 0)
	backlight = 0;
    if (backlight > 1)
	backlight = 1;

    drv_generic_parport_control(SIGNAL_BACKLIGHT, backlight ? SIGNAL_BACKLIGHT : 0);
    
    return backlight;
}

#endif


static int drv_HD_brightness(int brightness)
{
    char cmd;

    if (!(Capabilities & CAP_BRIGHTNESS))
	return -1;

    if (brightness < 0)
	brightness = 0;
    if (brightness > 3)
	brightness = 3;

    cmd = '0' + brightness;

    drv_HD_command(allControllers, 0x38, T_EXEC);	/* enable function */
    drv_HD_data(allControllers, &cmd, 1, T_WRCG);	/* set brightness */

    return brightness;
}


#ifdef WITH_PARPORT

static int drv_HD_GPO(const int num, const int val)
{
    int v;

    if (val > 0) {
	/* set bit */
	v = 1;
	GPO |= 1 << num;
    } else {
	/* clear bit */
	v = 0;
	GPO &= ~(1 << num);
    }

    /* put data on DB1..DB8 */
    drv_generic_parport_data(GPO);

    /* 74HCT573 set-up time */
    ndelay(T_GPO_ST);

    /* send data */
    /* 74HCT573 enable pulse width */
    drv_generic_parport_toggle(SIGNAL_GPO, 1, T_GPO_PW);

    return v;
}

#endif


#ifdef WITH_PARPORT

static void drv_HD_LCM162_timer(void __attribute__ ((unused)) * notused)
{
    static unsigned char data = 0x00;

    /* Bit 3+5 : key number */
    /* Bit 6   : key press/release */
    unsigned char mask3 = 1 << 3;
    unsigned char mask5 = 1 << 5;
    unsigned char mask6 = 1 << 6;
    unsigned char mask = mask3 | mask5 | mask6;

    int keynum;
    int updown;

    unsigned char temp;

    temp = drv_generic_parport_status() & mask;

    if (data != temp) {
	data = temp;

	keynum = (data & mask3 ? 1 : 0) + (data & mask5 ? 2 : 0);
	updown = (data & mask6 ? 1 : 0);

	debug("key %d press %d", keynum, updown);
    }
}

#endif


static int drv_HD_start(const char *section, const int quiet)
{
    char *model, *size, *bus;
    int rows = -1, cols = -1, gpos = -1, i;
    int size_defined = 0;
    int size_missing = 0;

    model = cfg_get(section, "Model", "generic");
    if (model != NULL && *model != '\0') {
	int i;
	for (i = 0; Models[i].type != 0xff; i++) {
	    if (strcasecmp(Models[i].name, model) == 0)
		break;
	}
	if (Models[i].type == 0xff) {
	    error("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
	    return -1;
	}
	Model = i;
	Capabilities = Models[Model].capabilities;
	info("%s: using model '%s'", Name, Models[Model].name);
    } else {
	error("%s: empty '%s.Model' entry from %s", Name, section, cfg_source());
	free(model);
	return -1;
    }
    free(model);

    bus = cfg_get(section, "Bus", "parport");
    if (bus == NULL && *bus == '\0') {
	error("%s: empty '%s.Bus' entry from %s", Name, section, cfg_source());
	free(bus);
	return -1;
    }

    if (strcasecmp(bus, "parport") == 0) {

#ifdef WITH_PARPORT
	info("%s: using parallel port", Name);
	Bus = BUS_PP;
	drv_HD_load = drv_HD_PP_load;
	drv_HD_command = drv_HD_PP_command;
	drv_HD_data = drv_HD_PP_data;
	drv_HD_stop = drv_HD_PP_stop;
#else
	error("%s: %s.Bus '%s' from %s not available:", Name, section, bus, cfg_source());
	error("%s: lcd4linux was compiled without parport support!", Name);
	free(bus);
	return -1;
#endif

    } else if (strcasecmp(bus, "i2c") == 0) {

#ifdef WITH_I2C
	info("%s: using I2C bus", Name);
	Bus = BUS_I2C;
	drv_HD_load = drv_HD_I2C_load;
	drv_HD_command = drv_HD_I2C_command;
	drv_HD_data = drv_HD_I2C_data;
	drv_HD_stop = drv_HD_I2C_stop;
#else
	error("%s: %s.Bus '%s' from %s not available:", Name, section, bus, cfg_source());
	error("%s: lcd4linux was compiled without i2c support!", Name);
	free(bus);
	return -1;
#endif

    } else {
	error("%s: bad %s.Bus '%s' from %s, should be 'parport' or 'i2c'", Name, section, bus, cfg_source());
	free(bus);
	return -1;
    }

    /* sanity check: Model can use bus */
    if (!(Capabilities & Bus)) {
	error("%s: Model '%s' cannot be used on the %s bus!", Name, Models[Model].name, bus);
	free(bus);
	return -1;
    }
    free(bus);

    if (cfg_number(section, "Controllers", 1, 1, 4, (int *) &numControllers) < 0)
	return -1;
    info("%s: using %d Controller(s)", Name, numControllers);

    /* current Controller */
    currController = 1;

    /* Bitmask for *all* Controllers */
    allControllers = (1 << numControllers) - 1;


    DCOLS = 0;
    DROWS = 0;

    for (i = 0; i < numControllers; i++) {
	char key[6];
	qprintf(key, sizeof(key), "Size%d", i + 1);
	size = cfg_get(section, key, NULL);
	if (size == NULL || *size == '\0') {
	    size_missing++;
	    free(size);
	    continue;
	}
	if (sscanf(size, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	    error("%s: bad %s.%s '%s' from %s", Name, section, key, size, cfg_source());
	    free(size);
	    return -1;
	}
	free(size);
	CCOLS[i] = cols;
	CROWS[i] = rows;
	size_defined++;
	info("%s: Controller %d: %dx%d", Name, i + 1, cols, rows);
	/* grow the size */
	if (cols > DCOLS)
	    DCOLS = cols;
	DROWS += rows;
    }
    if (size_defined && size_missing) {
	error("%s: bad %s.Size* definition in %s:", Name, section, cfg_source());
	error("%s: either you specify the size for *all* controllers or for none.", Name);
	return -1;
    }

    size = cfg_get(section, "Size", NULL);
    if (size != NULL && *size != '\0') {
	if (sscanf(size, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	    error("%s: bad %s.Size '%s' from %s", Name, section, size, cfg_source());
	    free(size);
	    return -1;
	}
	if (DCOLS == 0 && DROWS == 0) {
	    for (i = 0; i < numControllers; i++) {
		CCOLS[i] = cols;
		CROWS[i] = rows / numControllers;
		DCOLS = CCOLS[i];
		DROWS += CROWS[i];
	    }
	}
	if (rows != DROWS || cols != DCOLS) {
	    error("%s: bad %s.Size definition in %s:", Name, section, cfg_source());
	    error("%s: Size %dx%d should be %dx%d", Name, cols, rows, DCOLS, DROWS);
	    return -1;
	}
    }
    free(size);


    if (cfg_number(section, "GPOs", 0, 0, 8, &gpos) < 0)
	return -1;
    if (gpos > 0 && !(Capabilities & CAP_GPO)) {
	error("%s: Model '%s' does not support GPO's!", Name, Models[Model].name);
	gpos = 0;
    }
    GPOS = gpos;
    if (GPOS > 0) {
	info("%s: using %d GPO's", Name, GPOS);
    }

    if (drv_HD_load(section) < 0) {
	error("%s: start display failed!", Name);
	return -1;
    }

    drv_HD_command(allControllers, 0x08, T_EXEC);	/* Controller off, cursor off, blink off */
    drv_HD_command(allControllers, 0x0c, T_ONOFF);	/* Display on, cursor off, blink off, wait 1.64 ms */
    drv_HD_command(allControllers, 0x06, T_EXEC);	/* curser moves to right, no shift */

    if ((Capabilities & CAP_HD66712) && DROWS > 2) {
	drv_HD_command(allControllers, Bits == 8 ? 0x3c : 0x2c, T_EXEC);	/* set extended register enable bit */
	drv_HD_command(allControllers, 0x09, T_EXEC);	/* set 4-line mode */
	drv_HD_command(allControllers, Bits == 8 ? 0x38 : 0x28, T_EXEC);	/* clear extended register enable bit */
    }

    drv_HD_clear();		/* clear *all* displays */
    drv_HD_command(allControllers, 0x03, T_HOME);	/* return home */

    /* maybe set backlight */
#ifdef WITH_PARPORT
    if (Capabilities & CAP_BACKLIGHT) {
	int backlight;
	if (cfg_number(section, "Backlight", 0, 0, 1, &backlight) > 0) {
	    info("%s: backlight %s", Name, backlight ? "enabled" : "disabled");
	    drv_HD_backlight(backlight);
	}
    }
#endif

    /* maybe set brightness */
    if (Capabilities & CAP_BRIGHTNESS) {
	int brightness;
	if (cfg_number(section, "Brightness", 0, 0, 3, &brightness) > 0) {
	    info("%s: brightness level %d", Name, brightness);
	    drv_HD_brightness(brightness);
	}
    }

    /* install keypad polling timer for LCM-162 */
#ifdef WITH_PARPORT
    if (Capabilities & CAP_LCM162) {
	timer_add(drv_HD_LCM162_timer, NULL, 10, 0);
    }
#endif

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, NULL)) {
	    sleep(3);
	    drv_HD_clear();
	}
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/


#ifdef WITH_PARPORT
static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_HD_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}
#endif

static void plugin_brightness(RESULT * result, RESULT * arg1)
{
    double brightness;

    brightness = drv_HD_brightness(R2N(arg1));
    SetResult(&result, R_NUMBER, &brightness);
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
int drv_HD_list(void)
{
    int i;

    for (i = 0; Models[i].type != 0xff; i++) {
	printf("%s ", Models[i].name);
    }
    return 0;
}


/* initialize driver & display */
int drv_HD_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 1;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_HD_write;
    drv_generic_text_real_defchar = drv_HD_defchar;
#ifdef WITH_PARPORT
    drv_generic_gpio_real_set = drv_HD_GPO;
#endif

    /* start display */
    if ((ret = drv_HD_start(section, quiet)) != 0)
	return ret;

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic icon driver */
    if ((ret = drv_generic_text_icon_init()) != 0)
	return ret;

    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(0)) != 0)
	return ret;

    /* add fixed chars to the bar driver */
    /* most displays have a full block on ascii 255, but some have kind of  */
    /* an 'inverted P'. If you specify 'asc255bug 1 in the config, this */
    /* char will not be used, but rendered by the bar driver */
    cfg_number(section, "asc255bug", 0, 0, 1, &asc255bug);
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    if (!asc255bug)
	drv_generic_text_bar_add_segment(255, 255, 255, 255);	/* ASCII 255 = block */

    /* initialize generic GPIO driver */
    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_text_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);

    /* register plugins */
#ifdef WITH_PARPORT
    if (Capabilities & CAP_BACKLIGHT) {
	AddFunction("LCD::backlight", 1, plugin_backlight);
    }
#endif
    if (Capabilities & CAP_BRIGHTNESS) {
	AddFunction("LCD::brightness", 1, plugin_brightness);
    }

    return 0;
}


/* close driver & display */
int drv_HD_quit(const int quiet)
{

    info("%s: shutting down display.", Name);

    drv_generic_text_quit();
    drv_generic_gpio_quit();

    /* clear display */
    drv_HD_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_HD_stop();

    return (0);
}


DRIVER drv_HD44780 = {
  name:Name,
  list:drv_HD_list,
  init:drv_HD_init,
  quit:drv_HD_quit,
};
