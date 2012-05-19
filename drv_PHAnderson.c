/* $Id: drv_PHAnderson.c 840 2008-11-19 23:56:42 guimli $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_PHAnderson.c $
 *
 * driver for the PHAnderson serial-to-HD44780 adapter boards
 * http://www.phanderson.com/lcd106/lcd107.html
 * http://moderndevice.com/Docs/LCD117CommandSummary.doc
 * http://wulfden.org/TheShoppe/k107/index.shtml
 *
 * Copyright (C) 2008 Nicolas Weill <guimli@free.fr>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_PHAnderson
 *
 */

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
#include "drv_generic_serial.h"

#define PHAnderson_CMD  '?'

static char Name[] = "PHAnderson";

static int T_READ, T_WRITE, T_BOOT;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_PHAnderson_open(const char *section)
{
    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_PHAnderson_close(void)
{
    drv_generic_serial_close();

    return 0;
}


static void drv_PHAnderson_send(const char *data, const unsigned int len)
{
    char buffer[255];
    unsigned int i, j;

    j = 0;
    for (i = 0; i < len; i++) {
	if (data[i] < 8) {
	    buffer[j++] = '?';
	    buffer[j++] = '0' + data[i];
	    drv_generic_serial_write(buffer, j);
	    udelay(T_READ);

	    j = 0;
	} else {
	    buffer[j++] = data[i];
	}
    }

    if (j > 0) {
	drv_generic_serial_write(buffer, j);
    }
}


static void drv_PHAnderson_clear(void)
{
    char cmd[2];

    cmd[0] = '?';
    cmd[1] = 'f';
    drv_PHAnderson_send(cmd, 2);
}


static void drv_PHAnderson_write(const int row, const int col, const char *data, int len)
{
    char cmd[7];

    cmd[0] = '?';
    cmd[1] = 'y';
    cmd[2] = row + '0';
    cmd[3] = '?';
    cmd[4] = 'x';
    cmd[5] = col / 10 + '0';
    cmd[6] = col % 10 + '0';
    drv_PHAnderson_send(cmd, 7);

    drv_PHAnderson_send(data, len);
}


static void drv_PHAnderson_defchar(const int ascii, const unsigned char *matrix)
{
    char cmd[19];
    int i;

    cmd[0] = '?';
    cmd[1] = 'D';
    cmd[2] = ascii + '0';

    for (i = 0; i < 8; i++) {
	cmd[i * 2 + 3] = ((matrix[i] >> 4) & 1) + '0';
	cmd[i * 2 + 4] = (((matrix[i] & 0x0F) < 10) ? (matrix[i] & 0x0F) + '0' : (matrix[i] & 0x0F) + '7');
    }
    drv_PHAnderson_send(cmd, 19);
    udelay(T_WRITE);
}


static int drv_PHAnderson_blacklight(int blacklight)
{
    char cmd[4];

    if (blacklight < 0)
	blacklight = 0;
    if (blacklight > 255)
	blacklight = 255;

    cmd[0] = '?';
    cmd[1] = 'B';
    cmd[2] = (((blacklight >> 4) < 10) ? (blacklight >> 4) + '0' : (blacklight >> 4) + '7');
    cmd[3] = (((blacklight & 0x0F) < 10) ? (blacklight & 0x0F) + '0' : (blacklight & 0x0F) + '7');
    drv_PHAnderson_send(cmd, 4);
    udelay(T_WRITE);

    return blacklight;
}

static int drv_PHAnderson_bootscreen(const char *bootmsg)
{
    char cmd[255];
    int i, j, k;

    i = 0;
    j = 0;
    k = 0;

    if (bootmsg == NULL || *bootmsg == '\0') {
	cmd[0] = '?';
	cmd[1] = 'S';
	cmd[2] = '0';
	drv_PHAnderson_send(cmd, 3);
	udelay(T_WRITE);
    } else {
	cmd[0] = '?';
	cmd[1] = 'S';
	cmd[2] = '2';
	drv_PHAnderson_send(cmd, 3);
	udelay(T_WRITE);
	cmd[0] = '?';
	cmd[1] = 'C';
	for (i = 0; i < DROWS; i++) {
	    cmd[2] = '0' + i;
	    for (k = 0; k < DCOLS; k++) {
		if (bootmsg[j] != '\0') {
		    cmd[3 + k] = bootmsg[j];
		    j++;
		} else {
		    cmd[3 + k] = ' ';
		}
	    }
	    drv_PHAnderson_send(cmd, k + 3);
	    udelay(T_BOOT);
	}
    }

    return j;
}

static int drv_PHAnderson_start(const char *section)
{
    int blacklight;
    int rows = -1, cols = -1;
    char *s;
    char cmd[255];

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

    /* PHAnderson execution timings [milliseconds]
     * we use the worst-case default values, but allow
     * modification from the config file.
     */

    T_WRITE = timing(Name, section, "WRITE", 170, "ms") * 1000;	/* write eeprom time */
    T_READ = timing(Name, section, "READ", 10, "ms") * 1000;	/* read eeprom time */
    T_BOOT = timing(Name, section, "BOOT", 300, "ms") * 1000;	/* boot screen write eeprom time */

    if (drv_PHAnderson_open(section) < 0) {
	return -1;
    }

    /* Define screen size */
    cmd[0] = '?';
    cmd[1] = 'G';
    cmd[2] = rows + '0';
    cmd[3] = (cols / 10) + '0';
    cmd[4] = (cols % 10) + '0';
    drv_PHAnderson_send(cmd, 5);
    udelay(T_WRITE);

    /* Hide cursor */
    cmd[0] = '?';
    cmd[1] = 'c';
    cmd[2] = '0';
    drv_PHAnderson_send(cmd, 3);
    udelay(T_WRITE);

    if (cfg_number(section, "Blacklight", 0, 0, 255, &blacklight) > 0) {
	drv_PHAnderson_blacklight(blacklight);
    }

    s = cfg_get(section, "Bootscreen", NULL);
    printf("%s", s);
    drv_PHAnderson_bootscreen(s);

    drv_PHAnderson_clear();	/* clear display */

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_blacklight(RESULT * result, RESULT * arg1)
{
    double blacklight;

    blacklight = drv_PHAnderson_blacklight(R2N(arg1));
    SetResult(&result, R_NUMBER, &blacklight);
}

static void plugin_bootscreen(RESULT * result, RESULT * arg1)
{
    double bootmsg;

    bootmsg = drv_PHAnderson_bootscreen(R2S(arg1));
    SetResult(&result, R_NUMBER, &bootmsg);
}

/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_PHAnderson_list(void)
{
    printf("PHAnderson serial-to-HD44780 adapter");
    return 0;
}

int drv_PHAnderson_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev: 840 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 7;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_PHAnderson_write;
    drv_generic_text_real_defchar = drv_PHAnderson_defchar;

    /* start display */
    if ((ret = drv_PHAnderson_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "www.bwct.de")) {
	    sleep(3);
	    drv_PHAnderson_clear();
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

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 0, 32);	/* ASCII  32 = blank */

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
    AddFunction("LCD::blacklight", 1, plugin_blacklight);

    AddFunction("LCD::bootscreen", 1, plugin_bootscreen);

    return 0;
}


int drv_PHAnderson_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    drv_PHAnderson_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_PHAnderson_close();

    return (0);
}


DRIVER drv_PHAnderson = {
    .name = Name,
    .list = drv_PHAnderson_list,
    .init = drv_PHAnderson_init,
    .quit = drv_PHAnderson_quit,
};
