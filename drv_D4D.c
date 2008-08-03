/* $Id: drv_D4D.c 840 2007-09-09 12:17:42Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_D4D.c $
 *
 * lcd4linux driver for 4D Systems serial displays
 *
 * Copyright (C) 2008 Sven Killig <sven@killig.de>
 * Modified from sample code by:
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_D4D
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <termios.h>
#include <fcntl.h>
#include <math.h>

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
#include "drv_generic_graphic.h"
#include "drv_generic_serial.h"

static char Name[] = "D4D";
char NAME_[20];
int FONT = 1, MODE = 0, EXTRA = 0, SECTOR = 0, SECTOR_SIZE, NOPOWERCYCLE = 0;
/* int CONTRAST_; */

#define address_mmsb(variable) ((variable >> 24) & 0xFF)
#define address_mlsb(variable) ((variable >> 16) & 0xFF)
#define address_lmsb(variable) ((variable >>  8) & 0xFF)
#define address_llsb(variable)  (variable        & 0xFF)

#define address_hi(variable)   ((variable >> 16) & 0xFF)
#define address_mid(variable)  ((variable >>  8) & 0xFF)
#define address_lo(variable)    (variable        & 0xFF)

#define msb(variable)          ((variable >>  8) & 0xFF)
#define lsb(variable)           (variable        & 0xFF)

RGBA FG_COL_ = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0xff };
RGBA BG_COL_ = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0xff };

short int FG_COLOR, BG_COLOR;

int RGB_24to16(int red, int grn, int blu)
{
    return (((red >> 3) << 11) | ((grn >> 2) << 5) | (blu >> 3));
}

int RGB_24to8(int red, int grn, int blu)
{
    return (((red >> 5) << 5) | ((grn >> 5) << 2) | (blu >> 6));
}

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


static int drv_D4D_open(const char *section)
{
    int fd;
    fd = drv_generic_serial_open(section, Name, 0);
    if (fd < 0)
	return -1;
    fcntl(fd, F_SETFL, 0);
    return 0;
}


static int drv_D4D_close(void)
{
    drv_generic_serial_close();
    return 0;
}


static void drv_D4D_receive_ACK()
{
    char ret[1];
    while (drv_generic_serial_read(ret, sizeof(ret)) != 1) {
	usleep(1); /* loop should be unneccessary */
    }
    /* drv_generic_serial_poll(ret,1); */
    if (ret[0] == 0x15) {
	error("NAK!");
    } else if (ret[0] != 6) {
	error("no ACK!");
    }
}

static void drv_D4D_send_nowait(const char *data, const unsigned int len)
{
    drv_generic_serial_write(data, len);
}

static void drv_D4D_send(const char *data, const unsigned int len)
{
    drv_D4D_send_nowait(data, len);
    drv_D4D_receive_ACK();
}

static void drv_D4D_send_nowait_extra(const char *data, const unsigned int len, unsigned char pos1, unsigned char pos2)
{
    if (EXTRA) {
	drv_D4D_send_nowait(data, len);
    } else {
	unsigned int i;
	char send[1];
	for (i = 0; i < len; i++) {
	    if (!pos1 || i != pos1) {
		if (!pos2 || i != pos2) {
		    send[0] = data[i];
		    drv_generic_serial_write(send, 1);
		}
	    }
	}
    }
}

static void drv_D4D_send_extra(const char *data, const unsigned int len, char pos1, char pos2)
{
    drv_D4D_send_nowait_extra(data, len, pos1, pos2);
    drv_D4D_receive_ACK();
}


static void drv_D4D_clear(void)
{
    char cmd[] = { 'E' };
    drv_D4D_send(cmd, sizeof(cmd));
}


static void drv_D4D_write(const int row, const int col, const char *data, int len)
{
    char out[len];
    char user_char[len];
    int user_x[len];
    int user_y[len];
    int i, k = 0;

    if (!SECTOR) {

	for (i = 0; i < len; i++) {
	    if (data[i] >= 31 && data[i] < CHAR0) {
		out[i] = data[i];
	    } else if (data[i] == 'µ') {	/* mu */
		if (FONT == 2)
		    out[i] = 31;	/* undocumented */
		else if (FONT == 0)
		    out[i] = 'u';
		else
		    out[i] = 127;	/* undocumented */
	    } else {
		out[i] = ' ';
		if (data[i] == '°')
		    user_char[k] = 0;	/* degree */
		else
		    user_char[k] = data[i] - CHAR0;
		user_x[k] = (col + i) * XRES;
		user_y[k] = row * YRES;
		k++;
	    }
	}

	char cmd[] = { 's', col, row, FONT, msb(FG_COLOR), lsb(FG_COLOR) };
	drv_D4D_send_nowait(cmd, sizeof(cmd));
	if (len > 256)
	    len = 256;
	drv_D4D_send_nowait(out, len);
	char cmdNull[1];
	cmdNull[0] = 0;
	drv_D4D_send(cmdNull, 1);

	char cmd_user[] = { 'D', 0, 0, 0, 0, 0, msb(FG_COLOR), lsb(FG_COLOR) };
	for (i = 0; i < k; i++) {
	    cmd_user[2] = user_char[i];
	    cmd_user[3] = user_x[i];
	    cmd_user[4] = msb(user_y[i]);
	    cmd_user[5] = lsb(user_y[i]);
	    drv_D4D_send_extra(cmd_user, sizeof(cmd_user), 1, 4);
	}
    } else {
	int sec;
	char cmd_sd[] = { '@', 'I', 0, msb(row * YRES), lsb(row * YRES), XRES, msb(YRES), lsb(YRES), 16, 0, 0, 0 };
	for (i = 0; i < len; i++) {
	    cmd_sd[2] = (col + i) * XRES;
	    sec = SECTOR + (unsigned char) data[i] * SECTOR_SIZE;
	    cmd_sd[9] = address_hi(sec);
	    cmd_sd[10] = address_mid(sec);
	    cmd_sd[11] = address_lo(sec);
	    drv_D4D_send_extra(cmd_sd, sizeof(cmd_sd), 3, 6);
	}
    }
}


static void drv_D4D_defchar(const int ascii, const unsigned char *matrix)
{
    /* error("drv_D4D_defchar"); */
    char cmd[11];
    int i;

    cmd[0] = 'A';
    cmd[1] = 0;
    cmd[2] = ascii - CHAR0;

    for (i = 0; i < 8; i++) {
	cmd[i + 3] = *matrix++;
    }
    drv_D4D_send_extra(cmd, sizeof(cmd), 1, 0);
}


static void drv_D4D_blit(const int row, const int col, const int height, const int width)
{
    /* error("drv_D4D_blit(%i, %i, %i, %i)",row, col, height, width); */
    int r, c;
    RGBA rgb, pixel0_0, pixel;
    short int color;
    char colorArray[2];


    pixel0_0 = drv_generic_graphic_rgb(0, 0);
    char unicolor = 1;
    for (r = row; r < row + height; r++) {
	if (!unicolor)
	    break;
	for (c = col; c < col + width; c++) {
	    if (!unicolor)
		break;
	    pixel = drv_generic_graphic_rgb(r, c);
	    if (pixel0_0.R != pixel.R || pixel0_0.G != pixel.G || pixel0_0.B != pixel.B || pixel0_0.A != pixel.A)
		unicolor = 0;
	}
    }

    if (unicolor) {
	color = RGB_24to16(pixel0_0.R, pixel0_0.G, pixel0_0.B);
	char row2 = row + height - 1;
	char cmdRect[] =
	    { 'r', col, msb(row), lsb(row), col + width - 1, msb(row2), lsb(row2), msb(color), lsb(color) };
	drv_D4D_send_extra(cmdRect, sizeof(cmdRect), 2, 5);
    } else {

	char cmd[] = { 'I', col, msb(row), lsb(row), width, msb(height), lsb(height), MODE };
	drv_D4D_send_nowait_extra(cmd, sizeof(cmd), 2, 5);
	for (r = row; r < row + height; r++) {
	    for (c = col; c < col + width; c++) {
		rgb = drv_generic_graphic_rgb(r, c);
		if (MODE == 8) {
		    colorArray[0] = RGB_24to8(rgb.R, rgb.G, rgb.B);
		    drv_D4D_send_nowait(colorArray, 1);
		} else {
		    color = RGB_24to16(rgb.R, rgb.G, rgb.B);
		    colorArray[0] = msb(color);
		    drv_D4D_send_nowait(colorArray, 1);	/* doesn't work if sent together (error: "partial write(/dev/tts/1): len=2 ret=1") */
		    /* colorArray[1]=lsb(color); */
		    colorArray[0] = lsb(color);
		    drv_D4D_send_nowait(colorArray, 1);
		}
		/* drv_D4D_send_nowait(colorArray, MODE/8); */
	    }
	}
	drv_D4D_receive_ACK();
    }
}


static int drv_D4D_contrast(int contrast)
{
    if (contrast < 0)
	contrast = 0;
    if (contrast > 15)
	contrast = 15;

    char cmd[] = { 'Y', 2, contrast };
    drv_D4D_send(cmd, sizeof(cmd));

    /* CONTRAST_ = contrast; */
    return contrast;
}


static int drv_D4D_start(const char *section)
{
    info("drv_D4D_start()");
    int contrast;
    int xres_cfg = -1, yres_cfg = -1;
    char *s;
    char *color;

    if (drv_D4D_open(section) < 0) {
	return -1;
    }

    char cmd[] = { 'U' };
    drv_D4D_send(cmd, sizeof(cmd));


    char getVersion[] = { 'V', 0 };
    drv_D4D_send_nowait(getVersion, sizeof(getVersion));
    char answer[5];
    drv_generic_serial_read(answer, sizeof(answer));
    char *ids[] = { "uOLED", "uLCD", "uVGA" };
    char *id;
    if (answer[0] <= 2)
	id = ids[(int) answer[0]];
    else
	id = "Unknown";
    qprintf(NAME_, sizeof(NAME_), "%s H%u S%u", id, (int) answer[1], (int) answer[2]);
    int res[2];
    int i;
    for (i = 0; i < 2; i++) {
	switch (answer[3 + i]) {
	case 0x22:
	    res[i] = 220;
	    break;
	case 0x28:
	    res[i] = 128;
	    break;
	case 0x32:
	    res[i] = 320;
	    EXTRA = 1;
	    break;
	case 0x60:
	    res[i] = 160;
	    break;
	case 0x64:
	    res[i] = 64;
	    break;
	case 0x76:
	    res[i] = 176;
	    break;
	case 0x96: /* Fixme: case label value exceeds maximum value for type */
	    res[i] = 96;
	    break;
	case 0x24:
	    res[i] = 240; /* undocumented? */
	    break;
	default:
	    error ("Fixme");
	}
    }
    DCOLS = res[0];
    DROWS = res[1];


    cfg_number(section, "Mode", 0, 0, 16, &MODE);

    s = cfg_get(section, "Font", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &xres_cfg, &yres_cfg) < 1 || xres_cfg < 0) {
	error("%s: bad %s.Font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }

    if (yres_cfg == -1) {
	SECTOR = xres_cfg * 512;
	char setAddress[] =
	    { '@', 'A', address_mmsb(SECTOR), address_mlsb(SECTOR), address_lmsb(SECTOR), address_llsb(SECTOR) };
	drv_D4D_send(setAddress, sizeof(setAddress));
	char answer1[1];
	char readByte[] = { '@', 'r' };
	drv_D4D_send_nowait(readByte, sizeof(readByte));
	drv_generic_serial_read(answer1, sizeof(answer1));
	XRES = answer1[0];
	drv_D4D_send_nowait(readByte, sizeof(readByte));
	drv_generic_serial_read(answer1, sizeof(answer1));
	YRES = answer1[0];
	SECTOR = xres_cfg + 1;
	SECTOR_SIZE = ceil((double) XRES * (double) YRES * 2.0 / 512.0);
    } else {
	XRES = xres_cfg;
	YRES = yres_cfg;
    }

    if (MODE == 0) {

	color = cfg_get(section, "foreground", "ffffff");
	if (color2RGBA(color, &FG_COL_) < 0) {
	    error("%s: ignoring illegal color '%s'", Name, color);
	}
	if (color)
	    free(color);
	FG_COLOR = RGB_24to16(FG_COL_.R, FG_COL_.G, FG_COL_.B);

	color = cfg_get(section, "background", "000000");
	if (color2RGBA(color, &BG_COL_) < 0) {
	    error("%s: ignoring illegal color '%s'", Name, color);
	}
	if (color)
	    free(color);
	BG_COLOR = RGB_24to16(BG_COL_.R, BG_COL_.G, BG_COL_.B);

	DCOLS = DCOLS / XRES;
	DROWS = DROWS / YRES;
	switch (yres_cfg) {
	case 8:
	    FONT = 1;
	    break;
	case 12:
	    FONT = 2;
	    break;
	case 16:
	    FONT = 3;
	    break;
	}
	if (xres_cfg == 5)
	    FONT = 0;
    }

    printf("XRES=%i; YRES=%i, DCOLS=%i; DROWS=%d; FONT=%d; SECTOR=%i; SECTOR_SIZE=%i\n", XRES, YRES, DCOLS, DROWS, FONT,
	   SECTOR, SECTOR_SIZE);



    cfg_number(section, "NoPowerCycle", 0, 1, 65536, &NOPOWERCYCLE);
    if (!NOPOWERCYCLE) {
	char powerOn[] = { 'Y', 3, 1 };
	drv_D4D_send(powerOn, sizeof(powerOn));

	/* char background[] = {'B', msb(BG_COLOR), lsb(BG_COLOR)};
	   drv_D4D_send(background, sizeof(background)); */
    }

    char opaque[] = { 'O', 1 };
    drv_D4D_send(opaque, sizeof(opaque));

    char penSize[] = { 'p', 0 };
    drv_D4D_send(penSize, sizeof(penSize));


    if (!MODE) {
	unsigned char buffer[] = { 0x18, 0x24, 0x24, 0x18, 0, 0, 0, 0 }; /* degree */
	drv_D4D_defchar(CHAR0, buffer);
	CHARS--;
	CHAR0++;
    }

    if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
	drv_D4D_contrast(contrast);
    }

    drv_D4D_clear();

    return 0;
}


int drv_D4D_text_draw(WIDGET * W)
{
    short int FG_COLOR_old = FG_COLOR, BG_COLOR_old = BG_COLOR;
    int ret;
    RGBA fg, bg;
    fg = W->fg_valid ? W->fg_color : FG_COL_;
    bg = W->bg_valid ? W->bg_color : BG_COL_;
    FG_COLOR = RGB_24to16(fg.R, fg.G, fg.B);
    BG_COLOR = RGB_24to16(bg.R, bg.G, bg.B);
    ret = drv_generic_text_draw(W);
    FG_COLOR = FG_COLOR_old;
    BG_COLOR = BG_COLOR_old;
    return ret;
}


int lastVal[48 * 40 * 2];	/* MAX_WIDGETS*2 */
int drv_D4D_bar_draw(WIDGET * W)
{
    WIDGET_BAR *Bar = W->data;
    int row, col, len, res, max, val1, val2;
    DIRECTION dir;
    STYLE style;

    row = W->row;
    col = W->col;
    dir = Bar->direction;
    style = Bar->style;
    len = Bar->length;

    res = dir & (DIR_EAST | DIR_WEST) ? XRES : YRES;
    max = len * res;
    val1 = Bar->val1 * (double) (max);
    val2 = Bar->val2 * (double) (max);

    if (val1 < 1)
	val1 = 1;
    else if (val1 > max)
	val1 = max;

    if (val2 < 1)
	val2 = 1;
    else if (val2 > max)
	val2 = max;

    short int FG_COLOR_old = FG_COLOR, BG_COLOR_old = BG_COLOR;
    RGBA fg, bg;
    fg = W->fg_valid ? W->fg_color : FG_COL_;
    bg = W->bg_valid ? W->bg_color : BG_COL_;
    FG_COLOR = RGB_24to16(fg.R, fg.G, fg.B);
    BG_COLOR = RGB_24to16(bg.R, bg.G, bg.B);

    char cmd[9];
    cmd[0] = 'r';
    int val[2];
    val[0] = val1;
    val[1] = val2;
    int i, vals;
    if (val1 == val2 && lastVal[row * DCOLS + col] == lastVal[row * DCOLS + col + DROWS * DCOLS])
	vals = 1;
    else
	vals = 2;
    for (i = 0; i < vals; i++) {
	int lastValIndex = row * DCOLS + col + DROWS * DCOLS * (i + 1);
	int y1 = row * YRES + (YRES / 2) * i;
	cmd[2] = msb(y1);
	cmd[3] = lsb(y1);
	int y2 = y1 + (YRES) / vals - 1;	/* YRES-1 */
	cmd[5] = msb(y2);
	cmd[6] = lsb(y2);
	if (val[i] > lastVal[lastValIndex]) {
	    /* cmd[1]=col*XRES; */
	    cmd[1] = col * XRES + lastVal[lastValIndex];
	    /* cmd[3]=cmd[1]+val[i]-1; */
	    cmd[4] = cmd[1] + val[i] - lastVal[lastValIndex] - 1;
	    cmd[7] = msb(FG_COLOR);
	    cmd[8] = lsb(FG_COLOR);
	    drv_D4D_send_extra(cmd, sizeof(cmd), 2, 5);
	} else if (val[i] < lastVal[lastValIndex]) {
	    /* cmd[1]=cmd[3]+1; */
	    cmd[1] = col * XRES + val[i] - 1 + 1;
	    /* cmd[3]=col*XRES+max; */
	    cmd[4] = cmd[1] + lastVal[lastValIndex] - val[i];
	    cmd[7] = msb(BG_COLOR);
	    cmd[8] = lsb(BG_COLOR);
	    drv_D4D_send_extra(cmd, sizeof(cmd), 2, 5);
	}
	lastVal[lastValIndex] = val[i];
    }

    FG_COLOR = FG_COLOR_old;
    BG_COLOR = BG_COLOR_old;
    return 0;
}





/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_D4D_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
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


int drv_D4D_list(void)
{
    printf("4D Systems serial displays");
    return 0;
}


int drv_D4D_init(const char *section, const int quiet)
{
    info("drv_D4D_init()");
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev: 840 $");

    if ((ret = drv_D4D_start(section)) != 0)
	return ret;

    if (EXTRA)
	CHARS = 64;
    else
	CHARS = 32;
    CHAR0 = 128;
    GOTO_COST = 7;

    if (MODE) {
	drv_generic_graphic_real_blit = drv_D4D_blit;
    } else {
	drv_generic_text_real_write = drv_D4D_write;
	drv_generic_text_real_defchar = drv_D4D_defchar;
    }




    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", NAME_, DCOLS, DROWS);
	if (MODE) {
	    if (drv_generic_graphic_greet(buffer, "www.4dsystems.com.au")) {
		sleep(3);
		drv_generic_graphic_clear();
	    }
	} else {
	    if (drv_generic_text_greet(buffer, "www.4dsystems.com.au")) {
		sleep(3);
		drv_D4D_clear();
	    }
	}
    }


    if (MODE) {
	if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	    return ret;
    } else {

	if ((ret = drv_generic_text_init(section, Name)) != 0)
	    return ret;

	if ((ret = drv_generic_text_icon_init()) != 0)
	    return ret;

	if ((ret = drv_generic_text_bar_init(0)) != 0)
	    return ret;

	drv_generic_text_bar_add_segment(0, 0, 255, 32);
	/* drv_generic_text_bar_add_segment(255, 255, 255, 31); */  /* ASCII 255 = block */


	wc = Widget_Text;
	wc.draw = drv_D4D_text_draw;
	widget_register(&wc);

	wc = Widget_Icon;
	wc.draw = drv_generic_text_icon_draw;
	widget_register(&wc);

	wc = Widget_Bar;
	wc.draw = drv_D4D_bar_draw;
	widget_register(&wc);

    }

    AddFunction("LCD::contrast", 1, plugin_contrast);

    return 0;
}


int drv_D4D_quit(const int quiet)
{
    error("drv_D4D_quit()");

    info("%s: shutting down.", Name);

    if (MODE == 0)
	drv_generic_text_quit();


    if (!NOPOWERCYCLE) {
	drv_D4D_clear();
    }

    if (!quiet) {
	if (MODE)
	    drv_generic_graphic_greet("goodbye!", NULL);
	else
	    drv_generic_text_greet("goodbye!", NULL);
    }

    /*int i;
       for(i=CONTRAST_;i>=0;i--) {
       drv_D4D_contrast(i);
       usleep(1000);
       } */

    if (MODE)
	drv_generic_graphic_quit();

    if (!NOPOWERCYCLE) {
	char powerDown[] = { 'Y', 3, 0 };
	drv_D4D_send(powerDown, sizeof(powerDown));
    }

    debug("closing connection");
    drv_D4D_close();

    return (0);
}



DRIVER drv_D4D = {
    .name = Name,
    .list = drv_D4D_list,
    .init = drv_D4D_init,
    .quit = drv_D4D_quit,
};
