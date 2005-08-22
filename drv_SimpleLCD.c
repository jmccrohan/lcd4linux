/* $Id: drv_SimpleLCD.c,v 1.6 2005/08/22 05:44:43 reinelt Exp $
 * 
 * driver for a simple serial terminal.
 * 
 * Copyright (C) 2005 Julien Aube <ob@obconseil.net>
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
 * $Log: drv_SimpleLCD.c,v $
 * Revision 1.6  2005/08/22 05:44:43  reinelt
 * new driver 'WincorNixdorf'
 * some fixes to the bar code
 *
 * Revision 1.5  2005/07/06 04:40:18  reinelt
 * GCC-4 fixes
 *
 * Revision 1.4  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.3  2005/04/20 05:49:21  reinelt
 * Changed the code to add some VT100-compatible control sequences (see the comments above).
 * A configfile boolean option 'VT100_Support' (default to 1) indicate if the display in
 * used support these control-sequences or not.
 *
 * Revision 1.2  2005/04/02 05:28:58  reinelt
 * fixed gcc4 warnings about signed/unsigned mismatches
 *
 * Revision 1.1  2005/02/24 07:06:48  reinelt
 * SimpleLCD driver added
 *
 */

/*
 * This driver simply send out caracters on the serial port, without any 
 * formatting instructions for a particular LCD device. 
 * This is useful for custom boards of for very simple LCD.
 *
 * I use it for tests on a custom-made board based on a AVR microcontroler
 * and also for driver a Point-of-Sale text-only display.
 * I assume the following :
 * - CR (0x0d) Return to the begining of the line without erasing,
 * - LF (0x0a) Initiate a new line (but without sending the cursor to 
 * the begining of the line)
 * - BS (0x08) Move the cursor to the previous caracter (but does no erase it).
 * - It's not possible to return to the first line. Thus a back buffer is used
 * in this driver.
 * 
 * ******** UPDATE *********
 * I have added a "VT-100 Compatible mode" that allows the driver to support
 * control-sequence code. This greatly reduce flickering and eliminate the need
 * for the back-buffer. But it is optional since all displays cannot support them.
 * Here are the codes:
 * Delete the display (but does not move the cursor) : 
 *    "ESC [ 2 J"     (0x1b 0x5b 0x32 0x4a)
 * Position the cursor :
 *    "ESC [ YY ; XX H" ( 0x1b 0x5b YY 0x3b XX 0x48 ) where YY is the ascii for the line 
 *    number, and XX is the ascii for the column number ( first line/column is '1', not zero)
 * Delete to the end of line from current cursor position :
 *    "ESC [ 0 K" ( 0x1b 0x5b 0x30 0x4b )
 * Set Country Code : 
 *    "ESC R NN"  (0x1b 0x52 NN) where NN is the country code *in byte, NOT ascii*.
 *    The default is 0 (USA), see below for specific countries. 
 *    the list of accessible characters page are available on this page : 
 *    http://www.wincor-nixdorf.com/internet/com/Services/Support/TechnicalSupport/POSSystems
 *    /Manuals/BAxx/index.html
 * Get the display identification : (Doesn't work reliably, timing issues here)
 *    "ESC [ 0 c" ( 0x1b 0x5b 0x30 0x63). Return a string which look like this : 
 *  ESC [ ? M ; NN ; OO ; PP ; QQ c) where M is type of display (2 for VFD),
 *  NN is the rom version, 00 is the current caracter set, PP is the number of lines and 
 *  QQ the number of colomns.  
 * 
 *
 * A "bar" capability is now provided if the config file has a "BarCharValue" parameter in it.
 *
 * The code come mostly taken from the LCDTerm driver in LCD4Linux, from 
 * Michaels Reinelt, many thanks to him.
 *
 * This driver is released under the GPL.
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_SimpleLCD
 *
 */

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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


static char Name[] = "SimpleLCD";
static char *backbuffer = 0;
static int backbuffer_size = 0;
static int vt100_mode = 0;
static unsigned char bar_char = 0;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/** No clear function on SimpleLCD : Just send CR-LF * number of lines **/
static void drv_SL_simple_clear(void)
{
    char cmd[2];
    int i;
    cmd[0] = '\r';
    cmd[1] = '\n';
    for (i = 0; i < DROWS; ++i) {
	drv_generic_serial_write(cmd, 2);
    }
    memset(backbuffer, ' ', backbuffer_size);
}

/** vt-100 mode : send the ESC-code **/
static void drv_SL_vt100_clear(void)
{
    char cmd[4];
    cmd[0] = 0x1b;
    cmd[1] = '[';
    cmd[2] = '2';
    cmd[3] = 'J';
    drv_generic_serial_write(cmd, 4);

}

static void drv_SL_clear(void)
{
    vt100_mode == 1 ? drv_SL_vt100_clear() : drv_SL_simple_clear();
}


/* If full_commit = true, then the whole buffer is to be sent to screen.
   if full_commit = false, then only the last line is to be sent (faster on slow screens)
*/

static void drv_SL_commit(int full_commit)
{
    int row;
    char cmd[2] = { '\r', '\n' };
    if (full_commit) {
	for (row = 0; row < DROWS; row++) {
	    drv_generic_serial_write(cmd, 2);
	    drv_generic_serial_write(backbuffer + (DCOLS * row), DCOLS);
	}
    } else {
	drv_generic_serial_write(cmd, 1);	/* Go to the beginning of the line only */
	drv_generic_serial_write(backbuffer + (DCOLS * (DROWS - 1)), DCOLS);
    }
}

static void drv_SL_simple_write(const int row, const int col, const char *data, int len)
{
    memcpy(backbuffer + (row * DCOLS) + col, data, len);
    if (row == DROWS - 1)
	drv_SL_commit(0);
    else
	drv_SL_commit(1);
}

static void drv_SL_vt100_write(const int row, const int col, const char *data, int len)
{
    char cmd[8];
    cmd[0] = 0x1b;
    cmd[1] = '[';
    cmd[2] = row + '1';
    cmd[3] = ';';
    cmd[4] = (col / 10) + '0';
    cmd[5] = (col % 10) + '1';
    cmd[6] = 'H';
    drv_generic_serial_write(cmd, 7);
    drv_generic_serial_write(data, len);
}


static int drv_SL_start(const char *section, const int quiet)
{
    int rows = -1, cols = -1;
    int value;
    unsigned int flags = 0;
    char *s;
    char *model = 0;

    vt100_mode = 0;
    model = cfg_get(section, "Model", "generic");
    if (model != NULL && *model != '\0') {
	if (strcasecmp("vt100", model) == 0)
	    vt100_mode = 1;
    }

    cfg_number(section, "BarCharValue", 0, 0, 255, &value);
    bar_char = value;
    cfg_number(section, "Options", 0, 0, 0xffff, &value);
    flags = value;
    if (drv_generic_serial_open(section, Name, flags) < 0)
	return -1;

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

    if (!vt100_mode) {
	backbuffer_size = DROWS * DCOLS;
	backbuffer = malloc(backbuffer_size);
	if (!backbuffer) {
	    return -1;
	}
    }

    /* real worker functions */
    if (vt100_mode) {
	drv_generic_text_real_write = drv_SL_vt100_write;
    } else {
	drv_generic_text_real_write = drv_SL_simple_write;
    }

    drv_SL_clear();		/* clear */


    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, NULL)) {
	    sleep(3);
	    drv_SL_clear();
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
int drv_SL_list(void)
{
    printf("generic vt100");
    return 0;
}


/* initialize driver & display */
int drv_SL_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */

    GOTO_COST = -1;		/* number of bytes a goto command requires */


    /* start display */
    if ((ret = drv_SL_start(section, quiet)) != 0)
	return ret;

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register plugins */
    /* none */

    return 0;
}


/* close driver & display */
int drv_SL_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_SL_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_generic_serial_close();

    if (backbuffer) {
	free(backbuffer);
	backbuffer = 0;
	backbuffer_size = 0;
    }
    return (0);
}


DRIVER drv_SimpleLCD = {
  name:Name,
  list:drv_SL_list,
  init:drv_SL_init,
  quit:drv_SL_quit,
};
