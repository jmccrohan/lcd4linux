/* $Id: Cwlinux.c,v 1.1 2002/09/11 05:16:32 reinelt Exp $
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

#define XRES 6
#define YRES 8
#define CHARS 7
#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 )

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

typedef struct {
    int len1;
    int len2;
    int type;
    int segment;
} BAR;

typedef struct {
    int len1;
    int len2;
    int type;
    int used;
    int ascii;
} SEGMENT;

static LCD Lcd;
static char *Port = NULL;
static speed_t Speed;
static int Device = -1;

static char Txt[4][20];
static BAR Bar[4][20];

static int nSegment = 2;
static SEGMENT Segment[128] = { {len1: 0, len2: 0, type: 255, \
				used: 0, ascii:32} };

int Read_LCD(int fd, char *c, int size)
{
    int rc;

    rc = read(fd, c, size);
    usleep(DELAY);
    return rc;
}

int Write_LCD(int fd, char *c, int size)
{
    int rc;

    rc = write(fd, c, size);
    usleep(DELAY);
    return rc;
}


void Enable_Cursor(int fd)
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

void Disable_Cursor(int fd)
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

void Clear_Screen(int fd)
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

void Enable_Wrap(int fd)
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

void Init_Port(fd)
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

void Setup_Port(int fd, speed_t speed)
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

void Set_9600(int fd)
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

void Set_19200(int fd)
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

int Write_Line_LCD(int fd, char *buf, int size)
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

void Set_Insert(int fd, int row, int col)
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

void CwLnx_backlight(int on)
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

static void CwLnx_process_bars(void)
{
    int row, col;
    int i, j;

    for (i = 2; i < nSegment && Segment[i].used; i++);
    for (j = i + 1; j < nSegment; j++) {
	if (Segment[j].used)
	    Segment[i++] = Segment[j];
    }
    nSegment = i;

    for (row = 0; row < Lcd.rows; row++) {
	for (col = 0; col < Lcd.cols; col++) {
	    if (Bar[row][col].type == 0)
		continue;
	    for (i = 0; i < nSegment; i++) {
		if (Segment[i].type & Bar[row][col].type &&
		    Segment[i].len1 == Bar[row][col].len1 &&
		    Segment[i].len2 == Bar[row][col].len2)
		    break;
	    }
	    if (i == nSegment) {
		nSegment++;
		Segment[i].len1 = Bar[row][col].len1;
		Segment[i].len2 = Bar[row][col].len2;
		Segment[i].type = Bar[row][col].type;
		Segment[i].used = 0;
		Segment[i].ascii = -1;
	    }
	    Bar[row][col].segment = i;
	}
    }
}

static int CwLnx_segment_diff(int i, int j)
{
    int RES;
    int i1, i2, j1, j2;

    if (i == j)
	return 65535;
    if (!(Segment[i].type & Segment[j].type))
	return 65535;
    if (Segment[i].len1 == 0 && Segment[j].len1 != 0)
	return 65535;
    if (Segment[i].len2 == 0 && Segment[j].len2 != 0)
	return 65535;
    RES = Segment[i].type & BAR_H ? XRES : YRES;
    if (Segment[i].len1 >= RES && Segment[j].len1 < RES)
	return 65535;
    if (Segment[i].len2 >= RES && Segment[j].len2 < RES)
	return 65535;
    if (Segment[i].len1 == Segment[i].len2
	&& Segment[j].len1 != Segment[j].len2)
	return 65535;

    i1 = Segment[i].len1;
    if (i1 > RES)
	i1 = RES;
    i2 = Segment[i].len2;
    if (i2 > RES)
	i2 = RES;
    j1 = Segment[j].len1;
    if (j1 > RES)
	i1 = RES;
    j2 = Segment[j].len2;
    if (j2 > RES)
	i2 = RES;

    return (i1 - i2) * (i1 - i2) + (j1 - j2) * (j1 - j2);
}

static void CwLnx_compact_bars(void)
{
    int i, j, r, c, min;
    int pack_i, pack_j;
    int pass1 = 1;
    int deviation[nSegment][nSegment];

    if (nSegment > CHARS + 2) {

	for (i = 2; i < nSegment; i++) {
	    for (j = 0; j < nSegment; j++) {
		deviation[i][j] = CwLnx_segment_diff(i, j);
	    }
	}

	while (nSegment > CHARS + 2) {
	    min = 65535;
	    pack_i = -1;
	    pack_j = -1;
	    for (i = 2; i < nSegment; i++) {
		if (pass1 && Segment[i].used)
		    continue;
		for (j = 0; j < nSegment; j++) {
		    if (deviation[i][j] < min) {
			min = deviation[i][j];
			pack_i = i;
			pack_j = j;
		    }
		}
	    }
	    if (pack_i == -1) {
		if (pass1) {
		    pass1 = 0;
		    continue;
		} else {
		    error("Cwlinux: unable to compact bar characters");
		    nSegment = CHARS;
		    break;
		}
	    }

	    nSegment--;
	    Segment[pack_i] = Segment[nSegment];

	    for (i = 0; i < nSegment; i++) {
		deviation[pack_i][i] = deviation[nSegment][i];
		deviation[i][pack_i] = deviation[i][nSegment];
	    }

	    for (r = 0; r < Lcd.rows; r++) {
		for (c = 0; c < Lcd.cols; c++) {
		    if (Bar[r][c].segment == pack_i)
			Bar[r][c].segment = pack_j;
		    if (Bar[r][c].segment == nSegment)
			Bar[r][c].segment = pack_i;
		}
	    }
	}
    }
}

void CwLnx_set_char(int n, char *dat)
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
	    Bar[row][col].len1 = -1;
	    Bar[row][col].len2 = -1;
	    Bar[row][col].type = 0;
	    Bar[row][col].segment = -1;
	}
    }
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
    int rev = 0;

    if (len1 < 1)
	len1 = 1;
    else if (len1 > max)
	len1 = max;

    if (len2 < 1)
	len2 = 1;
    else if (len2 > max)
	len2 = max;

    switch (type) {
    case BAR_L:
	len1 = max - len1;
	len2 = max - len2;
	rev = 1;

    case BAR_R:
	while (max > 0 && col < Lcd.cols) {
	    Bar[row][col].type = type;
	    Bar[row][col].segment = -1;
	    if (len1 >= XRES) {
		Bar[row][col].len1 = rev ? 0 : XRES;
		len1 -= XRES;
	    } else {
		Bar[row][col].len1 = rev ? XRES - len1 : len1;
		len1 = 0;
	    }
	    if (len2 >= XRES) {
		Bar[row][col].len2 = rev ? 0 : XRES;
		len2 -= XRES;
	    } else {
		Bar[row][col].len2 = rev ? XRES - len2 : len2;
		len2 = 0;
	    }
	    max -= XRES;
	    col++;
	}
	break;

    case BAR_U:
	len1 = max - len1;
	len2 = max - len2;
	rev = 1;

    case BAR_D:
	while (max > 0 && row < Lcd.rows) {
	    Bar[row][col].type = type;
	    Bar[row][col].segment = -1;
	    if (len1 >= YRES) {
		Bar[row][col].len1 = rev ? 0 : YRES;
		len1 -= YRES;
	    } else {
		Bar[row][col].len1 = rev ? YRES - len1 : len1;
		len1 = 0;
	    }
	    if (len2 >= YRES) {
		Bar[row][col].len2 = rev ? 0 : YRES;
		len2 -= YRES;
	    } else {
		Bar[row][col].len2 = rev ? YRES - len2 : len2;
		len2 = 0;
	    }
	    max -= YRES;
	    row++;
	}
	break;

    }
    return 0;
}

int CwLnx_flush(void)
{
    char buffer[256];
    char *p;
    int s, row, col;

    CwLnx_process_bars();
    CwLnx_compact_bars();
    CwLnx_define_chars();

    for (s = 0; s < nSegment; s++) {
	Segment[s].used = 0;
    }

    for (row = 0; row < Lcd.rows; row++) {
	for (col = 0; col < Lcd.cols; col++) {
	    s = Bar[row][col].segment;
	    if (s != -1) {
		Segment[s].used = 1;
		Txt[row][col] = Segment[s].ascii;
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
    {"CW12232", 4, 20, XRES, YRES, BARS, 0, CwLnx_init, CwLnx_clear, \
	    CwLnx_put, CwLnx_bar, NULL, CwLnx_flush, CwLnx_quit},
    {NULL}
};
