/* $Id$
 * $URL$
 *
 * new style driver for Crystalfontz display modules
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
 * struct DRIVER drv_Crystalfontz
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "thread.h"
#include "timer.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "widget_keypad.h"
#include "drv.h"
#include "drv_generic_text.h"
#include "drv_generic_gpio.h"
#include "drv_generic_serial.h"
#include "drv_generic_keypad.h"


static char Name[] = "Crystalfontz";

static int Model;
static int Protocol;
static int Payload;

/* ring buffer for bytes received from the display */
static unsigned char RingBuffer[256];
static unsigned int RingRPos = 0;
static unsigned int RingWPos = 0;

/* packet from the display */
struct {
    unsigned char type;
    unsigned char code;
    unsigned char size;
    unsigned char data[16 + 1];	/* trailing '\0' */
} Packet;

/* Line Buffer for 633 displays */
static unsigned char Line[2 * 16];

/* Fan RPM */
static double Fan_RPM[4] = { 0.0, };

/* Temperature sensors */
static double Temperature[32] = { 0.0, };


typedef struct {
    int type;
    char *name;
    int rows;
    int cols;
    int gpis;
    int gpos;
    int protocol;
    int payload;
} MODEL;

/* Fixme #1: number of GPI's & GPO's should be verified */
/* Fixme #2: protocol should be verified */
/* Fixme #3: number of keys on the keypad should be verified */

static MODEL Models[] = {
    {626, "626", 2, 16, 0, 0, 1, 0},
    {631, "631", 2, 20, 0, 0, 3, 22},
    {632, "632", 2, 16, 0, 0, 1, 0},
    {633, "633", 2, 16, 4, 4, 2, 18},
    {634, "634", 4, 20, 0, 0, 1, 0},
    {635, "635", 4, 20, 4, 12, 3, 22},
    {636, "636", 2, 16, 0, 0, 1, 0},
    {-1, "Unknown", -1, -1, 0, 0, 0, 0}
};


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/* x^0 + x^5 + x^12 */
#define CRCPOLY 0x8408

static unsigned short CRC(const unsigned char *p, size_t len, unsigned short seed)
{
    int i;
    while (len--) {
	seed ^= *p++;
	for (i = 0; i < 8; i++)
	    seed = (seed >> 1) ^ ((seed & 1) ? CRCPOLY : 0);
    }
    return ~seed;
}

static unsigned char LSB(const unsigned short word)
{
    return word & 0xff;
}

static unsigned char MSB(const unsigned short word)
{
    return word >> 8;
}


static unsigned char byte(unsigned int pos)
{
    pos += RingRPos;
    if (pos >= sizeof(RingBuffer))
	pos -= sizeof(RingBuffer);
    return RingBuffer[pos];
}


static void drv_CF_process_packet(void)
{

    switch (Packet.type) {

    case 0x02:

	/* async response from display to host */
	switch (Packet.code) {

	case 0x00:
	    /* Key Activity */
	    debug("Key Activity: %d", Packet.data[0]);
	    drv_generic_keypad_press(Packet.data[0]);
	    break;

	case 0x01:
	    /* Fan Speed Report */
	    if (Packet.data[1] == 0xff) {
		Fan_RPM[Packet.data[0]] = -1.0;
	    } else if (Packet.data[1] < 4) {
		Fan_RPM[Packet.data[0]] = 0.0;
	    } else {
		Fan_RPM[Packet.data[0]] =
		    (double) 27692308L *(Packet.data[1] - 3) / (Packet.data[2] + 256 * Packet.data[3]);
	    }
	    break;

	case 0x02:
	    /* Temperature Sensor Report */
	    switch (Packet.data[3]) {
	    case 0:
		error("%s: 1-Wire device #%d: CRC error", Name, Packet.data[0]);
		break;
	    case 1:
	    case 2:
		Temperature[Packet.data[0]] = (Packet.data[1] + 256 * Packet.data[2]) / 16.0;
		break;
	    default:
		error("%s: 1-Wire device #%d: unknown CRC status %d", Name, Packet.data[0], Packet.data[3]);
		break;
	    }
	    break;

	default:
	    /* this should not happen */
	    error("%s: unexpected response type=0x%02x code=0x%02x size=%d", Name, Packet.type, Packet.code,
		  Packet.size);
	    break;
	}

	break;

    case 0x03:
	/* error response from display to host */
	error("%s: error response type=0x%02x code=0x%02x size=%d", Name, Packet.type, Packet.code, Packet.size);
	break;

    default:
	/* these should not happen: */
	/* type 0x00: command from host to display: should never come back */
	/* type 0x01: command response from display to host: are processed within send() */
	error("%s: unexpected packet type=0x%02x code=0x%02x size=%d", Name, Packet.type, Packet.code, Packet.size);
	break;
    }

}


static int drv_CF_poll(void)
{
    /* read into RingBuffer */
    while (1) {
	char buffer[32];
	int num, n;
	num = drv_generic_serial_poll(buffer, sizeof(buffer));
	if (num <= 0)
	    break;
	/* put result into RingBuffer */
	for (n = 0; n < num; n++) {
	    RingBuffer[RingWPos++] = (unsigned char) buffer[n];
	    if (RingWPos >= sizeof(RingBuffer))
		RingWPos = 0;
	}
    }

    /* process RingBuffer */
    while (1) {
	unsigned char buffer[32];
	int n, num, size;
	unsigned short crc;
	/* packet size */
	num = RingWPos - RingRPos;
	if (num < 0)
	    num += sizeof(RingBuffer);
	/* minimum packet size=4 */
	if (num < 4)
	    return 0;
	/* valid response types: 01xxxxx 10.. 11.. */
	/* therefore: 00xxxxxx is invalid */
	if (byte(0) >> 6 == 0)
	    goto GARBAGE;
	/* command length */
	size = byte(1);
	/* valid command length is 0 to 16 */
	if (size > 16)
	    goto GARBAGE;
	/* all bytes available? */
	if (num < size + 4)
	    return 0;
	/* check CRC */
	for (n = 0; n < size + 4; n++)
	    buffer[n] = byte(n);
	crc = CRC(buffer, size + 2, 0xffff);
	if (LSB(crc) != buffer[size + 2])
	    goto GARBAGE;
	if (MSB(crc) != buffer[size + 3])
	    goto GARBAGE;
	/* process packet */
	Packet.type = buffer[0] >> 6;
	Packet.code = buffer[0] & 0x3f;
	Packet.size = size;
	memcpy(Packet.data, buffer + 2, size);
	Packet.data[size] = '\0';	/* trailing zero */
	/* increment read pointer */
	RingRPos += size + 4;
	if (RingRPos >= sizeof(RingBuffer))
	    RingRPos -= sizeof(RingBuffer);
	/* a packet arrived */
	return 1;
      GARBAGE:
	debug("dropping garbage byte %02x", byte(0));
	RingRPos++;
	if (RingRPos >= sizeof(RingBuffer))
	    RingRPos = 0;
	continue;
    }

    /* not reached */
    return 0;
}


static void drv_CF_timer(void __attribute__ ((unused)) * notused)
{
    while (drv_CF_poll()) {
	drv_CF_process_packet();
    }
}


static void drv_CF_send(const unsigned char cmd, const unsigned char len, const unsigned char *data)
{
    /* 1 cmd + 1 len + 22 payload + 2 crc = 26 */
    unsigned char buffer[26];
    unsigned short crc;
    struct timeval now, end;

    if (len > Payload) {
	error("%s: internal error: packet length %d exceeds payload size %d", Name, len, Payload);
	return;
    }

    buffer[0] = cmd;
    buffer[1] = len;
    memcpy(buffer + 2, data, len);
    crc = CRC(buffer, len + 2, 0xffff);
    buffer[len + 2] = LSB(crc);
    buffer[len + 3] = MSB(crc);

    drv_generic_serial_write((char *) buffer, len + 4);

    /* wait for acknowledge packet */
    gettimeofday(&now, NULL);
    while (1) {
	/* delay 1 msec */
	usleep(1 * 1000);
	if (drv_CF_poll()) {
	    if (Packet.type == 0x01 && Packet.code == cmd) {
		/* this is the ack we're waiting for */
		if (0) {
		    gettimeofday(&end, NULL);
		    debug("%s: ACK after %ld usec", Name,
			  1000000 * (end.tv_sec - now.tv_sec) + end.tv_usec - now.tv_usec);
		}
		break;
	    } else {
		/* some other (maybe async) packet, just process it */
		drv_CF_process_packet();
	    }
	}
	gettimeofday(&end, NULL);
	/* don't wait more than 250 msec */
	if ((1000000 * (end.tv_sec - now.tv_sec) + end.tv_usec - now.tv_usec) > 250 * 1000) {
	    error("%s: timeout waiting for response to cmd 0x%02x", Name, cmd);
	    break;
	}
    }
}


static void drv_CF_write1(const int row, const int col, const char *data, const int len)
{
    char cmd[3] = "\021xy";	/* set cursor position */

    if (row == 0 && col == 0) {
	drv_generic_serial_write("\001", 1);	/* cursor home */
    } else {
	cmd[1] = (char) col;
	cmd[2] = (char) row;
	drv_generic_serial_write(cmd, 3);
    }

    drv_generic_serial_write(data, len);
}


static void drv_CF_write2(const int row, const int col, const char *data, const int len)
{
    int l = len;

    /* limit length */
    if (col + l > 16)
	l = 16 - col;
    if (l < 0)
	l = 0;

    /* sanity check */
    if (row >= 2 || col + l > 16) {
	error("%s: internal error: write outside linebuffer bounds!", Name);
	return;
    }
    memcpy(Line + 16 * row + col, data, l);
    drv_CF_send(7 + row, 16, (unsigned char *) (Line + 16 * row));
}


static void drv_CF_write3(const int row, const int col, const char *data, const int len)
{
    int l = len;
    unsigned char cmd[23];

    /* limit length */
    if (col + l > DCOLS)
	l = DCOLS - col;
    if (l < 0)
	l = 0;

    /* sanity check */
    if (row >= DROWS || col + l > DCOLS) {
	error("%s: internal error: write outside display bounds!", Name);
	return;
    }

    cmd[0] = col;
    cmd[1] = row;
    memcpy(cmd + 2, data, l);

    drv_CF_send(31, l + 2, cmd);

}


static void drv_CF_defchar1(const int ascii, const unsigned char *matrix)
{
    int i;
    char cmd[10] = "\031n";	/* set custom char bitmap */

    /* user-defineable chars start at 128, but are defined at 0 */
    cmd[1] = (char) (ascii - CHAR0);
    for (i = 0; i < 8; i++) {
	cmd[i + 2] = matrix[i] & 0x3f;
    }
    drv_generic_serial_write(cmd, 10);
}


static void drv_CF_defchar23(const int ascii, const unsigned char *matrix)
{
    int i;
    unsigned char buffer[9];

    /* user-defineable chars start at 128, but are defined at 0 */
    buffer[0] = (char) (ascii - CHAR0);

    /* clear bit 6 and 7 of the bitmap (blinking) */
    for (i = 0; i < 8; i++) {
	buffer[i + 1] = matrix[i] & 0x3f;
    }

    drv_CF_send(9, 9, buffer);
}


static int drv_CF_contrast(int contrast)
{
    static unsigned char Contrast = 0;
    char buffer[2];

    /* -1 is used to query the current contrast */
    if (contrast == -1)
	return Contrast;

    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;
    Contrast = contrast;

    switch (Protocol) {

    case 1:
	/* contrast range 0 to 100 */
	if (Contrast > 100)
	    Contrast = 100;
	buffer[0] = 15;		/* Set LCD Contrast */
	buffer[1] = Contrast;
	drv_generic_serial_write(buffer, 2);
	break;

    case 2:
	/* contrast range 0 to 50 */
	if (Contrast > 50)
	    Contrast = 50;
	drv_CF_send(13, 1, &Contrast);
	break;

    case 3:
	/* contrast range 0 to 255 */
	drv_CF_send(13, 1, &Contrast);
	break;
    }

    return Contrast;
}


static int drv_CF_backlight(int backlight)
{
    static unsigned char Backlight = 0;
    char buffer[2];

    /* -1 is used to query the current backlight */
    if (backlight == -1)
	return Backlight;

    if (backlight < 0)
	backlight = 0;
    if (backlight > 100)
	backlight = 100;
    Backlight = backlight;

    switch (Protocol) {

    case 1:
	buffer[0] = 14;		/* Set LCD Backlight */
	buffer[1] = Backlight;
	drv_generic_serial_write(buffer, 2);
	break;

    case 2:
    case 3:
	drv_CF_send(14, 1, &Backlight);
	break;
    }

    return Backlight;
}


static int drv_CF_keypad(const int num)
{
    int val = 0;

    switch (Protocol) {
    case 1:
    case 2:
	break;
    case 3:
	if (num < 8)
	    val = WIDGET_KEY_PRESSED;
	else
	    val = WIDGET_KEY_RELEASED;
	switch (num) {
	case 1:
	case 8:
	    val += WIDGET_KEY_UP;
	    break;
	case 2:
	case 9:
	    val += WIDGET_KEY_DOWN;
	    break;
	case 3:
	case 10:
	    val += WIDGET_KEY_LEFT;
	    break;
	case 4:
	case 11:
	    val += WIDGET_KEY_RIGHT;
	    break;
	case 5:
	case 12:
	    val += WIDGET_KEY_CONFIRM;
	    break;
	case 7:
	case 13:
	    val += WIDGET_KEY_CANCEL;
	    break;
	}
	break;
    }

    return val;
}


static int drv_CF_GPI(const int num)
{
    if (num < 0 || num > 3) {
	return 0;
    }
    return Fan_RPM[num];
}


static int drv_CF_GPO(const int num, const int val)
{
    static unsigned char PWM2[4] = { 0, 0, 0, 0 };
    static unsigned char PWM3[2];

    int v = val;

    if (v < 0)
	v = 0;
    if (v > 100)
	v = 100;

    switch (Protocol) {
    case 2:
	PWM2[num] = v;
	drv_CF_send(17, 4, PWM2);
	break;
    case 3:
	PWM3[0] = num + 1;
	PWM3[1] = v;
	drv_CF_send(34, 2, PWM3);
	break;
    }

    return v;
}


static int drv_CF_autodetect(void)
{
    int m;

    /* only autodetect newer displays */
    if (Protocol < 2)
	return -1;

    /* read display type */
    drv_CF_send(1, 0, NULL);

    /* send() did already wait for response packet */
    if (Packet.type == 0x01 && Packet.code == 0x01) {
	char t[7], c;
	float h, v;
	info("%s: display identifies itself as '%s'", Name, Packet.data);
	if (sscanf((char *) Packet.data, "%6s:h%f,%c%f", t, &h, &c, &v) != 4) {
	    error("%s: error parsing display identification string", Name);
	    return -1;
	}
	info("%s: display type '%s', hardware version %3.1f, firmware version %c%3.1f", Name, t, h, c, v);
	if (strncmp(t, "CFA", 3) == 0) {
	    for (m = 0; Models[m].type != -1; m++) {
		/* omit the 'CFA' */
		if (strcasecmp(Models[m].name, t + 3) == 0)
		    return m;
	    }
	}
	error("%s: display type '%s' may be not supported!", Name, t);
	return -1;
    }

    error("%s: display detection failed!", Name);

    return -1;
}


static char *drv_CF_print_ROM(void)
{
    static char buffer[17];

    snprintf(buffer, sizeof(buffer), "0x%02x%02x%02x%02x%02x%02x%02x%02x",
	     Packet.data[1], Packet.data[2], Packet.data[3], Packet.data[4], Packet.data[5], Packet.data[6],
	     Packet.data[7], Packet.data[8]);

    return buffer;
}


static int drv_CF_scan_DOW(unsigned char index)
{
    int i;

    /* Read DOW Device Information */
    drv_CF_send(18, 1, &index);

    i = 0;
    while (1) {
	/* wait 10 msec */
	usleep(10 * 1000);
	/* packet available? */
	if (drv_CF_poll()) {
	    /* DOW Device Info */
	    if (Packet.type == 0x01 && Packet.code == 0x12) {
		switch (Packet.data[1]) {
		case 0x00:
		    /* no device found */
		    return 0;
		case 0x22:
		    info("%s: 1-Wire device #%d: DS1822 temperature sensor found at %s", Name, Packet.data[0],
			 drv_CF_print_ROM());
		    return 1;
		case 0x28:
		    info("%s: 1-Wire device #%d: DS18B20 temperature sensor found at %s", Name, Packet.data[0],
			 drv_CF_print_ROM());
		    return 1;
		default:
		    info("%s: 1-Wire device #%d: unknown device found at %s", Name, Packet.data[0], drv_CF_print_ROM());
		    return 0;
		}
	    } else {
		drv_CF_process_packet();
	    }
	}
	/* wait no longer than 300 msec */
	if (++i > 30) {
	    error("%s: 1-Wire device #%d detection timed out", Name, index);
	    return -1;
	}
    }

    /* not reached */
    return -1;
}


/* clear display */
static void drv_CF_clear(void)
{
    switch (Protocol) {
    case 1:
	drv_generic_serial_write("\014", 1);
	break;
    case 2:
    case 3:
	drv_CF_send(6, 0, NULL);
	break;
    }
}


/* init sequences for 626, 632, 634, 636  */
static void drv_CF_start_1(void)
{
    drv_generic_serial_write("\014", 1);	/* Form Feed (Clear Display) */
    drv_generic_serial_write("\004", 1);	/* hide cursor */
    drv_generic_serial_write("\024", 1);	/* scroll off */
    drv_generic_serial_write("\030", 1);	/* wrap off */
}


/* init sequences for 633 */
static void drv_CF_start_2(void)
{
    int i;
    unsigned long mask;
    unsigned char buffer[4];

    /* Clear Display */
    drv_CF_send(6, 0, NULL);

    /* Set LCD Cursor Style */
    buffer[0] = 0;
    drv_CF_send(12, 1, buffer);

    /* enable Fan Reporting */
    buffer[0] = 15;
    drv_CF_send(16, 1, buffer);

    /* Set Fan Power to 100% */
    buffer[0] = buffer[1] = buffer[2] = buffer[3] = 100;
    drv_CF_send(17, 4, buffer);

    /* Read DOW Device Information */
    mask = 0;
    for (i = 0; i < 32; i++) {
	if (drv_CF_scan_DOW(i) == 1) {
	    mask |= 1 << i;
	}
    }

    /* enable Temperature Reporting */
    buffer[0] = mask & 0xff;
    buffer[1] = (mask >> 8) & 0xff;
    buffer[2] = (mask >> 16) & 0xff;
    buffer[3] = (mask >> 24) & 0xff;
    drv_CF_send(19, 4, buffer);
}


/* init sequences for 631 */
static void drv_CF_start_3(void)
{
    unsigned char buffer[1];

    /* Clear Display */
    drv_CF_send(6, 0, NULL);

    /* Set LCD Cursor Style */
    buffer[0] = 0;
    drv_CF_send(12, 1, buffer);

}


static int drv_CF_start(const char *section)
{
    int i;
    char *model;

    model = cfg_get(section, "Model", NULL);
    if (model != NULL && *model != '\0') {
	for (i = 0; Models[i].type != -1; i++) {
	    if (strcasecmp(Models[i].name, model) == 0)
		break;
	}
	if (Models[i].type == -1) {
	    error("%s: %s.Model '%s' is unknown from %s", Name, section, model, cfg_source());
	    return -1;
	}
	Model = i;
	Protocol = Models[Model].protocol;
	info("%s: using model '%s'", Name, Models[Model].name);
    } else {
	Model = -1;
	Protocol = 2;		/*auto-detect only newer displays */
	info("%s: no '%s.Model' entry from %s, auto-detecting", Name, section, cfg_source());
    }

    /* open serial port */
    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    /* Fixme: why such a large delay? */
    usleep(350 * 1000);

    /* display autodetection */
    i = drv_CF_autodetect();
    if (Model == -1)
	Model = i;
    if (Model == -1) {
	error("%s: auto-detection failed, please specify a '%s.Model' entry in %s", Name, section, cfg_source());
	return -1;
    }
    if (i != -1 && Model != i) {
	error("%s: %s.Model '%s' from %s does not match detected model '%s'", Name, section, model, cfg_source(),
	      Models[i].name);
	return -1;
    }

    /* initialize global variables */
    DROWS = Models[Model].rows;
    DCOLS = Models[Model].cols;
    GPIS = Models[Model].gpis;
    GPOS = Models[Model].gpos;
    Protocol = Models[Model].protocol;
    Payload = Models[Model].payload;


    switch (Protocol) {

    case 1:
	drv_CF_start_1();
	break;

    case 2:
	/* regularly process display answers */
	/* Fixme: make 100msec configurable */
	timer_add(drv_CF_timer, NULL, 100, 0);
	drv_CF_start_2();
	/* clear 633 linebuffer */
	memset(Line, ' ', sizeof(Line));
	break;

    case 3:
	/* regularly process display answers */
	/* Fixme: make 100msec configurable */
	timer_add(drv_CF_timer, NULL, 100, 0);
	drv_CF_start_3();
	break;
    }

    /* set contrast */
    if (cfg_number(section, "Contrast", 0, 0, 255, &i) > 0) {
	drv_CF_contrast(i);
    }

    /* set backlight */
    if (cfg_number(section, "Backlight", 0, 0, 100, &i) > 0) {
	drv_CF_backlight(i);
    }

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
	contrast = drv_CF_contrast(-1);
	SetResult(&result, R_NUMBER, &contrast);
	break;
    case 1:
	contrast = drv_CF_contrast(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &contrast);
	break;
    default:
	error("%s.contrast(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
}


static void plugin_backlight(RESULT * result, const int argc, RESULT * argv[])
{
    double backlight;

    switch (argc) {
    case 0:
	backlight = drv_CF_backlight(-1);
	SetResult(&result, R_NUMBER, &backlight);
	break;
    case 1:
	backlight = drv_CF_backlight(R2N(argv[0]));
	SetResult(&result, R_NUMBER, &backlight);
	break;
    default:
	error("%s.backlight(): wrong number of parameters", Name);
	SetResult(&result, R_STRING, "");
    }
}


/* Fixme: other plugins for Fans, Temperature sensors, ... */



/****************************************/
/***        widget callbacks          ***/
/****************************************/

/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_gpio_draw(W) */
/* using drv_generic_keypad_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_CF_list(void)
{
    int i;

    for (i = 0; Models[i].type != -1; i++) {
	printf("%s ", Models[i].name);
    }
    return 0;
}


/* initialize driver & display */
int drv_CF_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s", Name, "$Revision: 1.47 $");

    /* start display */
    if ((ret = drv_CF_start(section)) != 0) {
	return ret;
    }

    /* display preferences */
    XRES = 6;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */

    /* real worker functions */
    switch (Protocol) {
    case 1:
	CHAR0 = 128;		/* ASCII of first user-defineable char */
	GOTO_COST = 3;		/* number of bytes a goto command requires */
	drv_generic_text_real_write = drv_CF_write1;
	drv_generic_text_real_defchar = drv_CF_defchar1;
	break;
    case 2:
	CHAR0 = 0;		/* ASCII of first user-defineable char */
	GOTO_COST = -1;		/* there is no goto on 633 */
	drv_generic_text_real_write = drv_CF_write2;
	drv_generic_text_real_defchar = drv_CF_defchar23;
	drv_generic_gpio_real_get = drv_CF_GPI;
	drv_generic_gpio_real_set = drv_CF_GPO;
	break;
    case 3:
	CHAR0 = 0;		/* ASCII of first user-defineable char */
	GOTO_COST = 3;		/* number of bytes a goto command requires */
	drv_generic_text_real_write = drv_CF_write3;
	drv_generic_text_real_defchar = drv_CF_defchar23;
	drv_generic_gpio_real_get = drv_CF_GPI;
	drv_generic_gpio_real_set = drv_CF_GPO;
	drv_generic_keypad_real_press = drv_CF_keypad;
	break;
    }

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %s", Name, Models[Model].name);
	if (drv_generic_text_greet(buffer, "www.crystalfontz.com")) {
	    sleep(3);
	    drv_CF_clear();
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
    if (Protocol == 2)
	drv_generic_text_bar_add_segment(255, 255, 255, 255);	/* ASCII 255 = block */

    /* initialize generic GPIO driver */
    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

    /* initialize generic key pad driver */
    if ((ret = drv_generic_keypad_init(section, Name)) != 0)
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
    AddFunction("LCD::contrast", -1, plugin_contrast);
    AddFunction("LCD::backlight", -1, plugin_backlight);

    return 0;
}


/* close driver & display */
int drv_CF_quit(const int quiet)
{

    info("%s: shutting down display.", Name);

    drv_generic_text_quit();
    drv_generic_gpio_quit();
    drv_generic_keypad_quit();

    /* clear display */
    drv_CF_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    drv_generic_serial_close();

    return (0);
}


DRIVER drv_Crystalfontz = {
  name:Name,
  list:drv_CF_list,
  init:drv_CF_init,
  quit:drv_CF_quit,
};
