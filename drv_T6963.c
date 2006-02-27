/* $Id: drv_T6963.c,v 1.20 2006/02/27 06:14:46 reinelt Exp $
 *
 * new style driver for T6963-based displays
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
 * $Log: drv_T6963.c,v $
 * Revision 1.20  2006/02/27 06:14:46  reinelt
 * graphic bug resulting in all black pixels solved
 *
 * Revision 1.19  2006/02/08 04:55:05  reinelt
 * moved widget registration to drv_generic_graphic
 *
 * Revision 1.18  2006/01/30 06:25:54  reinelt
 * added CVS Revision
 *
 * Revision 1.17  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 * Revision 1.16  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.15  2005/05/05 08:36:12  reinelt
 * changed SELECT to SLCTIN
 *
 * Revision 1.14  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.13  2004/12/22 20:24:02  reinelt
 * T6963 fix for displays > 8 rows
 *
 * Revision 1.12  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.11  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.10  2004/06/20 10:09:54  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.9  2004/06/17 06:23:39  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.8  2004/06/09 06:40:29  reinelt
 *
 * splash screen for T6963 driver
 *
 * Revision 1.7  2004/06/06 06:51:59  reinelt
 *
 * do not display end splash screen if quiet=1
 *
 * Revision 1.6  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.5  2004/06/01 06:45:30  reinelt
 *
 * some Fixme's processed
 * documented some code
 *
 * Revision 1.4  2004/02/24 05:55:04  reinelt
 *
 * X11 driver ported
 *
 * Revision 1.3  2004/02/22 17:35:41  reinelt
 * some fixes for generic graphic driver and T6963
 * removed ^M from plugin_imon (Nico, are you editing under Windows?)
 *
 * Revision 1.2  2004/02/18 06:39:20  reinelt
 * T6963 driver for graphic displays finished
 *
 * Revision 1.1  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_T6963
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
#include "drv.h"
#include "drv_generic_graphic.h"
#include "drv_generic_parport.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[] = "T6963";
static int Model;

typedef struct {
    int type;
    char *name;
} MODEL;

static MODEL Models[] = {
    {0x01, "generic"},
    {0xff, "Unknown"}
};


static unsigned char SIGNAL_CE;
static unsigned char SIGNAL_CD;
static unsigned char SIGNAL_RD;
static unsigned char SIGNAL_WR;

unsigned char *Buffer1, *Buffer2;

static int bug = 0;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/* perform normal status check */
static void drv_T6_status1(void)
{
    int n;

    /* turn off data line drivers */
    drv_generic_parport_direction(1);

    /* lower CE and RD */
    drv_generic_parport_control(SIGNAL_CE | SIGNAL_RD, 0);

    /* Access Time: 150 ns  */
    ndelay(150);

    /* wait for STA0=1 and STA1=1 */
    n = 0;
    do {
	rep_nop();
	if (++n > 1000) {
	    debug("hang in status1");
	    bug = 1;
	    break;
	}
    } while ((drv_generic_parport_read() & 0x03) != 0x03);

    /* rise RD and CE */
    drv_generic_parport_control(SIGNAL_RD | SIGNAL_CE, SIGNAL_RD | SIGNAL_CE);

    /* Output Hold Time: 50 ns  */
    ndelay(50);

    /* turn on data line drivers */
    drv_generic_parport_direction(0);
}


/* perform status check in "auto mode" */
static void drv_T6_status2(void)
{
    int n;

    /* turn off data line drivers */
    drv_generic_parport_direction(1);

    /* lower RD and CE */
    drv_generic_parport_control(SIGNAL_RD | SIGNAL_CE, 0);

    /* Access Time: 150 ns  */
    ndelay(150);

    /* wait for STA3=1 */
    n = 0;
    do {
	rep_nop();
	if (++n > 1000) {
	    debug("hang in status2");
	    bug = 1;
	    break;
	}
    } while ((drv_generic_parport_read() & 0x08) != 0x08);

    /* rise RD and CE */
    drv_generic_parport_control(SIGNAL_RD | SIGNAL_CE, SIGNAL_RD | SIGNAL_CE);

    /* Output Hold Time: 50 ns  */
    ndelay(50);

    /* turn on data line drivers */
    drv_generic_parport_direction(0);
}


static void drv_T6_write_cmd(const unsigned char cmd)
{
    /* wait until the T6963 is idle */
    drv_T6_status1();

    /* put data on DB1..DB8 */
    drv_generic_parport_data(cmd);

    /* lower WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, 0);

    /* Pulse width */
    ndelay(80);

    /* rise WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

    /* Data Hold Time */
    ndelay(40);
}


static void drv_T6_write_data(const unsigned char data)
{
    /* wait until the T6963 is idle */
    drv_T6_status1();

    /* put data on DB1..DB8 */
    drv_generic_parport_data(data);

    /* lower C/D */
    drv_generic_parport_control(SIGNAL_CD, 0);

    /* C/D Setup Time */
    ndelay(20);

    /* lower WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, 0);

    /* Pulse Width */
    ndelay(80);

    /* rise WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

    /* Data Hold Time */
    ndelay(40);

    /* rise CD */
    drv_generic_parport_control(SIGNAL_CD, SIGNAL_CD);
}


static void drv_T6_write_auto(const unsigned char data)
{
    /* wait until the T6963 is idle */
    drv_T6_status2();

    /* put data on DB1..DB8 */
    drv_generic_parport_data(data);

    /* lower C/D */
    drv_generic_parport_control(SIGNAL_CD, 0);

    /* C/D Setup Time */
    ndelay(20);

    /* lower WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, 0);

    /* Pulse Width */
    ndelay(80);

    /* rise WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

    /* Data Hold Time */
    ndelay(40);

    /* rise CD */
    drv_generic_parport_control(SIGNAL_CD, SIGNAL_CD);
}


#if 0				/* not used */
static void drv_T6_send_byte(const unsigned char cmd, const unsigned char data)
{
    drv_T6_write_data(data);
    drv_T6_write_cmd(cmd);
}
#endif

static void drv_T6_send_word(const unsigned char cmd, const unsigned short data)
{
    drv_T6_write_data(data & 0xff);
    drv_T6_write_data(data >> 8);
    drv_T6_write_cmd(cmd);
}


static void drv_T6_clear(const unsigned short addr, const int len)
{
    int i;

    drv_T6_send_word(0x24, addr);	/* Set Adress Pointer */
    drv_T6_write_cmd(0xb0);	/* Set Data Auto Write */
    for (i = 0; i < len; i++) {
	drv_T6_write_auto(0);
	if (bug) {
	    bug = 0;
	    debug("bug occured at byte %d of %d", i, len);
	}
    }
    drv_T6_status2();
    drv_T6_write_cmd(0xb2);	/* Auto Reset */
}


static void drv_T6_copy(const unsigned short addr, const unsigned char *data, const int len)
{
    int i;

    drv_T6_send_word(0x24, 0x0200 + addr);	/* Set Adress Pointer */
    drv_T6_write_cmd(0xb0);	/* Set Data Auto Write */
    for (i = 0; i < len; i++) {
	drv_T6_write_auto(*(data++));
	if (bug) {
	    bug = 0;
	    debug("bug occured at byte %d of %d, addr=%d", i, len, addr);
	}
    }
    drv_T6_status2();
    drv_T6_write_cmd(0xb2);	/* Auto Reset */
}


static void drv_T6_blit(const int row, const int col, const int height, const int width)
{
    int i, j, e, m;
    int r, c;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    unsigned char mask = 1 << (XRES - 1 - c % XRES);
	    if (drv_generic_graphic_black(r, c)) {
		/* set bit */
		Buffer1[(r * DCOLS + c) / XRES] |= mask;
	    } else {
		/* clear bit */
		Buffer1[(r * DCOLS + c) / XRES] &= ~mask;
	    }
	}
    }

    /* upper half */

    /* max address */
    if (row + height - 1 < 64) {
	m = ((row + height - 1) * DCOLS + col + width) / XRES;
    } else {
	m = (64 * DCOLS + col + width) / XRES;
    }

    for (i = (row * DCOLS + col) / XRES; i <= m; i++) {
	if (Buffer1[i] == Buffer2[i])
	    continue;
	for (j = i, e = 0; i <= m; i++) {
	    if (Buffer1[i] == Buffer2[i]) {
		if (++e > 4)
		    break;
	    } else {
		e = 0;
	    }
	}
	memcpy(Buffer2 + j, Buffer1 + j, i - j - e + 1);
	drv_T6_copy(j, Buffer1 + j, i - j - e + 1);
    }

    /* lower half */

    /* max address */
    m = ((row + height - 1) * DCOLS + col + width) / XRES;

    for (i = (64 * DCOLS + col) / XRES; i <= m; i++) {
	if (Buffer1[i] == Buffer2[i])
	    continue;
	for (j = i, e = 0; i <= m; i++) {
	    if (Buffer1[i] == Buffer2[i]) {
		if (++e > 4)
		    break;
	    } else {
		e = 0;
	    }
	}
	memcpy(Buffer2 + j, Buffer1 + j, i - j - e + 1);
	drv_T6_copy(j, Buffer1 + j, i - j - e + 1);
    }
}


static int drv_T6_start(const char *section)
{
    char *model, *s;
    int rows, TROWS, TCOLS;

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
	info("%s: using model '%s'", Name, Models[Model].name);
    } else {
	error("%s: empty '%s.Model' entry from %s", Name, section, cfg_source());
	return -1;
    }

    /* read display size from config */
    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }

    DROWS = -1;
    DCOLS = -1;
    if (sscanf(s, "%dx%d", &DCOLS, &DROWS) != 2 || DCOLS < 1 || DROWS < 1) {
	error("%s: bad Size '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    TROWS = DROWS / YRES;	/* text rows */
    TCOLS = DCOLS / XRES;	/* text cols */

    Buffer1 = malloc(TCOLS * DROWS);
    if (Buffer1 == NULL) {
	error("%s: framebuffer #1 could not be allocated: malloc() failed", Name);
	return -1;
    }

    debug("malloc buffer 2 (%d*%d)=%d", TCOLS, DROWS, TCOLS * DROWS);
    Buffer2 = malloc(TCOLS * DROWS);
    if (Buffer2 == NULL) {
	error("%s: framebuffer #2 could not be allocated: malloc() failed", Name);
	return -1;
    }

    memset(Buffer1, 0, TCOLS * DROWS * sizeof(*Buffer1));
    memset(Buffer2, 0, TCOLS * DROWS * sizeof(*Buffer2));

    if (drv_generic_parport_open(section, Name) != 0) {
	error("%s: could not initialize parallel port!", Name);
	return -1;
    }

    if ((SIGNAL_CE = drv_generic_parport_wire_ctrl("CE", "STROBE")) == 0xff)
	return -1;
    if ((SIGNAL_CD = drv_generic_parport_wire_ctrl("CD", "SLCTIN")) == 0xff)
	return -1;
    if ((SIGNAL_RD = drv_generic_parport_wire_ctrl("RD", "AUTOFD")) == 0xff)
	return -1;
    if ((SIGNAL_WR = drv_generic_parport_wire_ctrl("WR", "INIT")) == 0xff)
	return -1;

    /* rise CE, CD, RD and WR */
    drv_generic_parport_control(SIGNAL_CE | SIGNAL_CD | SIGNAL_RD | SIGNAL_WR,
				SIGNAL_CE | SIGNAL_CD | SIGNAL_RD | SIGNAL_WR);
    /* set direction: write */
    drv_generic_parport_direction(0);


    /* initialize display */

    drv_T6_send_word(0x40, 0x0000);	/* Set Text Home Address */
    drv_T6_send_word(0x41, TCOLS);	/* Set Text Area */

    drv_T6_send_word(0x42, 0x0200);	/* Set Graphic Home Address */
    drv_T6_send_word(0x43, TCOLS);	/* Set Graphic Area */

    drv_T6_write_cmd(0x80);	/* Mode Set: OR mode, Internal CG RAM mode */
    drv_T6_send_word(0x22, 0x0002);	/* Set Offset Register */
    drv_T6_write_cmd(0x98);	/* Set Display Mode: Curser off, Text off, Graphics on */
    drv_T6_write_cmd(0xa0);	/* Set Cursor Pattern: 1 line cursor */
    drv_T6_send_word(0x21, 0x0000);	/* Set Cursor Pointer to (0,0) */


    /* clear display */

    /* upper half */
    rows = TROWS > 8 ? 8 : TROWS;
    drv_T6_clear(0x0000, TCOLS * rows);	/* clear text area  */
    drv_T6_clear(0x0200, TCOLS * rows * 8);	/* clear graphic area */

    /* lower half */
    if (TROWS > 8) {
	rows = TROWS - 8;
	drv_T6_clear(0x8000, TCOLS * rows);	/* clear text area #2 */
	drv_T6_clear(0x8200, TCOLS * rows * 8);	/* clear graphic area #2 */
    }

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_T6_list(void)
{
    int i;

    for (i = 0; Models[i].type != 0xff; i++) {
	printf("%s ", Models[i].name);
    }
    return 0;
}


/* initialize driver & display */
int drv_T6_init(const char *section, const int quiet)
{
    int ret;

    info("%s: %s", Name, "$Revision: 1.20 $");

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_T6_blit;

    /* start display */
    if ((ret = drv_T6_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    /* register plugins */
    /* none at the moment... */


    return 0;
}


/* close driver & display */
int drv_T6_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_graphic_clear();

    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();
    drv_generic_parport_close();

    if (Buffer1) {
	free(Buffer1);
	Buffer1 = NULL;
    }

    if (Buffer2) {
	free(Buffer2);
	Buffer2 = NULL;
    }

    return (0);
}


DRIVER drv_T6963 = {
  name:Name,
  list:drv_T6_list,
  init:drv_T6_init,
  quit:drv_T6_quit,
};
