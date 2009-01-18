/* $Id$
 * $URL$
 *
 * ULA200 driver for lcd4linux
 *
 * Copyright (C) 2008 Bernhard Walle <bernhard.walle@gmx.de>
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
 * Driver for the ELV ULA200 USB device. The device can control one
 * HD44780 display up to 4x20 characters.
 *
 * Implemented functions:
 *   - displaying characters :-)
 *   - controlling the backlight
 *
 * Todo:
 *   - input buttons
 *
 * Configuration:
 *   - Size (XxY): size of the display (e.g. '20x4')
 *   - Backlight (0/1): initial state of the backlight
 *
 * Author:
 *   Bernhard Walle <bernhard.walle@gmx.de>
 *
 * exported fuctions:
 *   struct DRIVER drv_ula200
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <ftdi.h>

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

/* text mode display? */
#include "drv_generic_text.h"

/****************************************/
/***        Global variables          ***/
/****************************************/

static char Name[] = "ULA200";
static struct ftdi_context *Ftdi = NULL;


/****************************************/
/***        Constants                 ***/
/****************************************/

/* USB connection */
#define ULA200_VENDOR_ID    	0x0403
#define ULA200_PRODUCT_ID   	0xf06d

/* connection parameters */
#define	ULA200_BAUDRATE		19200
#define ULA200_DATABITS		BITS_8
#define	ULA200_STOPBITS		STOP_BIT_1
#define ULA200_PARITY		EVEN

/* character constants used for the communication */
#define ULA200_CH_STX  		0x02
#define ULA200_CH_ETX  		0x03
#define ULA200_CH_ENQ  		0x05
#define ULA200_CH_ACK  		0x06
#define ULA200_CH_NAK  		0x15
#define ULA200_CH_DC2  		0x12
#define ULA200_CH_DC3  		0x13

/* commands used for the communication (names are German) */
#define ULA200_CMD_POSITION	'p'	/* 'position' */
#define ULA200_CMD_STRING	's'	/* 'string' */
#define ULA200_CMD_CLEAR	'l'	/* 'loeschen' */
#define ULA200_CMD_BACKLIGHT	'h'	/* 'hintergrund' */
#define ULA200_CMD_CHAR     	'c'	/* 'character' */

/* raw register access */
#define ULA200_RS_DATA     	0x00	/* data */
#define ULA200_RS_INSTR    	0x01	/* instruction */
#define ULA200_SETCHAR		0x40	/* set user-defined character */

/* character sizes */
#define ULA200_CELLWIDTH 	5
#define ULA200_CELLHEIGHT	8

/* internal implementation constants */
#define ULA200_BUFFER_LENGTH	1024
#define ULA200_MAXLEN		512
#define ULA200_MAX_REPEATS	20

/* define TRUE and FALSE for better code readability if not already defined */
#ifndef TRUE
#   define TRUE 1
#endif
#ifndef FALSE
#   define FALSE 0
#endif


/****************************************/
/***        Macros                    ***/
/****************************************/

#define ULA200_ERROR(msg, ...) \
	error("%s: In %s():%d: " msg, Name, \
	    __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ULA200_INFO(msg, ...) \
	info("%s: " msg, Name, ##__VA_ARGS__)

#define ULA200_DEBUG(msg, ...) \
	debug("%s: In %s():%d: " msg, Name, \
	    __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ULA200_TRACE() \
	debug("%s: Calling %s()", Name, __FUNCTION__)


/****************************************/
/***        Prototypes                ***/
/****************************************/

static int drv_ula200_ftdi_read_response(void);
static int drv_ula200_ftdi_usb_read(void);
static int drv_ula200_ftdi_write_command(const unsigned char *, int);
static int drv_ula200_backlight(int);
static int drv_ula200_close(void);

static void plugin_backlight(RESULT *, RESULT *);

/****************************************/
/***        Internal (helper) funcs   ***/
/****************************************/

/**
 * Write a command to the display. Adds the STX and ETX header/trailer.
 *
 * @param[in] data the data bytes
 * @param[in] length the number of bytes in data which are valid
 * @return 0 on success, negative value on error
 */
static int drv_ula200_ftdi_write_command(const unsigned char *data, int length)
{
    int i, err;
    int repeat_count = 0;
    int pos = 0;
    unsigned char buffer[ULA200_BUFFER_LENGTH];

    /* check for the maximum length */
    if (length > ULA200_MAXLEN) {
	return -EINVAL;
    }

    /* fill the array */
    buffer[pos++] = ULA200_CH_STX;
    for (i = 0; i < length; i++) {
	if (data[i] == ULA200_CH_STX) {
	    buffer[pos++] = ULA200_CH_ENQ;
	    buffer[pos++] = ULA200_CH_DC2;
	} else if (data[i] == ULA200_CH_ETX) {
	    buffer[pos++] = ULA200_CH_ENQ;
	    buffer[pos++] = ULA200_CH_DC3;
	} else if (data[i] == ULA200_CH_ENQ) {
	    buffer[pos++] = ULA200_CH_ENQ;
	    buffer[pos++] = ULA200_CH_NAK;
	} else {
	    buffer[pos++] = data[i];
	}
    }
    buffer[pos++] = ULA200_CH_ETX;

    do {
	/* ULA200_DEBUG("ftdi_write_data(%p, %d)", buffer, pos); */
	err = ftdi_write_data(Ftdi, buffer, pos);
	if (err < 0) {
	    ULA200_ERROR("ftdi_write_data() failed");
	    return -1;
	}
    }
    while (!drv_ula200_ftdi_read_response() && (repeat_count++ < ULA200_MAX_REPEATS));

    return 0;
}

/**
 * Reads a character from USB.
 *
 * @return a positive value between 0 and 255 indicates the character that
 *         has been read successfully, -1 indicates an error
 */
static int drv_ula200_ftdi_usb_read(void)
{
    unsigned char buffer[1];
    int err;

    while ((err = ftdi_read_data(Ftdi, buffer, 1)) == 0);
    return err >= 0 ? buffer[0] : -1;
}


/**
 * Reads the response of the display. Currently, key input is ignored
 * and only ACK / NACK is read.
 *
 * @return TRUE on success (ACK), FALSE on failure (NACK)
 */
static int drv_ula200_ftdi_read_response(void)
{
    int result = FALSE;
    int answer_read = FALSE;
    int ret;
    int ch;

    while (!answer_read) {
	/* wait until STX */
	do {
	    ret = drv_ula200_ftdi_usb_read();
	    /* ULA200_DEBUG("STX drv_ula200_ftdi_usb_read = %d", ret); */
	} while ((ret != ULA200_CH_STX) && (ret > 0));

	if (ret < 0) {
	    return FALSE;
	}

	/* read next char */
	ch = drv_ula200_ftdi_usb_read();
	/* ULA200_DEBUG("drv_ula200_ftdi_usb_read = %d", ch); */

	switch (ch) {
	case 't':
	    ch = drv_ula200_ftdi_usb_read();
	    /* ULA200_DEBUG("drv_ula200_ftdi_usb_read = %d", ch); */
	    /* ignore currently */
	    break;

	case ULA200_CH_ACK:
	    answer_read = TRUE;
	    result = TRUE;
	    break;

	case ULA200_CH_NAK:
	    answer_read = TRUE;
	    result = FALSE;
	    break;

	default:
	    answer_read = TRUE;
	    ULA200_ERROR("Read invalid answer");
	}

	/* wait until ETX */
	do {
	    ret = drv_ula200_ftdi_usb_read();
	    /* ULA200_DEBUG("ETX drv_ula200_ftdi_usb_read = %d", ret); */
	} while ((ret != ULA200_CH_ETX) && (ret > 0));

	if (ret < 0) {
	    return FALSE;
	}
    }

    return result;
}

static int drv_ula200_ftdi_enable_raw_mode(void)
{
    unsigned char command[3];

    command[0] = 'R';
    command[1] = 'E';
    command[2] = '1';
    return drv_ula200_ftdi_write_command(command, 3);
}


/**
 * Writes raw data (access the HD44780 registers directly.
 *
 * @param[in] flags ULA200_RS_DATA or ULA200_RS_INSTR
 * @param[in] ch the real data
 * @return 0 on success, a negative value on error
 */
static int drv_ula200_ftdi_rawdata(unsigned char flags, unsigned char ch)
{
    unsigned char command[3];
    int err;

    command[0] = 'R';
    command[1] = flags == ULA200_RS_DATA ? '2' : '0';
    command[2] = ch;
    err = drv_ula200_ftdi_write_command(command, 3);
    if (err < 0) {
	ULA200_ERROR("ula200_ftdi_write_command() failed");
	return -1;
    }

    return 0;
}

/**
 * Sets the cursor position.
 *
 * @param[in] x the x coordinate of the position
 * @param[in] y the y coordinate of the position
 * @return 0 on success, a negative value on error
 */
static int drv_ula200_set_position(int x, int y)
{
    unsigned char command[3];
    int err;

    if (y >= 2) {
	y -= 2;
	x += DCOLS;		/* XXX: multiply by 2? */
    }

    command[0] = ULA200_CMD_POSITION;
    command[1] = x;
    command[2] = y;
    err = drv_ula200_ftdi_write_command(command, 3);
    if (err < 0) {
	ULA200_ERROR("ula200_ftdi_write_command() failed");
    }

    return err;
}

/**
 * Sends the text
 *
 * @param[in] data the data bytes
 * @param[in] len the number of valid bytes in @p data
 * @return 0 on success, a negative value on error
 */
static int drv_ula200_send_text(const unsigned char *data, int len)
{
    unsigned char buffer[ULA200_BUFFER_LENGTH];
    int err;

    if (len > ULA200_MAXLEN) {
	return -EINVAL;
    }

    buffer[0] = ULA200_CMD_STRING;
    buffer[1] = len;
    memcpy(buffer + 2, data, len);
    buffer[2 + len] = 0;	/* only necessary for the debug message */

    /* ULA200_DEBUG("Text: =%s= (%d)", buffer+2, len); */

    err = drv_ula200_ftdi_write_command(buffer, len + 2);
    if (err < 0) {
	ULA200_ERROR("ula200_ftdi_write_command() failed");
	return -1;
    }

    return 0;
}

/**
 * Sends one character.
 *
 * @param[in] ch the character to send
 * @return 0 on success, a negative value on error
 */
static int drv_ula200_send_char(char ch)
{
    unsigned char buffer[2];
    int err;

    buffer[0] = ULA200_CMD_CHAR;
    buffer[1] = ch;

    err = drv_ula200_ftdi_write_command(buffer, 2);
    if (err < 0) {
	ULA200_ERROR("ula200_ftdi_write_command() failed");
	return -1;
    }

    return 0;
}

/**
 * Opens the ULA200 display. Uses libftdi to initialise the USB communication to
 * the display.
 *
 @ @return a value less then zero on failure, 0 on success
 */
static int drv_ula200_open(void)
{
    int err;

    /* check if the device was already open */
    if (Ftdi != NULL) {
	ULA200_ERROR("open called although device was already open");
	drv_ula200_close();
    }

    /* get memory for the device descriptor */
    Ftdi = malloc(sizeof(struct ftdi_context));
    if (Ftdi == NULL) {
	ULA200_ERROR("Memory allocation failed");
	return -1;
    }

    /* open the ftdi library */
    ftdi_init(Ftdi);
    Ftdi->usb_write_timeout = 20;
    Ftdi->usb_read_timeout = 20;

    /* open the device */
    err = ftdi_usb_open(Ftdi, ULA200_VENDOR_ID, ULA200_PRODUCT_ID);
    if (err < 0) {
	ULA200_ERROR("ftdi_usb_open() failed");
	free(Ftdi);
	Ftdi = NULL;
	return -1;
    }

    /* set the baudrate */
    err = ftdi_set_baudrate(Ftdi, ULA200_BAUDRATE);
    if (err < 0) {
	ULA200_ERROR("ftdi_set_baudrate() failed");
	ftdi_usb_close(Ftdi);
	free(Ftdi);
	Ftdi = NULL;
	return -1;
    }
    /* set communication parameters */
    err = ftdi_set_line_property(Ftdi, ULA200_DATABITS, ULA200_STOPBITS, ULA200_PARITY);
    if (err < 0) {
	ULA200_ERROR("ftdi_set_line_property() failed");
	ftdi_usb_close(Ftdi);
	free(Ftdi);
	Ftdi = NULL;
	return -1;
    }

    return 0;
}

/**
 * Closes the display.
 *
 * @return 0 on success, a negative value on failure
 */
static int drv_ula200_close(void)
{
    ULA200_TRACE();

    ftdi_usb_purge_buffers(Ftdi);
    ftdi_usb_close(Ftdi);
    ftdi_deinit(Ftdi);

    free(Ftdi);
    Ftdi = NULL;

    return 0;
}

/**
 * Clears the contents of the display.
 *
 * @return 0 on success, a negative value on error
 */
static void drv_ula200_clear(void)
{
    unsigned const char command[] = { ULA200_CMD_CLEAR };
    int err;

    ULA200_TRACE();

    err = drv_ula200_ftdi_write_command(command, 1);
    if (err < 0) {
	ULA200_ERROR("ula200_ftdi_write_command() failed");
    }
}

/**
 * Writes data to the display.
 *
 * @param[in] row the row where the data should be written to
 * @param[in] col the column where the data should be written to
 * @param[in] data the data that should actually be written
 * @param[in] len the number of valid bytes in @p data
 */
static void drv_ula200_write(const int row, const int col, const char *data, int len)
{
    int ret;

    /* do the cursor positioning here */
    ret = drv_ula200_set_position(col, row);
    if (ret < 0) {
	ULA200_ERROR("drv_ula200_set_position() failed");
	return;
    }

    /* send string to the display */
    if (len == 1) {
	ret = drv_ula200_send_char(data[0]);
    } else {
	ret = drv_ula200_send_text((unsigned char *) data, len);
    }
    if (ret < 0) {
	ULA200_ERROR("drv_ula200_send_text() failed");
	return;
    }
}

/* text mode displays only */
static void drv_ula200_defchar(const int ascii, const unsigned char *matrix)
{
    int err, i;

    if (ascii >= 8) {
	ULA200_ERROR("Invalid value in drv_ula200_defchar");
	return;
    }

    /* Tell the HD44780 we will redefine char number 'ascii' */
    err = drv_ula200_ftdi_rawdata(ULA200_RS_INSTR, ULA200_SETCHAR | (ascii * 8));
    if (err < 0) {
	ULA200_ERROR("drv_ula200_ftdi_rawdata() failed");
	return;
    }

    /* Send the subsequent rows */
    for (i = 0; i < YRES; i++) {
	err = drv_ula200_ftdi_rawdata(ULA200_RS_DATA, *matrix++ & 0x1f);
	if (err < 0) {
	    ULA200_ERROR("ula200_ftdi_rawdata() failed");
	    return;
	}
    }
}

/**
 * Controls the backlight of the ULA200 display.
 *
 * @param[in] backlight a negative value if the backlight should be turned off,
 *            a positive value if it should be turned on
 * @return 0 on success, any other value on failure
 */
static int drv_ula200_backlight(int backlight)
{
    unsigned char cmd[2] = { ULA200_CMD_BACKLIGHT };
    int ret;

    if (backlight <= 0) {
	backlight = '0';
    } else {
	backlight = '1';
    }

    cmd[1] = backlight;
    ret = drv_ula200_ftdi_write_command(cmd, 2);
    if (ret < 0) {
	ULA200_ERROR("ula200_ftdi_write_command() failed");
    }

    return backlight == '1';
}

/**
 * Starts the display.
 *
 * @param[in] section the section of the configuration file
 * @return 0 on success, a negative value on failure
 */
static int drv_ula200_start(const char *section)
{
    int rows = -1, cols = -1;
    char *s;
    int backlight = 0;
    int err;

    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	ULA200_ERROR("No '%s.Size' entry from %s", section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	ULA200_ERROR("Bad %s.Size '%s' from %s", section, s, cfg_source());
	free(s);
	return -1;
    }

    DROWS = rows;
    DCOLS = cols;

    /* open communication with the display */
    err = drv_ula200_open();
    if (err < 0) {
	return -1;
    }

    cfg_number(section, "Backlight", 0, 0, 1, &backlight);
    err = drv_ula200_backlight(backlight);
    if (err < 0) {
	ULA200_ERROR("drv_ula200_backlight() failed");
	return -1;
    }

    /* clear display */
    drv_ula200_clear();

    /* enable raw mode for defining own chars */
    drv_ula200_ftdi_enable_raw_mode();

    return 0;
}

/****************************************/
/***            plugins               ***/
/****************************************/

/**
 * Backlight plugin
 */
static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_ula200_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}

/****************************************/
/***        exported functions        ***/
/****************************************/

/**
 * list models
 *
 * @return 0 on success, a negative value on failure
 */
int drv_ula200_list(void)
{
    printf("ULA200");
    return 0;
}

/**
 * initialize driver & display
 *
 * @param[in] section the name of the section in the configuration file
 * @param[in] quiet TRUE on quiet mode
 * @return 0 on success, any negative error value on failure
 */
/* use this function for a text display */
int drv_ula200_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    ULA200_INFO("%s", "$Rev$");

    /* display preferences */
    XRES = ULA200_CELLWIDTH;	/* pixel width of one char  */
    YRES = ULA200_CELLHEIGHT;	/* pixel height of one char  */
    CHARS = 7;			/* number of user-defineable characters */
    CHAR0 = 1;			/* ASCII of first user-defineable char */
    GOTO_COST = 4;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_ula200_write;
    drv_generic_text_real_defchar = drv_ula200_defchar;

    /* start display */
    if ((ret = drv_ula200_start(section)) != 0) {
	return ret;
    }

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "ULA 200")) {
	    sleep(3);
	    drv_ula200_clear();
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
    AddFunction("LCD::backlight", -1, plugin_backlight);

    return 0;
}

/**
 * close driver & display
 *
 * @param[in] quiet TRUE on quiet mode
 * @return 0 on success, any negative error value on failure
 */
/* use this function for a text display */
int drv_ula200_quit(int quiet)
{
    ULA200_INFO("shutting down.");

    drv_generic_text_quit();

    /* turn backlight off */
    drv_ula200_backlight(0);

    /* clear display */
    drv_ula200_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_ula200_close();

    return 0;
}

/* use this one for a text display */
DRIVER drv_ula200 = {
    .name = Name,
    .list = drv_ula200_list,
    .init = drv_ula200_init,
    .quit = drv_ula200_quit,
};

/* :indentSize=4:tabSize=8:noTabs=false: */
