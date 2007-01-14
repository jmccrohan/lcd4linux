/* $Id$
 * $URL$
 *
 * new style driver for Cwlinux display modules
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_Cwlinux
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_gpio.h"
#include "drv_generic_serial.h"


static char Name[] = "Cwlinux";

static int Model;
static int Protocol;

typedef struct {
    int type;
    char *name;
    int rows;
    int cols;
    int xres;
    int gpos;
    int protocol;
} MODEL;


/* Fixme: number of gpo's should be verified */

static MODEL Models[] = {
    {0x01, "CW1602", 2, 16, 5, 0, 1},
    {0x02, "CW12232", 4, 20, 6, 0, 2},
    {0xff, "Unknown", -1, -1, -1, -1, -1}
};


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_CW_send(const char *string, const int len)
{
    drv_generic_serial_write(string, len);
    usleep(20);
}


static void drv_CW_write(const int row, const int col, const char *data, const int len)
{
    char cmd[6] = "\376Gxy\375";

    cmd[2] = (char) col;
    cmd[3] = (char) row;
    drv_CW_send(cmd, 5);
    drv_CW_send(data, len);
}


static void drv_CW1602_defchar(const int ascii, const unsigned char *buffer)
{
    int i;
    char cmd[12] = "\376Nn12345678\375";

    cmd[2] = (char) ascii;

    for (i = 0; i < 8; i++) {
	cmd[3 + i] = buffer[i] & 0x1f;
    }
    drv_CW_send(cmd, 12);
}


static void drv_CW12232_defchar(const int ascii, const unsigned char *buffer)
{
    int i, j;
    char cmd[10] = "\376Nn123456\375";

    cmd[2] = (char) ascii;

    /* The CW12232 uses a vertical bitmap layout, */
    /* so we have to 'rotate' the bitmap. */

    for (i = 0; i < 6; i++) {
	cmd[3 + i] = 0;
	for (j = 0; j < 8; j++) {
	    if (buffer[j] & (1 << (5 - i))) {
		cmd[3 + i] |= (1 << j);
	    }
	}
    }
    drv_CW_send(cmd, 10);
}


static int drv_CW_GPO(const int num, const int val)
{
    /* Fixme: GPO's not yet implemented! */
    error("%s: GPO's not yet implemented!", Name);
    /* Fixme: num*val to avoid compiler warning */
    return num * val;
}


static void drv_CW_clear(void)
{
#if 1
    drv_CW_send("\376X\375", 3);	/* Clear Display */
    usleep(500000);
#else
    /* for some mysterious reason, we have to sleep after  */
    /* the command _and_ after the CMD_END... */
    drv_CW_send("\376X", 2);	/* Clear Display */
    drv_CW_send("\375", 1);	/* Command End */
#endif
}


static int drv_CW_brightness(int brightness)
{
    static unsigned char Brightness = 0;
    char cmd[5] = "\376A_\375";

    /* -1 is used to query the current brightness */
    if (brightness == -1)
	return Brightness;

    if (brightness < 0)
	brightness = 0;
    if (brightness > 255)
	brightness = 255;
    Brightness = brightness;

    switch (Brightness) {
    case 0:
	/* backlight off */
	drv_CW_send("\376F\375", 3);
	break;
    case 8:
	/* backlight on */
	drv_CW_send("\376B\375", 3);
	break;
    default:
	/* backlight level */
	cmd[2] = (char) Brightness;
	drv_CW_send(cmd, 4);
	break;
    }

    return Brightness;
}


static int drv_CW_start(const char *section)
{
    int i;
    char *model;

    model = cfg_get(section, "Model", NULL);
    if (model != NULL && *model != '\0') {
	for (i = 0; Models[i].type != 0xff; i++) {
	    if (strcasecmp(Models[i].name, model) == 0)
		break;
	}
	if (Models[i].type == 0xff) {
	    error("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
	    return -1;
	}
	Model = i;
	info("%s: using model '%s'", Name, Models[Model].name);
    } else {
	error("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
	return -1;
    }

    /* open serial port */
    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    /* this does not work as I'd expect it... */
#if 0
    /* read firmware version */
    generic_serial_read(buffer, sizeof(buffer));
    usleep(100000);
    generic_serial_write("\3761", 2);
    usleep(100000);
    generic_serial_write("\375", 1);
    usleep(100000);
    if (generic_serial_read(buffer, 2) != 2) {
	info("unable to read firmware version!");
    }
    info("Cwlinux Firmware %d.%d", (int) buffer[0], (int) buffer[1]);
#endif

    /* initialize global variables */
    DROWS = Models[Model].rows;
    DCOLS = Models[Model].cols;
    XRES = Models[Model].xres;
    GPOS = Models[Model].gpos;
    Protocol = Models[Model].protocol;

    drv_CW_clear();

    drv_CW_send("\376D\375", 3);	/* auto line wrap off */
    drv_CW_send("\376R\375", 3);	/* auto scroll off */
    drv_CW_send("\376K\375", 3);	/* underline cursor off */
    drv_CW_send("\376B\375", 3);	/* backlight on */

    /* set brightness */
    if (cfg_number(section, "Brightness", 0, 0, 8, &i) > 0) {
	drv_CW_brightness(i);
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/


static void plugin_brightness(RESULT * result, const int argc, RESULT * argv[])
{
    double brightness;

    switch (argc) {
    case 0:
	brightness = drv_CW_brightness(-1);
	SetResult(&result, R_NUMBER, &brightness);
	break;
    case 1:
	brightness = drv_CW_brightness(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &brightness);
	break;
    default:
	error("%s.brightness(): wrong number of parameters", Name);
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
int drv_CW_list(void)
{
    int i;

    for (i = 0; Models[i].type != 0xff; i++) {
	printf("%s ", Models[i].name);
    }
    return 0;
}


/* initialize driver & display */
int drv_CW_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Revision: 1.26 $");

    /* display preferences */
    XRES = 6;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 16;			/* number of user-defineable characters */
    CHAR0 = 1;			/* ASCII of first user-defineable char */
    GOTO_COST = 3;		/* number of bytes a goto command requires */
    INVALIDATE = 1;		/* re-defined chars must be re-sent to the display */

    /* start display */
    if ((ret = drv_CW_start(section)) != 0)
	return ret;

    /* real worker functions */
    drv_generic_text_real_write = drv_CW_write;
    drv_generic_gpio_real_set = drv_CW_GPO;

    switch (Protocol) {
    case 1:
	drv_generic_text_real_defchar = drv_CW1602_defchar;
	break;
    case 2:
	drv_generic_text_real_defchar = drv_CW12232_defchar;
	break;
    }

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %s", Name, Models[Model].name);
	if (drv_generic_text_greet(buffer, "www.cwlinux.com")) {
	    sleep(3);
	    drv_CW_clear();
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
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII 32 = blank */

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
    AddFunction("LCD::brightness", -1, plugin_brightness);

    return 0;
}


/* close driver & display */
int drv_CW_quit(const int quiet)
{

    info("%s: shutting down.", Name);
    drv_generic_text_quit();
    drv_generic_gpio_quit();

    /* clear display */
    drv_CW_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_generic_serial_close();

    return (0);
}


DRIVER drv_Cwlinux = {
  name:Name,
  list:drv_CW_list,
  init:drv_CW_init,
  quit:drv_CW_quit,
};
