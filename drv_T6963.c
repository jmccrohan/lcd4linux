/* $Id$
 * $URL$
 *
 * new style driver for T6963-based displays
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
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
    {0x01, "T6963"},
    {0xff, "Unknown"}
};


/* font width of display */
static int CELL;

/* text rows/columns */
static int TROWS, TCOLS;

/* SingleScan or DualScan */
static int DualScan = 0;

/* Timings */
static int T_ACC, T_OH, T_PW, T_DH, T_CDS;

/* soft-wiring */
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

    /* Access Time */
    ndelay(T_ACC);

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

    /* Output Hold Time */
    ndelay(T_OH);

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

    /* Access Time */
    ndelay(T_ACC);

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

    /* Output Hold Time */
    ndelay(T_OH);

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
    ndelay(T_PW);

    /* rise WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

    /* Data Hold Time */
    ndelay(T_DH);
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
    ndelay(T_CDS);

    /* lower WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, 0);

    /* Pulse Width */
    ndelay(T_PW);

    /* rise WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

    /* Data Hold Time */
    ndelay(T_DH);

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
    ndelay(T_CDS);

    /* lower WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, 0);

    /* Pulse Width */
    ndelay(T_PW);

    /* rise WR and CE */
    drv_generic_parport_control(SIGNAL_WR | SIGNAL_CE, SIGNAL_WR | SIGNAL_CE);

    /* Data Hold Time */
    ndelay(T_DH);

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
	drv_T6_write_auto(0x0);
	if (bug) {
	    bug = 0;
	    debug("bug occurred at byte %d of %d", i, len);
	}
    }
    drv_T6_status2();
    drv_T6_write_cmd(0xb2);	/* Auto Reset */
}


static void drv_T6_copy(const unsigned short addr, const unsigned char *data, const int len)
{
    int i;

    drv_T6_send_word(0x24, addr);	/* Set Adress Pointer */
    drv_T6_write_cmd(0xb0);	/* Set Data Auto Write */
    for (i = 0; i < len; i++) {
	drv_T6_write_auto(*(data++));
	if (bug) {
	    bug = 0;
	    debug("bug occurred at byte %d of %d, addr=%d", i, len, addr);
	}
    }
    drv_T6_status2();
    drv_T6_write_cmd(0xb2);	/* Auto Reset */
}


static void drv_T6_blit(const int row, const int col, const int height, const int width)
{
    int r, c, a, b;
    int i, j, e, n;
    int base;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    unsigned char mask = 1 << (CELL - 1 - c % CELL);
	    if (drv_generic_graphic_black(r, c)) {
		/* set bit */
		Buffer1[(r * DCOLS + c) / CELL] |= mask;
	    } else {
		/* clear bit */
		Buffer1[(r * DCOLS + c) / CELL] &= ~mask;
	    }
	}
	a = (r * DCOLS + col) / CELL;
	b = (r * DCOLS + col + width + CELL - 1) / CELL;
	for (i = a; i <= b; i++) {
	    if (Buffer1[i] == Buffer2[i])
		continue;
	    for (j = i, e = 0; i <= b; i++) {
		if (Buffer1[i] == Buffer2[i]) {
		    if (++e > 4)
			break;
		} else {
		    e = 0;
		}
	    }
	    if (DualScan && r >= DROWS / 2) {
		base = 0x8200 - DCOLS * DROWS / 2 / CELL;
	    } else {
		base = 0x0200;
	    }
	    n = i - j - e + 1;
	    memcpy(Buffer2 + j, Buffer1 + j, n);
	    drv_T6_copy(base + j, Buffer1 + j, n);
	}
    }
}


static int drv_T6_start(const char *section)
{
    char *model, *s;

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

    if (sscanf(s = cfg_get(section, "font", "6x8"), "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad %s.Font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);


    /* get font width of display */
    cfg_number(section, "Cell", 6, 5, 8, &CELL);

    TROWS = DROWS / 8;		/* text rows */
    TCOLS = DCOLS / CELL;	/* text columns */

    /* get DualScan mode */
    cfg_number(section, "DualScan", 0, 0, 1, &DualScan);

    info("%s: %dx%d %sScan %d bits/cell", Name, DCOLS, DROWS, DualScan ? "Dual" : "Single", CELL);

    Buffer1 = malloc(TCOLS * DROWS);
    if (Buffer1 == NULL) {
	error("%s: framebuffer #1 could not be allocated: malloc() failed", Name);
	return -1;
    }

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

    /* soft-wiring */
    if ((SIGNAL_CE = drv_generic_parport_wire_ctrl("CE", "STROBE")) == 0xff)
	return -1;
    if ((SIGNAL_CD = drv_generic_parport_wire_ctrl("CD", "SLCTIN")) == 0xff)
	return -1;
    if ((SIGNAL_RD = drv_generic_parport_wire_ctrl("RD", "AUTOFD")) == 0xff)
	return -1;
    if ((SIGNAL_WR = drv_generic_parport_wire_ctrl("WR", "INIT")) == 0xff)
	return -1;

    /* timings */
    T_ACC = timing(Name, section, "ACC", 150, "ns");	/* Access Time */
    T_OH = timing(Name, section, "OH", 50, "ns");	/* Output Hold Time */
    T_PW = timing(Name, section, "PW", 80, "ns");	/* CE, RD, WR Pulse Width */
    T_DH = timing(Name, section, "DH", 40, "ns");	/* Data Hold Time */
    T_CDS = timing(Name, section, "CDS", 100, "ns");	/* C/D Setup Time */


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

    if (DualScan) {
	/* upper half */
	drv_T6_clear(0x0000, TCOLS * TROWS / 2);	/* clear text area  */
	drv_T6_clear(0x0200, TCOLS * TROWS * 8 / 2);	/* clear graphic area */
	/* lower half */
	drv_T6_clear(0x8000, TCOLS * TROWS / 2);	/* clear text area  */
	drv_T6_clear(0x8200, TCOLS * TROWS * 8 / 2);	/* clear graphic area */
    } else {
	drv_T6_clear(0x0000, TCOLS * TROWS);	/* clear text area  */
	drv_T6_clear(0x0200, TCOLS * TROWS * 8);	/* clear graphic area */
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

    info("%s: %s", Name, "$Rev$");

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
    .name = Name,
    .list = drv_T6_list,
    .init = drv_T6_init,
    .quit = drv_T6_quit,
};
