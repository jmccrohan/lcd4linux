/* $Id: Cwlinux.c,v 1.2 2002/09/11 05:32:35 reinelt Exp $
 *
 * driver for Cwlinux serial display modules
 *
 * Copyright 2002 by Andrew Ip (aip@cwlinux.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: Cwlinux.c,v $
 * Revision 1.2  2002/09/11 05:32:35  reinelt
 * changed to use new bar functions
 *
 * Revision 1.1  2002/09/11 05:16:32  reinelt
 * added Cwlinux driver
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD Cwlinux[]
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "lock.h"
#include "display.h"
#include "bar.h"

#define XRES 6
#define YRES 8
#define CHARS 7

#define LCD_CMD			254
#define LCD_CMD_END		253
#define LCD_INIT_CHINESE_T	56
#define LCD_INIT_CHINESE_S	55
#define LCD_LIGHT_ON		66
#define LCD_LIGHT_OFF		70
#define LCD_CLEAR		88
#define LCD_SET_INSERT		71
#define LCD_INIT_INSERT		72
#define LCD_SET_BAUD		57
#define LCD_ENABLE_WRAP		67
#define LCD_DISABLE_WRAP	68
#define LCD_SETCHAR		78
#define LCD_ENABLE_CURSOR	81
#define LCD_DISABLE_CURSOR	82

#define LCD_LENGTH		20

#define DELAY			20
#define UPDATE_DELAY		0	/* 1 sec */
#define SETUP_DELAY		0	/* 2 sec */

static LCD Lcd;
static char *Port = NULL;
static speed_t Speed;
static int Device = -1;

static char Txt[4][20];

static int nSegment = 2;
static SEGMENT Segment[128] = { {len1: 0, len2: 0, type: 255, \
				used: 0, ascii:32} };

static int Read_LCD(int fd, char *c, int size)
{
    int rc;

    rc = read(fd, c, size);
    usleep(DELAY);
    return rc;
}

static int Write_LCD(int fd, char *c, int size)
{
    int rc;

    rc = write(fd, c, size);
    usleep(DELAY);
    return rc;
}


static void Enable_Cursor(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_ENABLE_CURSOR;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static void Disable_Cursor(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_DISABLE_CURSOR;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static void Clear_Screen(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CLEAR;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static void Enable_Wrap(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_ENABLE_WRAP;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static void Init_Port(fd)
{
    /* Posix - set baudrate to 0 and back */
    struct termios tty, old;

    tcgetattr(fd, &tty);
    tcgetattr(fd, &old);
    cfsetospeed(&tty, B0);
    cfsetispeed(&tty, B0);
    tcsetattr(fd, TCSANOW, &tty);
    usleep(SETUP_DELAY);
    tcsetattr(fd, TCSANOW, &old);
}

static void Setup_Port(int fd, speed_t speed)
{
    struct termios portset;

    tcgetattr(fd, &portset);
    cfsetospeed(&portset, speed);
    cfsetispeed(&portset, speed);
    portset.c_iflag = IGNBRK;
    portset.c_lflag = 0;
    portset.c_oflag = 0;
    portset.c_cflag |= CLOCAL | CREAD;
    portset.c_cflag &= ~CRTSCTS;
    portset.c_cc[VMIN] = 1;
    portset.c_cc[VTIME] = 5;
    tcsetattr(fd, TCSANOW, &portset);
}

static void Set_9600(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_SET_BAUD;
    rc = Write_LCD(fd, &c, 1);
    c = 0x20;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static void Set_19200(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_SET_BAUD;
    rc = Write_LCD(fd, &c, 1);
    c = 0xf;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static int Write_Line_LCD(int fd, char *buf, int size)
{
    int i;
    char c;
    int isEnd = 0;
    int rc;

    for (i = 0; i < size; i++) {
	if (buf[i] == '\0') {
	    isEnd = 1;
	}
	if (isEnd) {
	    c = ' ';
	} else {
	    c = buf[i];
	}
	rc = Write_LCD(fd, &c, 1);
    }
    printf("%s\n", buf);
    return 0;
}

static void Set_Insert(int fd, int row, int col)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_SET_INSERT;
    rc = Write_LCD(fd, &c, 1);
    c = col;
    rc = Write_LCD(fd, &c, 1);
    c = row;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

static void CwLnx_backlight(int on)
{
    static int current = -1;
    int realbacklight = -1;
    char c;
    int rc;

    if (on == current)
	return;

    /* validate backlight value */
    if (on > 255)
	on = 255;
    if (on < 0)
	on = 0;

    current = on;

    realbacklight = (int) (current * 100 / 255);

    c = LCD_CMD;
    rc = Write_LCD(Device, &c, 1);
    c = LCD_LIGHT_ON;
    rc = Write_LCD(Device, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(Device, &c, 1);
}

static void CwLnx_linewrap(int on)
{
    char out[4];

    if (on)
	snprintf(out, sizeof(out), "%c", 23);
    else
	snprintf(out, sizeof(out), "%c", 24);
    Enable_Wrap(Device);
}

static void CwLnx_autoscroll(int on)
{
    return;
}

static void CwLnx_hidecursor()
{
    return;
}

static int CwLnx_open(void)
{
    int fd;
    pid_t pid;
    struct termios portset;

    if ((pid = lock_port(Port)) != 0) {
	if (pid == -1)
	    error("Cwlinux: port %s could not be locked", Port);
	else
	    error("Cwlinux: port %s is locked by process %d", Port, pid);
	return -1;
    }
    fd = open(Port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
	error("Cwlinux: open(%s) failed: %s", Port, strerror(errno));
	unlock_port(Port);
	return -1;
    }
    if (tcgetattr(fd, &portset) == -1) {
	error("Cwlinux: tcgetattr(%s) failed: %s", Port, strerror(errno));
	unlock_port(Port);
	return -1;
    }
    cfmakeraw(&portset);
    cfsetospeed(&portset, Speed);
    if (tcsetattr(fd, TCSANOW, &portset) == -1) {
	error("Cwlinux: tcsetattr(%s) failed: %s", Port, strerror(errno));
	unlock_port(Port);
	return -1;
    }
    return fd;
}

static void CwLnx_write(char *string, int len)
{
    if (Device == -1)
	return;

    if (Write_Line_LCD(Device, string, len) < 0) {
	error("Cwlinux: write(%s) failed: %s", Port, strerror(errno));
    }
}

static int CwLnx_contrast(void)
{
    return 0;
}

static void CwLnx_set_char(int n, char *dat)
{
    int row, col;
    char letter;
    char c;
    int rc;

    if (n < 1 || n > 8)
	return;
    if (!dat)
	return;

    c = LCD_CMD;
    rc = Write_LCD(Device, &c, 1);
    c = LCD_SETCHAR;
    rc = Write_LCD(Device, &c, 1);
    c = (char)n;
    rc = Write_LCD(Device, &c, 1);

    for (col = 0; col < 6; col++) {
	letter = 0;
	for (row = 0; row < 8; row++) {
	    letter <<= 1;
	    letter |= (dat[(col * 8) + row] > 0);
	}
	Write_LCD(Device, &letter, 1);
    }
    c = LCD_CMD_END;
    rc = Write_LCD(Device, &c, 1);
}

static void CwLnx_define_chars(void)
{
    int c, i, j, m, n;
    char buffer[48];
    char Pixelr[] = { 1, 9, 17, 25, 33, 41 };
    char Pixell[] = { 40, 32, 24, 16, 8, 0 };

    for (i = 2; i < nSegment; i++) {
	if (Segment[i].used)
	    continue;
	if (Segment[i].ascii != -1)
	    continue;
	for (c = 1; c < CHARS; c++) {
	    for (j = 2; j < nSegment; j++) {
		if (Segment[j].ascii == c)
		    break;
	    }
	    if (j == nSegment)
		break;
	}
	Segment[i].ascii = c;
	memset(buffer, 0, 48);
	switch (Segment[i].type) {
	case BAR_L:
	    for (n = 0; n < Segment[i].len1 - 1; n++) {
		for (m = 0; m < 4; m++) {
		    buffer[Pixell[n] + 4 + m] = 1;
		}
	    }
	    for (n = 0; n < Segment[i].len2 - 1; n++) {
		for (m = 0; m < 4; m++) {
		    buffer[Pixell[n] + 0 + m] = 1;
		}
	    }
	    break;
	case BAR_R:
	    for (n = 0; n < Segment[i].len1; n++) {
		for (m = 0; m < 3; m++) {
		    buffer[Pixelr[n] + 4 + m] = 1;
		}
	    }
	    for (n = 0; n < Segment[i].len2; n++) {
		for (m = 0; m < 3; m++) {
		    buffer[Pixelr[n] + 0 + m] = 1;
		}
	    }
	    break;
	case BAR_U:
	    for (j = 0; j < Segment[i].len1; j++) {
		for (m = 0; m < 3; m++) {
		    buffer[j + m * 8] = 1;
		}
	    }
	    for (j = 0; j < Segment[i].len2; j++) {
		for (m = 0; m < 3; m++) {
		    buffer[j + m * 8 + 16] = 1;
		}
	    }
	    break;
	case BAR_D:
	    for (j = 0; j < Segment[i].len1; j++) {
		for (m = 0; m < 3; m++) {
		    buffer[7 - j + m * 8] = 1;
		}
	    }
	    for (j = 0; j < Segment[i].len2; j++) {
		for (m = 0; m < 3; m++) {
		    buffer[7 - j + m * 8 + 16] = 1;
		}
	    }
	    break;
	}
	CwLnx_set_char(c, buffer);
    }
}

int CwLnx_clear(void)
{
    int row, col;
    for (row = 0; row < Lcd.rows; row++) {
	for (col = 0; col < Lcd.cols; col++) {
	    Txt[row][col] = '\t';
	}
    }

    bar_clear();

    Clear_Screen(Device);
    return 0;
}

int CwLnx_init(LCD * Self)
{
    char *port;
    char *speed;

    Lcd = *Self;

    if (Port) {
	free(Port);
	Port = NULL;
    }

    port = cfg_get("Port");
    if (port == NULL || *port == '\0') {
	error("Cwlinux: no 'Port' entry in %s", cfg_file());
	return -1;
    }
    Port = strdup(port);

    speed = cfg_get("Speed") ? : "19200";

    switch (atoi(speed)) {
    case 1200:
	Speed = B1200;
	break;
    case 2400:
	Speed = B2400;
	break;
    case 9600:
	Speed = B9600;
	break;
    case 19200:
	Speed = B19200;
	break;
    default:
	error("Cwlinux: unsupported speed '%s' in %s", speed, cfg_file());
	return -1;
    }

    debug("using port %s at %d baud", Port, atoi(speed));

    Device = CwLnx_open();
    if (Device == -1)
	return -1;

    bar_init(Lcd.rows, Lcd.cols, XRES, YRES, CHARS);
    bar_add_segment(  0,  0,255, 32); // ASCII  32 = blank
    bar_add_segment(255,255,255,255); // ASCII 255 = block
    
    CwLnx_clear();
    CwLnx_contrast();

    CwLnx_backlight(1);
    CwLnx_hidecursor(0);
    CwLnx_linewrap(0);
    CwLnx_autoscroll(0);

    return 0;
}

int CwLnx_put(int row, int col, char *text)
{
    char *p = &Txt[row][col];
    char *t = text;

    while (*t && col++ <= Lcd.cols) {
	*p++ = *t++;
    }
    return 0;
}

int CwLnx_bar(int type, int row, int col, int max, int len1, int len2)
{
  return bar_draw (type, row, col, max, len1, len2);
}

int CwLnx_flush(void)
{
    char buffer[256];
    char *p;
    int c, row, col;

    bar_process(CwLnx_define_char);

    for (row = 0; row < Lcd.rows; row++) {
	for (col = 0; col < Lcd.cols; col++) {
	    c=bar_peek(row, col);
	    if (c!=-1) {
	        Txt[row][col]=(char)c;
	    }
	}
	Set_Insert(Device, row, col);
	for (col = 0; col < Lcd.cols; col++) {
	    if (Txt[row][col] == '\t')
		continue;
	    Set_Insert(Device, row, col);
	    for (p = buffer; col < Lcd.cols; col++, p++) {
		if (Txt[row][col] == '\t')
		    break;
		*p = Txt[row][col];
	    }
	    CwLnx_write(buffer, p - buffer);
	}
    }
    return 0;
}

int CwLnx_quit(void)
{
    debug("closing port %s", Port);
    close(Device);
    unlock_port(Port);
    return (0);
}

LCD Cwlinux[] = {
    {name: "CW12232", 
     rows:  4, 
     cols:  20, 
     xres:  XRES, 
     yres:  YRES, 
     bars:  BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2,
     gpos:  0, 
     init:  CwLnx_init, 
     clear: CwLnx_clear,
     put:   CwLnx_put, 
     bar:   CwLnx_bar, 
     gpo:   NULL, 
     flush: CwLnx_flush, 
     quit:  CwLnx_quit
    },
    {NULL}
};
