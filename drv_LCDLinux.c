/* $Id: drv_LCDLinux.c,v 1.13 2006/04/17 08:10:42 reinelt Exp $
 *
 * driver for the LCD-Linux HD44780 kernel driver
 * http://lcd-linux.sourceforge.net
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
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
 *
 * $Log: drv_LCDLinux.c,v $
 * Revision 1.13  2006/04/17 08:10:42  reinelt
 * LCDLinux patch from Mattia; widget_image moved to EXTRA_SOURCE
 *
 * Revision 1.12  2006/01/30 06:25:52  reinelt
 * added CVS Revision
 *
 * Revision 1.11  2006/01/18 11:49:48  reinelt
 * adopted to lcd-linux-0.9.2
 *
 * Revision 1.10  2005/08/27 07:02:25  reinelt
 * LCD-Linux updated to 0.9.0
 *
 * Revision 1.9  2005/06/15 05:24:35  reinelt
 * updated LCD-Linux driver to version 0.8.9
 *
 * Revision 1.8  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.7  2005/05/02 05:15:46  reinelt
 * make busy-flag checking configurable for LCD-Linux driver
 *
 * Revision 1.6  2005/04/30 06:02:09  reinelt
 * LCD-Linux display size set up from lcd4linux.conf
 *
 * Revision 1.5  2005/04/09 07:36:42  reinelt
 * updated LCD-Linux driver to version 0.8.8
 *
 * Revision 1.4  2005/02/24 07:06:48  reinelt
 * SimpleLCD driver added
 *
 * Revision 1.3  2005/02/24 06:51:40  reinelt
 * LCD-Linux driver GOTO_COST corrected
 *
 * Revision 1.2  2005/01/30 06:43:22  reinelt
 * driver for LCD-Linux finished
 *
 * Revision 1.1  2005/01/22 22:57:57  reinelt
 * LCD-Linux driver added
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_LCDLinux
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

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

#define LCD_LINUX_MAIN
#include <linux/lcd-linux.h>
#include <linux/hd44780.h>

static char Name[] = "LCD-Linux";
static char Device[] = "/dev/lcd";
static int lcdlinux_fd = -1;
static int raw_mode = 0;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static void drv_LL_send(const char *string, const int len)
{
    int run, ret;

    for (run = 0; run < 10; run++) {
	ret = write(lcdlinux_fd, string, len);
	if (ret >= 0 || errno != EAGAIN)
	    break;
	if (run > 0)
	    info("%s: write(%s): EAGAIN #%d", Name, Device, run);
	usleep(1000);
    }

    if (ret < 0) {
	error("%s: write(%s) failed: %s", Name, Device, strerror(errno));
    } else if (ret != len) {
	error("%s: partial write(%s): len=%d ret=%d", Name, Device, len, ret);
    }

    return;
}


static void drv_LL_clear(void)
{
    /* No return value check since this ioctl cannot fail */
    ioctl(lcdlinux_fd, IOCTL_CLEAR_DISP);
}


static void drv_LL_write(const int row, const int col, const char *data, int len)
{
    int pos = row * DCOLS + col;

    if (lseek(lcdlinux_fd, pos, SEEK_SET) == (off_t) - 1) {
	error("%s: lseek(%s) failed: %s", Name, Device, strerror(errno));
    }
    drv_LL_send(data, len);
}


static void drv_LL_defchar(const int ascii, const unsigned char *matrix)
{
    char buffer[8];
    int pos = 1024 + ascii;
    int i;

    for (i = 0; i < 8; i++) {
	buffer[i] = *matrix++ & 0x1f;
    }

    if (lseek(lcdlinux_fd, pos, SEEK_SET) == (off_t) - 1) {
	error("%s: lseek(%s) failed: %s", Name, Device, strerror(errno));
    }

    drv_LL_send(buffer, 8);

}


static int drv_LL_start(const char *section, const int quiet)
{
    char *s;
    int rows = -1, cols = -1;
    int use_busy = 0, bus4bits = 0, commit = 0;
    struct lcd_parameters buf;

    /* emit version information */
    info("%s: Version %s", Name, LCD_LINUX_VERSION);

    /* get size from config file */
    s = cfg_get(section, "Size", NULL);
    if (s != NULL || *s != '\0') {
	if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	    error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	    free(s);
	    return -1;
	}
    }
    free(s);

    /* open device */
    lcdlinux_fd = open(Device, O_WRONLY);
    if (lcdlinux_fd == -1) {
	error("%s: open(%s) failed: %s", Name, Device, strerror(errno));
	return -1;
    }

    /* get display size */
    memset(&buf, 0, sizeof(buf));
    if (ioctl(lcdlinux_fd, IOCTL_GET_PARAM, &buf) != 0) {
	error("%s: ioctl(IOCTL_GET_PARAM) failed: %s", Name, strerror(errno));
	error("%s: Could not query display information!", Name);
	return -1;
    }
    info("%s: %dx%d display with %d controllers, flags=0x%02x:",
    	Name, buf.cntr_cols, buf.cntr_rows, buf.num_cntr, buf.flags);
    info("%s:   busy-flag checking %sabled", Name, buf.flags & HD44780_CHECK_BF ? "en" : "dis");
    info("%s:   bus width %d bits", Name, buf.flags & HD44780_4BITS_BUS ? 4 : 8);
    info("%s:   font size %s", Name, buf.flags & HD44780_5X10_FONT ? "5x10" : "5x8");



    /* overwrite width size from lcd4linux.conf */
    if ((rows > 0 && rows != buf.cntr_rows) || (cols > 0 && cols != buf.cntr_cols)) {
	info("%s: changing size to %dx%d", Name, cols, rows);
	buf.cntr_rows = rows;
	buf.cntr_cols = cols;
	commit = 1;
    }

    DROWS = buf.cntr_rows;
    DCOLS = buf.cntr_cols;

    /* overwrite busy-flag checking from lcd4linux.conf */
    cfg_number(section, "UseBusy", 0, 0, 1, &use_busy);
    if (use_busy && !(buf.flags & HD44780_CHECK_BF)) {
	info("%s: activating busy-flag checking", Name);
	buf.flags |= HD44780_CHECK_BF;
	commit = 1;
    } else if (!use_busy && (buf.flags & HD44780_CHECK_BF)) {
	info("%s: deactivating busy-flag checking", Name);
	buf.flags &= ~HD44780_CHECK_BF;
	commit = 1;
    }

    /* overwrite bus length from lcd4linux.conf */
    cfg_number(section, "Bus4Bit", 0, 0, 1, &bus4bits);
    if (bus4bits && !(buf.flags & HD44780_4BITS_BUS)) {
	info("%s: setting bus length to 4 bits", Name);
	buf.flags |= HD44780_4BITS_BUS;
	commit = 1;
    } else if (!bus4bits && (buf.flags & HD44780_4BITS_BUS)) {
	info("%s: setting bus length to 8 bits", Name);
	buf.flags &= ~HD44780_4BITS_BUS;
	commit = 1;
    }

    if (commit && ioctl(lcdlinux_fd, IOCTL_SET_PARAM, &buf) != 0) {
	error("%s: ioctl(IOCTL_SET_PARAM) failed: %s", Name, strerror(errno));
	return -1;
    }

    /* initialize display */
    drv_LL_clear();		/* clear display */

    /* Disable control characters interpretation. */
    /* No return value check since this ioctl cannot fail */
    raw_mode = 1;
    ioctl(lcdlinux_fd, IOCTL_RAW_MODE, &raw_mode);

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "lcd-linux.sf.net")) {
	    sleep(3);
	    drv_LL_clear();
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
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_LL_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_LL_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int asc255bug;
    int ret;

    info("%s: %s", Name, "$Revision: 1.13 $");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = -1;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_LL_write;
    drv_generic_text_real_defchar = drv_LL_defchar;


    /* start display */
    if ((ret = drv_LL_start(section, quiet)) != 0)
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
    /* most displays have a full block on ascii 255, but some have kind of  */
    /* an 'inverted P'. If you specify 'asc255bug 1 in the config, this */
    /* char will not be used, but rendered by the bar driver */
    cfg_number(section, "asc255bug", 0, 0, 1, &asc255bug);
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    if (!asc255bug)
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
    /* none */

    return 0;
}


/* close driver & display */
int drv_LL_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_LL_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    /* Enable control characters interpretation. */
    /* No return value check since this ioctl cannot fail */
    raw_mode = 0;
    ioctl(lcdlinux_fd, IOCTL_RAW_MODE, &raw_mode);

    /* close device */
    close(lcdlinux_fd);

    return (0);
}


DRIVER drv_LCDLinux = {
  name:Name,
  list:drv_LL_list,
  init:drv_LL_init,
  quit:drv_LL_quit,
};
