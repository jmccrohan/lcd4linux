/* $Id$
 * $URL$
 *
 * LCD4Linux driver for 4D Systems Display Graphics Modules
 *
 * Copyright (C) 2008, 2011 Sven Killig <sven@killig.de>
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

#include <sys/ioctl.h>
#include <linux/serial.h>

static char Name[] = "D4D";
char NAME_[20];
int FONT = 1, MODE = 0, EXTRA = 0, SECTOR = 0, SECTOR_SIZE, POWERCYCLE = 0;
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


static void drv_D4D_receive_ACK()
{
    char ret[1];
    while (drv_generic_serial_read(ret, sizeof(ret)) != 1)
	usleep(1);		/* loop should be unneccessary */
    if (ret[0] == 0x15) {
	error("NAK!");
    } else if (ret[0] != 6) {
	error("no ACK!");
    }
}


static void drv_D4D_send_nowait(const char *data, const unsigned int len)
{
    if (len > 1 && data[0] >= 32 && data[0] < 127)
	debug("drv_D4D_send_nowait('%c'", data[0]);
    drv_generic_serial_write(data, len);
}

static void drv_D4D_send(const char *data, const unsigned int len)
{
    drv_D4D_send_nowait(data, len);
    drv_D4D_receive_ACK();
}

static void drv_D4D_send_extra_nowait(const char *data, const unsigned int len, unsigned char pos1, unsigned char pos2,
				      unsigned char pos3, unsigned char pos4)
{
    /* possibly leave out bytes at posn for GOLDELOX protocol format */
    if (EXTRA) {
	drv_D4D_send_nowait(data, len);
    } else {
	unsigned int i, j = 0, lenNew = len;
	char send[len];
	for (i = 0; i < len; i++) {
	    if (i != pos1 && i != pos2 && i != pos3 && i != pos4) {
		send[j++] = data[i];
	    } else {
		lenNew--;
	    }
	}
	drv_D4D_send_nowait(send, lenNew);
    }
}

static void drv_D4D_send_extra(const char *data, const unsigned int len, unsigned char pos1, unsigned char pos2,
			       unsigned char pos3, unsigned char pos4)
{
    drv_D4D_send_extra_nowait(data, len, pos1, pos2, pos3, pos4);
    drv_D4D_receive_ACK();
}


static int drv_D4D_open(const char *section)
{
    info("drv_D4D_open()");
    int fd, picaso = 0;
    struct termios portset;
    static speed_t Speed = 0;
    fd = drv_generic_serial_open(section, Name, 0);
    if (fd < 0)
	return -1;
    int flags;
    flags = fcntl(fd, F_GETFL);
    flags &= ~FNDELAY;
    fcntl(fd, F_SETFL, flags);

    cfg_number(section, "PICASO", 0, 1200, 4000000, &picaso);
    if (picaso) {
	info("switching to 9600 baud");
	if (tcgetattr(fd, &portset) == -1) {
	    error("%s: tcgetattr failed: %s", Name, strerror(errno));
	    return -1;
	}
	cfsetispeed(&portset, B9600);
	cfsetospeed(&portset, B9600);
	if (tcsetattr(fd, TCSANOW, &portset) == -1) {
	    error("%s: tcsetattr failed: %s", Name, strerror(errno));
	    return -1;
	}
    }

    char cmd[] = { 'U' };
    drv_D4D_send(cmd, sizeof(cmd));
    debug("2");

    if (picaso) {
	char baud[] = { 'Q', 0 };
	switch (picaso) {
	case 1200:
	    Speed = B1200;
	    baud[1] = 0x03;
	    break;
	case 2400:
	    Speed = B2400;
	    baud[1] = 0x04;
	    break;
	case 4800:
	    Speed = B4800;
	    baud[1] = 0x05;
	    break;
	case 9600:
	    Speed = B9600;
	    baud[1] = 0x06;
	    break;
	case 19200:
	    Speed = B19200;
	    baud[1] = 0x08;
	    break;
	case 38400:
	    Speed = B38400;
	    baud[1] = 0x0A;
	    break;
	case 57600:
	    Speed = B57600;
	    baud[1] = 0x0B;
	    break;
	case 115200:
	    Speed = B115200;
	    baud[1] = 0x0D;
	    break;
	default:
	    if (picaso >= 128000 && picaso < 256000) {	/* FTDI: 129032 */
		Speed = B38400;
		baud[1] = 0x0E;
		break;
	    } else if (picaso >= 256000) {	/* FTDI: 282353 */
		Speed = B38400;
		baud[1] = 0x0F;
		break;
	    }
	    error("%s: unsupported speed '%d' from %s", Name, picaso, cfg_source());
	    return -1;
	}
	drv_D4D_send_nowait(baud, sizeof(baud));
	sleep(1);
	debug("3");
	cfsetispeed(&portset, Speed);
	cfsetospeed(&portset, Speed);
	if (picaso >= 128000) {
	    struct serial_struct sstruct;
	    if (ioctl(fd, TIOCGSERIAL, &sstruct) < 0) {
		error("Error: could not get comm ioctl\n");
		return -1;
	    }
	    sstruct.custom_divisor = sstruct.baud_base / picaso;
	    info("baud_base=%d; custom_divisor=%d -> effective baud:%d", sstruct.baud_base, sstruct.custom_divisor,
		 sstruct.baud_base / sstruct.custom_divisor);
	    sstruct.flags |= ASYNC_SPD_CUST;
	    if (ioctl(fd, TIOCSSERIAL, &sstruct) < 0) {
		error("Error: could not set custom comm baud divisor\n");
		return -1;
	    }
	}
	debug("4");
	info("switching to %d baud", picaso);
	if (tcsetattr(fd, /*TCSANOW*/ TCSAFLUSH /*TCSADRAIN*/, &portset) == -1) {
	    error("%s: tcsetattr failed: %s", Name, strerror(errno));
	    return -1;
	}
	debug("5");

    }

    return 0;
}


static int drv_D4D_close(void)
{
    drv_generic_serial_close();
    return 0;
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

    if (!SECTOR) {		/* font in ROM */

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

	char cmd[] = { 's', col, row, FONT, msb(FG_COLOR), lsb(FG_COLOR) };	/* normal chars */
	drv_D4D_send_nowait(cmd, sizeof(cmd));
	if (len > 256)
	    len = 256;
	drv_D4D_send_nowait(out, len);
	char cmdNull[1];
	cmdNull[0] = 0;
	drv_D4D_send(cmdNull, 1);

	/*                       1, 2, 3, 4, 5, 6
	   g, c,mx,lx,my,ly */
	char cmd_user[] = { 'D', 0, 0, 0, 0, 0, 0, msb(FG_COLOR), lsb(FG_COLOR) };	/* user defined symbols */
	for (i = 0; i < k; i++) {
	    cmd_user[2] = user_char[i];
	    cmd_user[3] = msb(user_x[i]);
	    cmd_user[4] = lsb(user_x[i]);
	    cmd_user[5] = msb(user_y[i]);
	    cmd_user[6] = lsb(user_y[i]);
	    drv_D4D_send_extra(cmd_user, sizeof(cmd_user), 1, 3, 5, -1);
	}
    } else {			/* font on SD card */
	int sec;
	/*                          2, 3, 4              , 5              , 6        , 7        , 8        , 9        , 10,11,12,13
	   mx,lx, my             , ly             , mw       , lw       , mh       , lh       , cm,hs,ms,ls */
	char cmd_sd[] =
	    { '@', 'I', 0, 0, msb(row * YRES), lsb(row * YRES), msb(XRES), lsb(XRES), msb(YRES), lsb(YRES), 16, 0, 0,
	    0
	};
	for (i = 0; i < len; i++) {
	    cmd_sd[2] = msb((col + i) * XRES);
	    cmd_sd[3] = lsb((col + i) * XRES);
	    sec = SECTOR + (unsigned char) data[i] * SECTOR_SIZE;
	    cmd_sd[11] = address_hi(sec);
	    cmd_sd[12] = address_mid(sec);
	    cmd_sd[13] = address_lo(sec);
	    drv_D4D_send_extra(cmd_sd, sizeof(cmd_sd), 2, 4, 6, 8);
	}
    }
}


static void drv_D4D_defchar(const int ascii, const unsigned char *matrix)
{
    info("drv_D4D_defchar");
    char cmd[11];
    int i;

    cmd[0] = 'A';
    cmd[1] = 0;
    cmd[2] = ascii - CHAR0;

    for (i = 0; i < 8; i++) {
	cmd[i + 3] = *matrix++;
    }
    drv_D4D_send_extra(cmd, sizeof(cmd), 1, -1, -1, -1);
}


static void drv_D4D_blit(const int row, const int col, const int height, const int width)
{
    debug("drv_D4D_blit(%i, %i, %i, %i)", row, col, height, width);
    int r, c;
    RGBA rgb;
    short int color;
    char colorArray[width * height * MODE / 8];

    /* optimization: single colour rectangle? */
    /* commented out because obviously seldom used and expensive */
    /*RGBA pixel0_0, pixel;
       pixel0_0 = drv_generic_graphic_rgb(row, col);
       char unicolor = 1;
       for (r = row; (r < row + height) && unicolor; r++) {
       for (c = col; (c < col + width) && unicolor; c++) {
       pixel = drv_generic_graphic_rgb(r, c);
       if (pixel0_0.R != pixel.R || pixel0_0.G != pixel.G || pixel0_0.B != pixel.B || pixel0_0.A != pixel.A)
       unicolor = 0;
       }
       }

       if (unicolor) {
       color = RGB_24to16(pixel0_0.R, pixel0_0.G, pixel0_0.B);
       unsigned char col2 = col + width - 1;
       unsigned char row2 = row + height - 1;
       char cmdRect[] =
       { 'r', msb(col), lsb(col), msb(row), lsb(row), msb(col2), lsb(col2), msb(row2), lsb(row2), msb(color),
       lsb(color)
       };
       debug("Rectangle(%i, %i, %i, %i, %i", col, row, col2, row2, color);
       drv_D4D_send_extra(cmdRect, sizeof(cmdRect), 1, 3, 5, 7);
       } else { */
    char cmd[] =
	{ 'I', msb(col), lsb(col), msb(row), lsb(row), msb(width), lsb(width), msb(height), lsb(height), MODE };
    int p = 0;
    drv_D4D_send_extra_nowait(cmd, sizeof(cmd), 1, 3, 5, 7);
    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    rgb = drv_generic_graphic_rgb(r, c);
	    if (MODE == 8) {
		colorArray[p++] = RGB_24to8(rgb.R, rgb.G, rgb.B);
	    } else {
		color = RGB_24to16(rgb.R, rgb.G, rgb.B);
		colorArray[p++] = msb(color);
		colorArray[p++] = lsb(color);
	    }
	}
    }
    drv_D4D_send_nowait(colorArray, width * height * MODE / 8);	/* doesn't werk if sent together (error: "partial write(/dev/tts/1): len=2 ret=1") */
    drv_D4D_receive_ACK();
    /*} */
}


static int drv_D4D_contrast(int contrast)
{
    if (contrast < 0)
	contrast = 0;
    if (contrast > 15)
	contrast = 15;

    char cmd[] = { 'Y', 2, contrast };
    drv_D4D_send(cmd, sizeof(cmd));

    /* CONTRAST_=contrast; */
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

    char getVersion[] = { 'V', 0 /*1 */  };
    drv_D4D_send_nowait(getVersion, sizeof(getVersion));
    char answer[5];
    debug("reading answer[0]");
    drv_generic_serial_read(answer + 0, 1);	/* ,5: PICASO 0/1, Speed 9600: error: "partial read(/dev/ttyUSB0): len=5 ret=1" */
    debug("reading answer[1]");
    drv_generic_serial_read(answer + 1, 1);
    debug("reading answer[2]");
    drv_generic_serial_read(answer + 2, 1);
    debug("reading answer[3]");
    drv_generic_serial_read(answer + 3, 1);
    debug("reading answer[4]");
    drv_generic_serial_read(answer + 4, 1);
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
	switch ((unsigned char) answer[3 + i]) {
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
	case 0x96:
	    res[i] = 96;
	    break;
	case 0x24:		/* undocumented? */
	    res[i] = 240;
	    break;
	default:
	    error("Can't detect display dimensions! answer: {%d, %d, %d, %d, %d}", answer[0], answer[1], answer[2],
		  answer[3], answer[4]);
	    return -1;
	}
    }
    DCOLS = res[0];
    DROWS = res[1];


    cfg_number(section, "Mode", 0, 0, 16, &MODE);

    if (MODE == 0) {
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

	if (yres_cfg == -1) {	/* font on SD card */
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
	switch (yres_cfg) {	/* font in ROM */
	case 8:
	    switch (xres_cfg) {
	    case 6:
		FONT = 0;
		break;
	    case 8:
		FONT = 1;
		break;
	    default:
		error("%s: unknown width in %s.Font '%s' from %s", Name, section, s, cfg_source());
		return -1;
	    };
	    break;
	case 12:
	    FONT = 2;
	    break;
	case 16:
	    FONT = 3;
	    break;
	case -1:
	    break;
	default:
	    error("%s: unknown height in %s.Font '%s' from %s", Name, section, s, cfg_source());
	    return -1;
	}
    }

    info("XRES=%i, YRES=%i;  DCOLS=%i, DROWS=%d;  FONT=%d, SECTOR=%i, SECTOR_SIZE=%i\n", XRES, YRES, DCOLS, DROWS,
	 FONT, SECTOR, SECTOR_SIZE);



    cfg_number(section, "PowerCycle", 0, 0, 1, &POWERCYCLE);
    if (POWERCYCLE) {
	char powerOn[] = { 'Y', 3, 1 };
	drv_D4D_send(powerOn, sizeof(powerOn));

	/*char background[] = {'B', msb(BG_COLOR), lsb(BG_COLOR)};
	   drv_D4D_send(background, sizeof(background)); */
    }

    char opaque[] = { 'O', 1 };
    drv_D4D_send(opaque, sizeof(opaque));

    char penSize[] = { 'p', 0 };
    drv_D4D_send(penSize, sizeof(penSize));


    if (!MODE) {
	unsigned char buffer[] = { 0x18, 0x24, 0x24, 0x18, 0, 0, 0, 0 };	/* degree */
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


int lastVal[40 * 40 * 2];	/* Fixme: MAX_WIDGETS*2 */
int drv_D4D_bar_draw(WIDGET * W)
{
    /* optimizations:
       - draw filled rectangles instead of user defined bar characters
       - cache last values */

    WIDGET_BAR *Bar = W->data;
    int row, col, len, res, max, val1, val2;
    DIRECTION dir;
#if 0
    STYLE style;		/* Fixme: missing style support */
#endif

    row = W->row;
    col = W->col;
    dir = Bar->direction;
#if 0
    style = Bar->style;		/* Fixme: missing style support */
#endif
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

    char cmd[11];
    cmd[0] = 'r';
    int val[2];
    val[0] = val1;
    val[1] = val2;
    int i, vals, x1, x2;
    int lastValIndex0 = row * DCOLS + col;
    int cells = DROWS * DCOLS;
    if (val1 == val2 && lastVal[lastValIndex0] == lastVal[lastValIndex0 + cells])
	vals = 1;
    else
	vals = 2;
    for (i = 0; i < vals; i++) {
	int lastValIndex = lastValIndex0 + cells * (i /*+1 */ );
	int y1 = row * YRES + (YRES / 2) * i;
	cmd[3] = msb(y1);
	cmd[4] = lsb(y1);
	int y2 = y1 + YRES / vals - 1;
	cmd[7] = msb(y2);
	cmd[8] = lsb(y2);
	if (val[i] > lastVal[lastValIndex]) {
	    x1 = col * XRES + lastVal[lastValIndex];
	    cmd[1] = msb(x1);
	    cmd[2] = lsb(x1);
	    x2 = x1 + val[i] - lastVal[lastValIndex] - 1;
	    cmd[5] = msb(x2);
	    cmd[6] = lsb(x2);
	    cmd[9] = msb(FG_COLOR);
	    cmd[10] = lsb(FG_COLOR);
	    drv_D4D_send_extra(cmd, sizeof(cmd), 1, 3, 5, 7);
	} else if (val[i] < lastVal[lastValIndex]) {
	    x1 = col * XRES + val[i] - 1 + 1;
	    cmd[1] = msb(x1);
	    cmd[2] = lsb(x1);
	    x2 = x1 + lastVal[lastValIndex] - val[i];
	    cmd[5] = msb(x2);
	    cmd[6] = lsb(x2);
	    cmd[9] = msb(BG_COLOR);
	    cmd[10] = lsb(BG_COLOR);
	    drv_D4D_send_extra(cmd, sizeof(cmd), 1, 3, 5, 7);
	} else {
	    /* do nothing */
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
    printf("4D Systems Display Graphics Modules");
    return 0;
}


int drv_D4D_init(const char *section, const int quiet)
{
    info("drv_D4D_init()");
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Rev$");

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
    info("drv_D4D_quit()");

    info("%s: shutting down.", Name);

    if (MODE == 0)
	drv_generic_text_quit();


    if (POWERCYCLE) {
	drv_D4D_clear();
    }

    if (!quiet) {
	if (MODE)
	    drv_generic_graphic_greet("goodbye!", NULL);
	else
	    drv_generic_text_greet("goodbye!", NULL);
    }

    /* fade */
    /*int i;
       for(i=CONTRAST_;i>=0;i--) {
       drv_D4D_contrast(i);
       usleep(1000);
       } */

    if (MODE)
	drv_generic_graphic_quit();

    if (POWERCYCLE) {
	char powerDown[] = { 'Y', 3, 0 };
	drv_D4D_send(powerDown, sizeof(powerDown));
    }

    if (EXTRA) {
	info("switching to 9600 baud");
	char baud[] = { 'Q', 0x06 };
	drv_D4D_send_nowait(baud, sizeof(baud));
    }

    info("closing connection");
    drv_D4D_close();

    return (0);
}



DRIVER drv_D4D = {
    .name = Name,
    .list = drv_D4D_list,
    .init = drv_D4D_init,
    .quit = drv_D4D_quit,
};
