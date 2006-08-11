/* $Id: drv_M50530.c,v 1.23 2006/08/11 11:59:29 reinelt Exp $
 *
 * new style driver for M50530-based displays
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: drv_M50530.c,v $
 * Revision 1.23  2006/08/11 11:59:29  reinelt
 * M50530 minor fixes
 *
 * Revision 1.22  2006/08/10 20:40:46  reinelt
 * M50530 enhancements: Timings, busy-flag checking
 *
 * Revision 1.21  2006/01/30 06:25:53  reinelt
 * added CVS Revision
 *
 * Revision 1.20  2006/01/05 18:56:57  reinelt
 * more GPO stuff
 *
 * Revision 1.19  2005/06/09 17:41:47  reinelt
 * M50530 fixes (many thanks to Szymon Bieganski)
 *
 * Revision 1.18  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.17  2005/05/05 08:36:12  reinelt
 * changed SELECT to SLCTIN
 *
 * Revision 1.16  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.15  2005/01/06 16:54:53  reinelt
 * M50530 fixes
 *
 * Revision 1.14  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.13  2004/06/26 09:27:20  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.12  2004/06/20 10:09:54  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.11  2004/06/06 06:51:59  reinelt
 *
 * do not display end splash screen if quiet=1
 *
 * Revision 1.10  2004/06/05 06:41:39  reinelt
 *
 * chancged splash screen again
 *
 * Revision 1.9  2004/06/05 06:13:12  reinelt
 *
 * splash screen for all text-based display drivers
 *
 * Revision 1.8  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.7  2004/06/01 06:45:29  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.6  2004/05/31 05:38:02  reinelt
 *
 * fixed possible bugs with user-defined chars (clear high bits)
 * thanks to Andy Baxter for debugging the MilfordInstruments driver!
 *
 * Revision 1.5  2004/05/29 15:53:28  reinelt
 *
 * M50530: reset parport signals on exit
 * plugin_ppp: ppp() has two parameters, not three
 * lcd4linux.conf.sample: diskstats() corrected
 *
 * Revision 1.4  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.3  2004/03/19 09:17:46  reinelt
 *
 * removed the extra 'goto' function, row and col are additional parameters
 * of the write() function now.
 *
 * Revision 1.2  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.1  2004/02/15 08:22:47  reinelt
 * ported USBLCD driver to NextGeneration
 * added drv_M50530.c (I forgot yesterday, sorry)
 * removed old drivers M50530.c and USBLCD.c
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_M50530
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
#include "drv_generic_text.h"
#include "drv_generic_gpio.h"
#include "drv_generic_parport.h"

static char Name[] = "M50530";

static int Model;

/* Timings */
static int T_SU, T_W, T_D, T_H;
static int T_INIT, T_EXEC, T_CLEAR;
static int T_GPO_ST, T_GPO_PW;

static unsigned char SIGNAL_RW;
static unsigned char SIGNAL_EX;
static unsigned char SIGNAL_IOC1;
static unsigned char SIGNAL_IOC2;
static unsigned char SIGNAL_GPO;

static int FONT5X11;
static int DDRAM;
static int DUTY;

/* maximum time to wait for the busy-flag (in usec) */
#define MAX_BUSYFLAG_WAIT 10000

/* maximum busy flag errors before falling back to busy-waiting */
#define MAX_BUSYFLAG_ERRORS 20

/* flag for busy-waiting vs. busy flag checking */
static int UseBusy = 0;

/* buffer holding the GPO state */
static unsigned char GPO = 0;


typedef struct {
    int type;
    char *name;
} MODEL;

static MODEL Models[] = {
    {0x01, "generic"},
    {0xff, "Unknown"}
};


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_M5_busy(void)
{
    static unsigned int errors = 0;

    unsigned char data = 0xFF;
    unsigned char busymask = 0x88;
    unsigned int counter;

    /* set data-lines to input */
    drv_generic_parport_direction(1);
    
    /* clear I/OC1 and I/OC2, set R/W */
    drv_generic_parport_control(SIGNAL_IOC1 | SIGNAL_IOC2 | SIGNAL_RW, SIGNAL_RW);
    
    /* Control data setup time */
    ndelay(T_SU);

    counter = 0;
    while (1) {

	/* rise enable */
	drv_generic_parport_control(SIGNAL_EX, SIGNAL_EX);

	/* data output delay time */
	ndelay(T_D);
	
	/* read the busy flag */
	data = drv_generic_parport_read();

	/* lower enable */
	/* as T_D is larger than T_W, we don't need to delay again */
	drv_generic_parport_control(SIGNAL_EX, 0);
	
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
		error("%s: timeout waiting for busy flag (0x%02x)", Name, data);
		if (++errors >= MAX_BUSYFLAG_ERRORS) {
		    error("%s: too many busy flag failures, turning off busy flag checking.", Name);
		    UseBusy = 0;
		}
		break;
	    }
	}
    }

    /* clear R/W */
    drv_generic_parport_control(SIGNAL_RW, 0);

    /* honour data hold time */
    ndelay (T_H);

    /* set data-lines to output */
    drv_generic_parport_direction(0);

}


static void drv_M5_command(const unsigned int cmd, const int delay)
{

    if (UseBusy)
	drv_M5_busy();

    /* put data on DB1..DB8 */
    drv_generic_parport_data(cmd & 0xff);

    /* set I/OC1 */
    /* set I/OC2 */
    /* clear RW */
    drv_generic_parport_control(SIGNAL_IOC1 | SIGNAL_IOC2 | SIGNAL_RW,
				(cmd & 0x100 ? SIGNAL_IOC1 : 0) | (cmd & 0x200 ? SIGNAL_IOC2 : 0));

    /* Control data setup time */
    ndelay(T_SU);

    /* send command */
    drv_generic_parport_toggle(SIGNAL_EX, 1, T_W);

    if (UseBusy) {
	/* honour data hold time */
	ndelay (T_H);
    } else {
	/* wait for command completion */
	udelay(delay);
    }
}


static void drv_M5_clear(void)
{
    drv_M5_command(0x0001, T_CLEAR);	/* clear display */
}


static void drv_M5_write(const int row, const int col, const char *data, const int len)
{
    int l = len;
    unsigned int cmd;
    unsigned int pos;

    if (row < 4) {
	pos = row * (DDRAM >> DUTY) + col;
    } else {
	pos = (row - 4) * (DDRAM >> DUTY) + (DDRAM >> DUTY) / 2 + col;
    }

    drv_M5_command(0x300 | pos, T_EXEC);

    while (l--) {
	cmd = *(unsigned char *) data++;
	drv_M5_command(0x200 | cmd, T_EXEC);
    }
}


static void drv_M5_defchar(const int ascii, const unsigned char *matrix)
{
    int i;

    drv_M5_command(0x300 + DDRAM + 8 * (ascii - CHAR0), T_EXEC);

    for (i = 0; i < YRES; i++) {
	drv_M5_command(0x200 | (matrix[i] & 0x3f), T_EXEC);
    }
}


static int drv_M5_GPO(const int num, const int val)
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


static int drv_M5_start(const char *section, const int quiet)
{
    char *s;
    int n;
    int rows = -1, cols = -1;
    unsigned int SF;

    s = cfg_get(section, "Model", "generic");
    if (s != NULL && *s != '\0') {
	int i;
	for (i = 0; Models[i].type != 0xff; i++) {
	    if (strcasecmp(Models[i].name, s) == 0)
		break;
	}
	free(s);
	if (Models[i].type == 0xff) {
	    error("%s: %s.Model '%s' is unknown from %s", Name, section, s, cfg_source());
	    return -1;
	}
	Model = i;
	info("%s: using model '%s'", Name, Models[Model].name);
    } else {
	error("%s: empty '%s.Model' entry from %s", Name, section, cfg_source());
	free(s);
	return -1;
    }

    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	free(s);
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);
    DROWS = rows;
    DCOLS = cols;

    s = cfg_get(section, "Font", "5x7");
    if (s == NULL && *s == '\0') {
	error("%s: empty '%s.Font' entry from %s", Name, section, cfg_source());
	free(s);
	return -1;
    }
    if (strcasecmp(s, "5x7") == 0) {
	XRES = 5;
	YRES = 7;
	FONT5X11 = 0;
    } else if (strcasecmp(s, "5x11") == 0) {
	XRES = 5;
	YRES = 11;
	FONT5X11 = 1;
    } else {
	error("%s: bad '%s.Font' entry '%s' from %s", Name, section, s, cfg_source());
	error("%s: should be '5x7' or '5x11'", Name);
	free(s);
	return -1;
    }
    free(s);


    if (DCOLS * DROWS > 256) {
	error("%s: %s.Size '%dx%d' is too big, would require %d bytes", Name, section, DCOLS, DROWS, DCOLS * DROWS,
	      cfg_source());
	return -1;
    } else if (DCOLS * DROWS > 224) {
	DDRAM = 256;
	CHARS = 0;
    } else if (DCOLS * DROWS > 192) {
	DDRAM = 224;
	CHARS = FONT5X11 ? 2 : 4;
    } else if (DCOLS * DROWS > 160) {
	DDRAM = 192;
	CHARS = FONT5X11 ? 4 : 8;
    } else {
	DDRAM = 160;
	CHARS = FONT5X11 ? 6 : 12;
    }
    info("%s: using %d words DDRAM / %d words CGRAM (%d user chars)", Name, DDRAM, 256 - DDRAM, CHARS);

    if (cfg_number(section, "DUTY", 2, 0, 2, &DUTY) < 0)
	return -1;
    info("%s: using duty %d: 1/%d, %d lines x %d words", Name, DUTY, (XRES + 1) << DUTY, 1 << DUTY, DDRAM >> DUTY);


    if (cfg_number(section, "GPOs", 0, 0, 8, &n) < 0)
	return -1;
    GPOS = n;
    if (GPOS > 0) {
	info("%s: controlling %d GPO's", Name, GPOS);
    }

    if (drv_generic_parport_open(section, Name) != 0) {
	error("%s: could not initialize parallel port!", Name);
	return -1;
    }

    if ((SIGNAL_RW = drv_generic_parport_wire_ctrl("RW", "GND")) == 0xff)
	return -1;
    if ((SIGNAL_EX = drv_generic_parport_wire_ctrl("EX", "STROBE")) == 0xff)
	return -1;
    if ((SIGNAL_IOC1 = drv_generic_parport_wire_ctrl("IOC1", "SLCTIN")) == 0xff)
	return -1;
    if ((SIGNAL_IOC2 = drv_generic_parport_wire_ctrl("IOC2", "AUTOFD")) == 0xff)
	return -1;
    if ((SIGNAL_GPO = drv_generic_parport_wire_ctrl("GPO", "GND")) == 0xff)
	return -1;

    /* Timings */

    /* low level communication timings [nanoseconds]
     * we use the worst-case default values, but allow
     * modification from the config file.
     */

    T_SU = timing(Name, section, "SU", 200, "ns"); /* control data setup time */
    T_W = timing(Name, section, "W", 500, "ns"); /* EX signal pulse width */
    T_D = timing(Name, section, "D", 300, "ns"); /* Data output delay time */
    T_H = timing(Name, section, "H", 100, "ns"); /* Data hold time */

    /* GPO timing */
    if (SIGNAL_GPO != 0) {
	T_GPO_ST = timing(Name, section, "GPO_ST", 20, "ns");	/* 74HCT573 set-up time */
	T_GPO_PW = timing(Name, section, "GPO_PW", 230, "ns");	/* 74HCT573 enable pulse width */
    } else {
	T_GPO_ST = 0;
	T_GPO_PW = 0;
    }

    /* M50530 execution timings [microseconds]
     * we use the worst-case default values, but allow
     * modification from the config file.
     */

    T_EXEC = timing(Name, section, "EXEC", 20, "us"); /* normal execution time */
    T_CLEAR = timing(Name, section, "CLEAR", 1250, "us"); /* 'clear display' execution time */
    T_INIT = timing(Name, section, "INIT", 2000, "us"); /* mysterious initialization time */


    /* maybe use busy-flag from now on  */
    cfg_number(section, "UseBusy", 0, 0, 1, &UseBusy);

    /* make sure we don't use the busy flag with RW wired to GND */
    if (UseBusy && !SIGNAL_RW) {
	error("%s: busy-flag checking is impossible with RW wired to GND!", Name);
	UseBusy = 0;
    }
    info("%s: %susing busy-flag checking", Name, UseBusy ? "" : "not ");

    /* clear all signals */
    drv_generic_parport_control(SIGNAL_RW | SIGNAL_EX | SIGNAL_IOC1 | SIGNAL_IOC2 | SIGNAL_GPO, 0);

    /* for some mysterious reason, this delay is necessary... */
    udelay(T_INIT);

    /* set direction: write */
    drv_generic_parport_direction(0);


    /* set function (SF) mode:
     * Bit 0 : RAM 0
     * Bit 1 : RAM 1
     * Bit 2 : DUTY 0
     * Bit 3 : DUTY 1
     * Bit 4 : FONT: 0 = 5x12,  1 = 5x8
     * Bit 5 : I/O:  0 = 4-bit, 1 = 8-bit
     * Bit 6 : 1
     * Bit 7 : 1
     *
     * RAM layout:
     * 00: 256 words DDRAM,  0 words CGRAM
     * 01: 224 words DDRAM, 32 words CGRAM
     * 10: 192 words DDRAM, 64 words CGRAM
     * 11: 160 words DDRAM, 96 words CGRAM
     *
     * Duty:
     * 00: 1/8  (1/12 for 5x11), 1 line
     * 00: 1/16 (1/24 for 5x11), 2 lines
     * 00: 1/32 (1/48 for 5x11), 4 lines
     * 11: undefined
     */

    /* 11100000 : SF, 8-bit mode */
    SF = 0xE0;

    /* RAM layout */
    switch (DDRAM) {
    case 160:
	SF |= 0x03;
	break;
    case 192:
	SF |= 0x02;
	break;
    case 224:
	SF |= 0x01;
	break;
    case 256:
	SF |= 0x00;
	break;
    }

    /* Duty */
    SF |= (DUTY << 2);

    /* Font */
    SF |= (!FONT5X11) << 4;


    debug("SET FUNCTION MODE 0x%02x", SF);
    drv_M5_command(SF, T_EXEC);


    /* set display (SD) mode:
     * Bit 0 : 1 = turn on character blinking
     * Bit 0 : 1 = turn on cursor blinking
     * Bit 0 : 1 = turn on underline display
     * Bit 0 : 1 = turn on cursor
     * Bit 0 : 1 = turn on display
     * Bit 0 : 1
     * Bit 0 : 0
     * Bit 0 : 0
     */

    /* SD: 0x20 = 00100000 */
    /* SD: turn off display */
    drv_M5_command(0x0020, T_EXEC);


    /* set entry (SE) mode:
     * Bit 0 : 1 = inc/dec cursor address after Read
     * Bit 1 : 1 = inc/dec cursor address after Write
     * Bit 2 : 0 = inc, 1 = dec
     * Bit 3 : 1 = inc/dec display start addr after Read
     * Bit 4 : 1 = inc/dec display start addr after Write
     * Bit 5 : 0 = inc, 1 = dec
     * Bit 6 : 1
     * Bit 7 : 0
     *
     */

    /* SE: 0x50 = 01010000 */
    /* SE: increment display start after Write */
    drv_M5_command(0x0050, T_EXEC);


    /* SD: 0x30 = 00110000 */
    /* SD: turn on display */
    drv_M5_command(0x0030, T_EXEC);


    drv_M5_clear();

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, NULL)) {
	    sleep(3);
	    drv_M5_clear();
	}
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment */


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
int drv_M5_list(void)
{
    int i;

    for (i = 0; Models[i].type != 0xff; i++) {
	printf("%s ", Models[i].name);
    }
    return 0;
}


/* initialize driver & display */
int drv_M5_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Revision: 1.23 $");

    /* display preferences */
    XRES = -1;			/* pixel width of one char  */
    YRES = -1;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 248;		/* ASCII of first user-defineable char */
    GOTO_COST = 1;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_M5_write;
    drv_generic_text_real_defchar = drv_M5_defchar;
    drv_generic_gpio_real_set = drv_M5_GPO;


    /* start display */
    if ((ret = drv_M5_start(section, quiet)) != 0)
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
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */

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
    /* none at the moment */

    return 0;
}


/* close driver & display */
int drv_M5_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();
    drv_generic_gpio_quit();

    /* clear display */
    drv_M5_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    /* clear all signals */
    drv_generic_parport_control(SIGNAL_RW | SIGNAL_EX | SIGNAL_IOC1 | SIGNAL_IOC2 | SIGNAL_GPO, 0);

    /* close port */
    drv_generic_parport_close();

    return (0);
}


DRIVER drv_M50530 = {
  name:Name,
  list:drv_M5_list,
  init:drv_M5_init,
  quit:drv_M5_quit,
};
