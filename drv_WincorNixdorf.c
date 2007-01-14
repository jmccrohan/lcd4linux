/* $Id$
 * $URL$
 * 
 * driver for WincorNixdorf serial cashier displays BA63 and BA66
 * 
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the SimpleLCD driver which is
 * Copyright (C) 2005 Julien Aube <ob@obconseil.net>
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
 * struct DRIVER drv_WincorNixdorf
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_serial.h"


#define ESC "\033"


static char Name[] = "WincorNixdorf";

typedef struct {
    int type;
    char *name;
    int rows;
    int cols;
} MODEL;

static MODEL Models[] = {
    {63, "BA63", 2, 20},
    {66, "BA66", 4, 20},
    {-1, "unknown", -1, -1},
};

static int Model;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_WN_clear(void)
{
    drv_generic_serial_write(ESC "[2J", 4);
}


static void drv_WN_write(const int row, const int col, const char *data, int len)
{
    char cmd[8] = ESC "[r;ccH";

    cmd[2] = '1' + row;
    cmd[4] = '0' + (col / 10);
    cmd[5] = '1' + (col % 10);

    drv_generic_serial_write(cmd, 7);
    drv_generic_serial_write(data, len);
}


static int drv_WN_start(const char *section, const int quiet)
{
    int i, len;
    int selftest;
    char *model = NULL;
    char buffer[32];

    model = cfg_get(section, "Model", NULL);
    if (model == NULL && *model == '\0') {
	error("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
	return -1;
    }

    for (i = 0; Models[i].type != -1; i++) {
	if (strcasecmp(Models[i].name, model) == 0)
	    break;
    }
    if (Models[i].type == -1) {
	error("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
	return -1;
    }
    Model = i;
    info("%s: using model '%s'", Name, Models[Model].name);

    /* initialize global variables */
    DROWS = Models[Model].rows;
    DCOLS = Models[Model].cols;

    if (drv_generic_serial_open(section, Name, CS8 | PARENB | PARODD) < 0)
	return -1;

    /* real worker functions */
    drv_generic_text_real_write = drv_WN_write;

    cfg_number(section, "SelfTest", 0, 0, 1, &selftest);
    if (selftest) {
	info("%s: initiating display selftest sequence", Name);

	/* read display identification */
	drv_generic_serial_write(ESC "[0c", 4);
	usleep(100 * 1000);

	if ((len = drv_generic_serial_read(buffer, -sizeof(buffer))) > 0) {
	    info("%s: waiting 15 seconds for selftest", Name);
	    drv_generic_serial_write(buffer, len);
	    sleep(15);
	    info("%s: selftest finished", Name);
	} else {
	    info("%s: selftest initiation failed", Name);
	}
    }

    /* clear display */
    drv_WN_clear();

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, NULL)) {
	    sleep(3);
	    drv_WN_clear();
	}
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_WN_list(void)
{
    printf("BA63 BA66");
    return 0;
}


/* initialize driver & display */
int drv_WN_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ascii;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 7;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    ICONS = 0;			/* number of user-defineable characters reserved for icons */
    GOTO_COST = 6;		/* number of bytes a goto command requires */

    /* start display */
    if ((ret = drv_WN_start(section, quiet)) != 0)
	return ret;

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(1)) != 0)
	return ret;

    cfg_number(section, "BarChar", '*', 1, 255, &ascii);

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    drv_generic_text_bar_add_segment(255, 255, 255, ascii);

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);

    /* register plugins */
    /* none */

    return 0;
}


/* close driver & display */
int drv_WN_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_WN_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_generic_serial_close();

    return (0);
}


DRIVER drv_WincorNixdorf = {
  name:Name,
  list:drv_WN_list,
  init:drv_WN_init,
  quit:drv_WN_quit,
};
