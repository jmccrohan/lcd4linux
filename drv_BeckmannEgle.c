/* $Id$
 * $URL$
 *
 * driver for Beckmann+Egle "Mini Terminals" and "Compact Terminals"
 * Copyright (C) 2000 Michael Reinelt <reinelt@eunet.at>
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
 * struct DRIVER drv_BeckmannEgle
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

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
#include "drv_generic_serial.h"


#define ESC "\033"

static char Name[] = "Beckmann+Egle";


typedef struct {
    char *name;
    int rows;
    int cols;
    int protocol;
    int type;
} MODEL;


static MODEL Models[] = {

    /* MultiTerminals */
    {"MT16x1", 1, 16, 1, 0},
    {"MT16x2", 2, 16, 1, 1},
    {"MT16x4", 4, 16, 1, 2},
    {"MT20x1", 1, 20, 1, 3},
    {"MT20x2", 2, 20, 1, 4},
    {"MT20x4", 4, 20, 1, 5},
    {"MT24x1", 1, 24, 1, 6},
    {"MT24x2", 2, 24, 1, 7},
    {"MT32x1", 1, 32, 1, 8},
    {"MT32x2", 2, 32, 1, 9},
    {"MT40x1", 1, 40, 1, 10},
    {"MT40x2", 2, 40, 1, 11},
    {"MT40x4", 4, 40, 1, 12},

    /* CompactTerminal */
    {"CT20x4", 4, 20, 2, 0},

    {NULL, 0, 0, 0, 0},
};


static int Model;
static int Protocol;



/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_BuE_clear(void)
{
    switch (Protocol) {
    case 1:
	drv_generic_serial_write(ESC "&#", 3);	/* clear display */
	break;
    case 2:
	drv_generic_serial_write(ESC "LL", 3);	/* clear display */
	break;
    }
}


static void drv_BuE_MT_write(const int row, const int col, const char *data, const int len)
{
    char cmd[] = ESC "[y;xH";

    cmd[2] = (char) row;
    cmd[4] = (char) col;

    drv_generic_serial_write(cmd, 6);
    drv_generic_serial_write(data, len);
}


static void drv_BuE_MT_defchar(const int ascii, const unsigned char *matrix)
{
    int i;
    char cmd[22] = ESC "&T";	/* enter transparent mode */

    cmd[3] = '\0';		/* write cmd */
    cmd[4] = 0x40 | 8 * ascii;	/* write CGRAM */

    for (i = 0; i < 8; i++) {
	cmd[2 * i + 5] = '\1';	/* write data */
	cmd[2 * i + 6] = matrix[i] & 0x1f;	/* character bitmap */
    }
    cmd[21] = '\377';		/* leave transparent mode */

    drv_generic_serial_write(cmd, 22);
}


static void drv_BuE_CT_write(const int row, const int col, const char *data, const int len)
{
    char cmd[] = ESC "LCzs\001";
    cmd[3] = (char) row + 1;
    cmd[4] = (char) col + 1;

    drv_generic_serial_write(cmd, 6);
    drv_generic_serial_write(data, len);

}


static void drv_BuE_CT_defchar(const int ascii, const unsigned char *matrix)
{
    int i;
    char cmd[13] = ESC "LZ";	/* set custom char */

    /* number of user-defined char (0..7) */
    cmd[3] = (char) ascii - CHAR0;

    /* ASCII code to replace */
    cmd[4] = (char) ascii;

    for (i = 0; i < 8; i++) {
	cmd[i + 5] = matrix[i] & 0x1f;
    }

    drv_generic_serial_write(cmd, 13);
}


static int drv_BuE_CT_contrast(int contrast)
{
    static char Contrast = 7;
    char cmd[4] = ESC "LKn";

    /* -1 is used to query the current contrast */
    if (contrast == -1)
	return Contrast;

    if (contrast < 0)
	contrast = 0;
    if (contrast > 15)
	contrast = 15;
    Contrast = contrast;

    cmd[3] = Contrast;

    drv_generic_serial_write(cmd, 4);

    return Contrast;
}


static int drv_BuE_CT_backlight(int backlight)
{
    static char Backlight = 0;
    char cmd[4] = ESC "LBn";

    /* -1 is used to query the current backlight */
    if (backlight == -1)
	return Backlight;

    if (backlight < 0)
	backlight = 0;
    if (backlight > 1)
	backlight = 1;
    Backlight = backlight;

    cmd[3] = Backlight;

    drv_generic_serial_write(cmd, 4);

    return Backlight;
}


static int drv_BuE_CT_gpo(int num, int val)
{
    static int GPO[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    char cmd[4] = ESC "Pnx";

    if (num < 0)
	num = 0;
    if (num > 7)
	num = 7;

    /* -1 is used to query the current GPO */
    if (val == -1)
	return GPO[num];

    if (val < 0)
	val = 0;
    if (val > 255)
	val = 255;
    GPO[num] = val;

    cmd[2] = (char) num;
    cmd[3] = (char) val;

    drv_generic_serial_write(cmd, 4);

    return GPO[num];
}


static int drv_BuE_CT_gpi(int num)
{
    char cmd[4] = ESC "?Pn";
    char buffer[4];

    if (num < 0)
	num = 0;
    if (num > 7)
	num = 7;

    cmd[3] = (char) num;
    drv_generic_serial_write(cmd, 4);

    usleep(10000);

    if (drv_generic_serial_read(buffer, 4) != 4) {
	error("%s: error reading port %d", Name, num);
	return -1;
    }

    return buffer[3];
}


static int drv_BuE_CT_adc(void)
{
    char buffer[4];

    drv_generic_serial_write(ESC "?A", 3);

    usleep(10000);

    if ((drv_generic_serial_read(buffer, 4) != 4) || (buffer[0] != 'A') || (buffer[1] != ':')
	) {
	error("%s: error reading ADC", Name);
	return -1;
    }

    /* 10 bit value: 8 bit high, 2 bit low */
    return 4 * (unsigned char) buffer[2] + (unsigned char) buffer[3];
}


static int drv_BuE_CT_pwm(int val)
{
    static int PWM = -1;
    char cmd[4] = ESC "Adm";

    /* -1 is used to query the current PWM */
    if (val == -1)
	return PWM;

    if (val < 0)
	val = 0;
    if (val > 255)
	val = 255;
    PWM = val;

    cmd[2] = (char) val;
    cmd[3] = val == 0 ? 1 : 2;
    drv_generic_serial_write(cmd, 4);

    return val;
}


static int drv_BuE_MT_start(const char *section)
{
    char cmd[] = ESC "&sX";

    /* CSTOPB: 2 stop bits */
    if (drv_generic_serial_open(section, Name, CSTOPB) < 0)
	return -1;

    cmd[4] = Models[Model].type;
    drv_generic_serial_write(cmd, 4);	/* select display type */
    drv_generic_serial_write(ESC "&D", 3);	/* cursor off */

    return 0;
}


static int drv_BuE_CT_start(const char *section)
{
    char buffer[16];
    char *size;
    int i, len;

    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

#if 0
    /* restart terminal */
    drv_generic_serial_write(ESC "Kr", 3);
    usleep(10000);
#endif

    /* Fixme: the CT does not return a serial number in byte mode */
    /* set parameter mode 'decimal' */
    drv_generic_serial_write(ESC "KM\073", 4);

    /* read version */
    drv_generic_serial_write(ESC "?V", 3);
    usleep(100000);
    if ((len = drv_generic_serial_read(buffer, -sizeof(buffer))) > 0) {
	int v, r, s;
	if (sscanf(buffer, "V:%d.%d,%d;", &v, &r, &s) != 3) {
	    error("%s: error parsing display identification <%*s>", Name, len, buffer);
	} else {
	    info("%s: display identified as version %d.%d, S/N %d", Name, v, r, s);
	}
    }

    /* set parameter mode 'byte' */
    drv_generic_serial_write(ESC "KM\072", 4);

    /* the CT20x4 can control smaller displays, too */
    size = cfg_get(section, "Size", NULL);
    if (size != NULL && *size != '\0') {
	int r, c;
	char cmd[6] = ESC "LArc";
	if (sscanf(size, "%dx%d", &c, &r) != 2 || r < 1 || c < 1) {
	    error("%s: bad %s.Size '%s' from %s", Name, section, size, cfg_source());
	    return -1;
	}
	info("%s: display size: %d rows %d columns", Name, r, c);
	/* set display size */
	cmd[3] = (char) r;
	cmd[4] = (char) c;
	drv_generic_serial_write(cmd, 5);
	DCOLS = c;
	DROWS = r;
    }

    /* set contrast */
    if (cfg_number(section, "Contrast", 7, 0, 15, &i) > 0) {
	drv_BuE_CT_contrast(i);
    }

    /* set backlight */
    if (cfg_number(section, "Backlight", 0, 0, 1, &i) > 0) {
	drv_BuE_CT_backlight(i);
    }


    /* identify modules */

    for (i = 0; i < 8; i++) {
	char cmd[5] = ESC "K?Pn";
	cmd[4] = (char) i;
	drv_generic_serial_write(cmd, 5);	/* query I/O port */
	usleep(10000);
	if ((len = drv_generic_serial_read(buffer, 4)) == 4) {
	    char *type = NULL;
	    if (i == 0) {
		if (buffer[3] == 8) {
		    /* internal port */
		    type = "CT 20x4 internal port";
		} else {
		    error("%s: internal error: port 0 type %d should be type 8", Name, buffer[3]);
		    continue;
		}
	    } else {
		switch (buffer[3]) {
		case 1:	/* Key Module */
		    type = "XM-KEY-2x4-LED";
		    break;
		case 8:	/* I/O Module */
		    type = "XM-IO8-T";
		    break;
		case 9:	/* I/O Module */
		    type = "XM-IO4-R";
		    break;
		case 15:	/* nothing */
		    continue;
		default:	/* unhandled */
		    type = NULL;
		    break;
		}
	    }
	    if (type != NULL) {
		info("%s: Port %d: %s", Name, i, type);
	    } else {
		error("%s: internal error: port %d unknown type %d", Name, i, cmd[3]);
	    }
	} else {
	    error("%s: error fetching type of port %d", Name, i);
	}
    }

    return 0;
}


static int drv_BuE_start(const char *section)
{
    int i, ret;
    char *model;

    model = cfg_get(section, "Model", NULL);
    if (model == NULL && *model == '\0') {
	error("%s: no '%s.Model' entry from %s", Name, section, cfg_source());
	return -1;
    }

    for (i = 0; Models[i].name != NULL; i++) {
	if (strcasecmp(Models[i].name, model) == 0)
	    break;
    }

    if (Models[i].name == NULL) {
	error("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
	return -1;
    }

    Model = i;
    Protocol = Models[Model].protocol;

    info("%s: using model '%s'", Name, Models[Model].name);

    /* initialize global variables */
    DROWS = Models[Model].rows;
    DCOLS = Models[Model].cols;

    ret = 0;
    switch (Protocol) {
    case 1:
	ret = drv_BuE_MT_start(section);
	break;
    case 2:
	ret = drv_BuE_CT_start(section);
	break;
    }
    if (ret != 0) {
	return ret;
    }

    drv_BuE_clear();

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, const int argc, RESULT * argv[])
{
    double contrast;

    switch (argc) {
    case 0:
	contrast = drv_BuE_CT_contrast(-1);
	SetResult(&result, R_NUMBER, &contrast);
	break;
    case 1:
	contrast = drv_BuE_CT_contrast(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &contrast);
	break;
    default:
	error("%s::contrast(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
}


static void plugin_backlight(RESULT * result, const int argc, RESULT * argv[])
{
    double backlight;

    switch (argc) {
    case 0:
	backlight = drv_BuE_CT_backlight(-1);
	SetResult(&result, R_NUMBER, &backlight);
	break;
    case 1:
	backlight = drv_BuE_CT_backlight(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &backlight);
	break;
    default:
	error("%s::backlight(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
}


static void plugin_gpo(RESULT * result, const int argc, RESULT * argv[])
{
    double gpo;

    switch (argc) {
    case 1:
	gpo = drv_BuE_CT_gpo(R2N(argv[0]), -1);
	SetResult(&result, R_NUMBER, &gpo);
	break;
    case 2:
	gpo = drv_BuE_CT_gpo(R2N(argv[0]), R2N(argv[1]));
	SetResult(&result, R_NUMBER, &gpo);
	break;
    default:
	error("%s::gpo(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
}


static void plugin_gpi(RESULT * result, RESULT * arg1)
{
    double gpi;

    gpi = drv_BuE_CT_gpi(R2N(arg1));
    SetResult(&result, R_NUMBER, &gpi);
}


static void plugin_adc(RESULT * result)
{
    double adc;

    adc = drv_BuE_CT_adc();
    SetResult(&result, R_NUMBER, &adc);
}


static void plugin_pwm(RESULT * result, const int argc, RESULT * argv[])
{
    double pwm;

    switch (argc) {
    case 0:
	pwm = drv_BuE_CT_pwm(-1);
	SetResult(&result, R_NUMBER, &pwm);
	break;
    case 1:
	pwm = drv_BuE_CT_pwm(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &pwm);
	break;
    default:
	error("%s::pwm(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
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
int drv_BuE_list(void)
{
    int i;

    for (i = 0; Models[i].name != NULL; i++) {
	printf("%s ", Models[i].name);
    }
    return 0;
}


/* initialize driver & display */
int drv_BuE_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* start display */
    if ((ret = drv_BuE_start(section)) != 0) {
	return ret;
    }

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */

    /* real worker functions */
    switch (Protocol) {
    case 1:
	CHAR0 = 0;		/* ASCII of first user-defineable char */
	GOTO_COST = 6;		/* number of bytes a goto command requires */
	drv_generic_text_real_write = drv_BuE_MT_write;
	drv_generic_text_real_defchar = drv_BuE_MT_defchar;
	break;
    case 2:
	CHAR0 = 128;		/* ASCII of first user-defineable char */
	GOTO_COST = 6;		/* number of bytes a goto command requires */
	drv_generic_text_real_write = drv_BuE_CT_write;
	drv_generic_text_real_defchar = drv_BuE_CT_defchar;
	break;
    }

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %s", Name, Models[Model].name);
	if (drv_generic_text_greet(buffer, "www.bue.com")) {
	    sleep(3);
	    drv_BuE_clear();
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
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    drv_generic_text_bar_add_segment(255, 255, 255, 255);	/* ASCII 255 = block */

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
    AddFunction("LCD::contrast", -1, plugin_contrast);
    AddFunction("LCD::backlight", -1, plugin_backlight);
    AddFunction("LCD::gpo", -1, plugin_gpo);
    AddFunction("LCD::gpi", 1, plugin_gpi);
    AddFunction("LCD::adc", 0, plugin_adc);
    AddFunction("LCD::pwm", -1, plugin_pwm);

    return 0;
}


/* close driver & display */
int drv_BuE_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display  */
    drv_BuE_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_generic_serial_close();

    return (0);
}


DRIVER drv_BeckmannEgle = {
  name:Name,
  list:drv_BuE_list,
  init:drv_BuE_init,
  quit:drv_BuE_quit,
};
