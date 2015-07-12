/* $Id$
 * $URL$
 *
 * TeakLCM lcd4linux driver
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 * Copyright (C) 2011 Hans Ulrich Niedermann <hun@n-dimensional.de>
 * Copyright (C) 2011, 2012 Andreas Thienemann <andreas@bawue.net>
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
 * struct DRIVER drv_TeakLCM
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <assert.h>

#include "event.h"
#include "timer.h"
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
#include "drv_generic_serial.h"


static char Name[] = "TeakLCM";


static int global_reset_rx_flag = 0;


#define HI8(value) ((uint8_t)(((value)>>8) & 0xff))
#define LO8(value) ((uint8_t)((value) & 0xff))


static uint16_t CRC16(uint8_t value, uint16_t crcin)
{
    uint16_t k = (((crcin >> 8) ^ value) & 255) << 8;
    uint16_t crc = 0;
    int bits;
    for (bits = 8; bits; --bits) {
	if ((crc ^ k) & 0x8000)
	    crc = (crc << 1) ^ 0x1021;
	else
	    crc <<= 1;
	k <<= 1;
    }
    return ((crcin << 8) ^ crc);
}


/** Return a printable character */
static char printable(const char ch)
{
    if ((32 <= ch) && (ch < 127)) {
	return ch;
    } else {
	return '.';
    }
}


static void debug_data_int(const char *prefix, const void *data, const size_t size, const unsigned int delta)
{
    const uint8_t *b = (const uint8_t *) data;
    size_t y;
    assert(delta <= 24);
    for (y = 0; y < size; y += delta) {
	char buf[100];
	size_t x;
	ssize_t idx = 0;
	idx += sprintf(&(buf[idx]), "%04x ", y);
	for (x = 0; x < delta; x++) {
	    const size_t i = x + y;
	    if (i < size) {
		idx += sprintf(&(buf[idx]), " %02x", b[i]);
	    } else {
		idx += sprintf(&(buf[idx]), "   ");
	    }
	}
	idx += sprintf(&buf[idx], "  ");
	for (x = 0; x < delta; x++) {
	    const size_t i = x + y;
	    if (i < size) {
		idx += sprintf(&buf[idx], "%c", printable(b[i]));
	    } else {
		idx += sprintf(&buf[idx], " ");
	    }
	}
	debug("%s%s", prefix, buf);
    }
}


static void debug_data(const char *prefix, const void *data, const size_t size)
{
    debug_data_int(prefix, data, size, 16);
}


typedef enum {
    CMD_CONNECT = 0x05,
    CMD_DISCONNECT = 0x06,
    CMD_ALARM = 0x07,
    CMD_WRITE = 0x08,
    CMD_PRINT1 = 0x09,
    CMD_PRINT2 = 0x0A,
    CMD_ACK = 0x0B,
    CMD_NACK = 0x0C,
    CMD_CONFIRM = 0x0D,
    CMD_RESET = 0x0E,

    LCM_CLEAR = 0x21,
    LCM_HOME = 0x22,
    LCM_CURSOR_SHIFT_R = 0x23,
    LCM_CURSOR_SHIFT_L = 0x24,
    LCM_BACKLIGHT_ON = 0x25,
    LCM_BACKLIGHT_OFF = 0x26,
    LCM_LINE2 = 0x27,
    LCM_DISPLAY_SHIFT_R = 0x28,
    LCM_DISPLAY_SHIFT_L = 0x29,
    LCM_CURSOR_ON = 0x2A,
    LCM_CURSOR_OFF = 0x2B,
    LCM_CURSOR_BLINK = 0x2C,
    LCM_DISPLAY_ON = 0x2D,
    LCM_DISPLAY_OFF = 0x2E
} lcm_cmd_t;


static
const char *cmdstr(const lcm_cmd_t cmd)
{
    switch (cmd) {
#define D(CMD) case CMD_ ## CMD: return "CMD_" # CMD; break;
	D(CONNECT);
	D(DISCONNECT);
	D(ACK);
	D(NACK);
	D(CONFIRM);
	D(RESET);
	D(ALARM);
	D(WRITE);
	D(PRINT1);
	D(PRINT2);
#undef D
#define D(CMD) case LCM_ ## CMD: return "LCM_" # CMD; break;
	D(CLEAR);
	D(HOME);
	D(CURSOR_SHIFT_R);
	D(CURSOR_SHIFT_L);
	D(BACKLIGHT_ON);
	D(BACKLIGHT_OFF);
	D(LINE2);
	D(DISPLAY_SHIFT_R);
	D(DISPLAY_SHIFT_L);
	D(CURSOR_ON);
	D(CURSOR_OFF);
	D(CURSOR_BLINK);
	D(DISPLAY_ON);
	D(DISPLAY_OFF);
#undef D
    }
    return "CMD_UNKNOWN";
}


/*
 * Magic defines
 */

#define LCM_FRAME_MASK      0xFF
#define LCM_TIMEOUT         2
#define LCM_ESC             0x1B

#define LCM_KEY1            0x31
#define LCM_KEY2            0x32
#define LCM_KEY3            0x33
#define LCM_KEY4            0x34
#define LCM_KEY12           0x35
#define LCM_KEY13           0x36
#define LCM_KEY14           0x37
#define LCM_KEY23           0x38
#define LCM_KEY24           0x39
#define LCM_KEY34           0x3A


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

/* global LCM state machine */


struct _lcm_fsm_t;
typedef struct _lcm_fsm_t lcm_fsm_t;


typedef enum {
    ST_IDLE,			/* mode == 0, IDLE */
    ST_COMMAND,			/* mode == 1, COMMAND */
    ST_CONNECTED		/* mode == 2, CONNECTED */
} lcm_state_t;


static
const char *state2str(const lcm_state_t state)
{
    switch (state) {
    case ST_IDLE:
	return "ST_IDLE (0)";
	break;
    case ST_COMMAND:
	return "ST_COMMAND (1)";
	break;
    case ST_CONNECTED:
	return "ST_CONNECTED (2)";
	break;
    }
    return "ST_UNKNOWN";
}


#if 0
static
void repeat_connect_to_display_callback(void *data);
#endif

static
void lcm_send_cmd(lcm_cmd_t cmd);

static
void drv_TeakLCM_clear(void);


static
void raw_send_cmd_frame(lcm_cmd_t cmd);

static
void raw_send_data_frame(lcm_cmd_t cmd, const char *data, const unsigned int len);


static
lcm_state_t fsm_get_state(lcm_fsm_t * fsm);

static
void fsm_handle_cmd(lcm_fsm_t * fsm, const lcm_cmd_t cmd);

static
void fsm_handle_datacmd(lcm_fsm_t * fsm, const lcm_cmd_t cmd, const uint8_t * payload, const unsigned int payload_len);

static
void try_reset(void);

static
void fsm_step(lcm_fsm_t * fsm);

static
void fsm_trans_noop(lcm_fsm_t * fsm, const lcm_state_t next_state);

static
void fsm_trans_cmd(lcm_fsm_t * fsm, const lcm_state_t next_state, const lcm_cmd_t cmd);

static
void fsm_trans_data(lcm_fsm_t * fsm,
		    const lcm_state_t next_state, const lcm_cmd_t cmd, const char *data, const unsigned int len);


static
void fsm_handle_bytes(lcm_fsm_t * fsm, uint8_t * rxbuf, const unsigned int buflen)
{
    if ((buflen >= 3) && (rxbuf[0] == LCM_FRAME_MASK) && (rxbuf[2] == LCM_FRAME_MASK)) {
	const lcm_cmd_t cmd = rxbuf[1];
	debug("%s Received cmd frame (cmd=%d=%s)", __FUNCTION__, cmd, cmdstr(cmd));
	fsm_handle_cmd(fsm, cmd);
	if (buflen > 3) {
	    /* recursively handle remaining bytes */
	    fsm_handle_bytes(fsm, &rxbuf[3], buflen - 3);
	}
	return;
    } else if ((buflen > 3) && (rxbuf[0] == LCM_FRAME_MASK)) {
	unsigned int ri;	/* raw indexed */
	unsigned int ci;	/* cooked indexed, i.e. after unescaping */

	debug("%s Received possible data frame", __FUNCTION__);

	/* unescape rxframe data in place */
	uint16_t crc0 = 0, crc1 = 0, crc2 = 0, crc3 = 0;
	for (ri = 1, ci = 1; ri < buflen; ri++) {
	    switch (rxbuf[ri]) {
	    case LCM_ESC:
		ri++;
		/* fall through */
	    default:
		rxbuf[ci++] = rxbuf[ri];
		crc3 = crc2;
		crc2 = crc1;
		crc1 = crc0;
		crc0 = CRC16(rxbuf[ri], crc0);
		break;
	    }
	    if ((rxbuf[ci - 1] == LCM_FRAME_MASK) && (rxbuf[ci - 2] == LO8(crc3)) && (rxbuf[ci - 3] == HI8(crc3))) {
		/* looks like a complete data frame */
		lcm_cmd_t cmd = rxbuf[1];
		uint16_t len = (rxbuf[3] << 8) + rxbuf[2];
		assert(ci == (unsigned int) (1 + 1 + 2 + len + 2 + 1));
		fsm_handle_datacmd(fsm, cmd, &rxbuf[4], len);
		if (ri + 1 < buflen) {
		    /* recursively handle remaining bytes */
		    fsm_handle_bytes(fsm, &rxbuf[ri + 1], buflen - ri);
		}
		return;
	    }
	}

	fsm_trans_cmd(fsm, fsm_get_state(fsm),	/* TODO: Is this a good next_state value? */
		      CMD_NACK);
	debug("%s checksum/framemask error", __FUNCTION__);
	return;
    } else {
	debug("%s Received garbage data:", __FUNCTION__);
	debug_data(" RXD ", rxbuf, buflen);
	return;
    }
}


static void fsm_handle_cmd(lcm_fsm_t * fsm, lcm_cmd_t cmd)
{
    // debug("fsm_handle_cmd: old state 0x%02x %s", lcm_mode, modestr(lcm_mode));
    const lcm_state_t old_state = fsm_get_state(fsm);
    if (CMD_RESET == cmd) {
	global_reset_rx_flag = 1;
    }
    switch (old_state) {
    case ST_IDLE:
    case ST_COMMAND:
	switch (cmd) {
	case CMD_CONNECT:
	    fsm_trans_cmd(fsm, ST_COMMAND, CMD_ACK);
	    break;
	case CMD_ACK:
	    fsm_trans_cmd(fsm, ST_CONNECTED, CMD_CONFIRM);
	    break;
	case CMD_NACK:
	    fsm_trans_cmd(fsm, ST_IDLE, CMD_CONFIRM);
	    break;
	case CMD_CONFIRM:
	    fsm_trans_noop(fsm, ST_CONNECTED);
	    break;
	case CMD_RESET:
	    fsm_trans_cmd(fsm, ST_COMMAND, CMD_CONNECT);
	    break;
	default:
	    error("%s: Unhandled cmd %s in state %s", Name, cmdstr(cmd), state2str(old_state));
	    fsm_trans_cmd(fsm, ST_IDLE, CMD_NACK);
	    break;
	}
	break;
    case ST_CONNECTED:		/* "if (mode == 2)" */
	switch (cmd) {
	case CMD_ACK:
	    fsm_trans_cmd(fsm, ST_CONNECTED, CMD_CONFIRM);
	    break;
	case CMD_CONNECT:
	    fsm_trans_cmd(fsm, ST_CONNECTED, CMD_NACK);
	    break;
	case CMD_DISCONNECT:
	    fsm_trans_cmd(fsm, ST_CONNECTED, CMD_ACK);
	    break;
	case CMD_RESET:
	    fsm_trans_cmd(fsm, ST_IDLE, CMD_CONNECT);
	    break;
	default:
	    debug("%s: Ignoring unhandled cmd %s in state %s", Name, cmdstr(cmd), state2str(old_state));
	    fsm_trans_noop(fsm, ST_CONNECTED);
	    break;
	}
	break;
    }
    fsm_step(fsm);
}


static
void fsm_handle_datacmd(lcm_fsm_t * fsm, const lcm_cmd_t cmd, const uint8_t * payload, unsigned int payload_len)
{
    const lcm_state_t old_state = fsm_get_state(fsm);
    debug("fsm_handle_datacmd: old state 0x%02x %s", old_state, state2str(old_state));
    switch (old_state) {
    case ST_CONNECTED:
	switch (cmd) {
	case CMD_WRITE:
	    assert(payload_len == 1);
	    debug("Got a key code 0x%02x", *payload);
	    fsm_trans_noop(fsm, ST_CONNECTED);
	    // lcm_send_cmd_frame(CMD_ACK);
	    break;
	default:
	    debug("Got an unknown data frame: %d=%s", cmd, cmdstr(cmd));
	    fsm_trans_noop(fsm, ST_CONNECTED);
	    // lcm_send_cmd_frame(CMD_NACK);
	    break;
	}
	break;
    case ST_IDLE:
    case ST_COMMAND:
	fsm_trans_cmd(fsm, old_state, CMD_NACK);
	break;
    }
    fsm_step(fsm);
}


struct _lcm_fsm_t {
    lcm_state_t state;
    lcm_state_t next_state;
    enum {
	ACTION_UNINITIALIZED,
	ACTION_NOOP,
	ACTION_CMD,
	ACTION_DATA
    } action_type;
    union {
	struct {
	    lcm_cmd_t cmd;
	} cmd_frame;
	struct {
	    lcm_cmd_t cmd;
	    const char *data;
	    unsigned int len;
	} data_frame;
    } action;
};


static
lcm_state_t fsm_get_state(lcm_fsm_t * fsm)
{
    return fsm->state;
}


static
void flush_shadow(void);


static
void fsm_step(lcm_fsm_t * fsm)
{
    debug("fsm: old_state=%s new_state=%s", state2str(fsm->state), state2str(fsm->next_state));
    switch (fsm->action_type) {
    case ACTION_UNINITIALIZED:
	error("Uninitialized LCM FSM action");
	abort();
	break;
    case ACTION_NOOP:
	break;
    case ACTION_CMD:
	raw_send_cmd_frame(fsm->action.cmd_frame.cmd);
	break;
    case ACTION_DATA:
	raw_send_data_frame(fsm->action.data_frame.cmd, fsm->action.data_frame.data, fsm->action.data_frame.len);
	break;
    }
    fsm->action_type = ACTION_UNINITIALIZED;
    switch (fsm->next_state) {
    case ST_IDLE:
    case ST_COMMAND:
	fsm->state = fsm->next_state;
	fsm->next_state = -1;
	return;
	break;
    case ST_CONNECTED:
	if (fsm->state != ST_CONNECTED) {
	    /* going from ST_IDLE or ST_COMMAND into ST_CONNECTED */
	    if (!global_reset_rx_flag) {
		try_reset();
#if 0
		int timer_res = timer_add(repeat_connect_to_display_callback, NULL, 50 /*ms */ , 1);
		debug("re-scheduled connect callback result: %d", timer_res);

		done by try_reset / fsm_init fsm->state = fsm->next_state;
		fsm->next_state = -1;
#endif
		return;
	    } else {
		/* properly connected for the first time */
		debug("%s: %s NOW CONNECTED!!!", Name, __FUNCTION__);

		fsm->state = fsm->next_state;
		fsm->next_state = -1;

		lcm_send_cmd(LCM_DISPLAY_ON);
		flush_shadow();
		lcm_send_cmd(LCM_BACKLIGHT_ON);
		return;
	    }
	} else {
	    debug("no state change in ST_CONNECTED");
	    fsm->state = fsm->next_state;
	    fsm->next_state = -1;
	    return;
	}
	error("we should never arrive here");
	abort();
	break;
    }
    error("LCM FSM: Illegal next_state");
    abort();
}


#if 0

#endif


static
void fsm_trans_noop(lcm_fsm_t * fsm, const lcm_state_t next_state)
{
    fsm->next_state = next_state;
    fsm->action_type = ACTION_NOOP;
}


static
void fsm_trans_cmd(lcm_fsm_t * fsm, const lcm_state_t next_state, const lcm_cmd_t cmd)
{
    fsm->next_state = next_state;
    fsm->action_type = ACTION_CMD;
    fsm->action.cmd_frame.cmd = cmd;
}


static
void fsm_trans_data(lcm_fsm_t * fsm,
		    const lcm_state_t next_state, const lcm_cmd_t cmd, const char *data, const unsigned int len)
{
    fsm->next_state = next_state;
    fsm->action_type = ACTION_DATA;
    fsm->action.data_frame.cmd = cmd;
    fsm->action.data_frame.data = data;
    fsm->action.data_frame.len = len;
}


static
void fsm_send(lcm_fsm_t * fsm, const lcm_cmd_t cmd)
{
    const lcm_state_t old_state = fsm_get_state(fsm);
    switch (old_state) {
    case ST_IDLE:
    case ST_COMMAND:
	debug("%s: %s, ignoring cmd 0x%02x=%s", __FUNCTION__, state2str(old_state), cmd, cmdstr(cmd));
	/* Silently ignore the command to send. */
	/* TODO: Would it be better to queue it and send it later? */
	break;
    case ST_CONNECTED:
	fsm_trans_cmd(fsm, ST_CONNECTED, cmd);
	fsm_step(fsm);
	break;
    }
}


static
void fsm_send_data(lcm_fsm_t * fsm, const lcm_cmd_t cmd, const void *data, const unsigned int len)
{
    const lcm_state_t old_state = fsm_get_state(fsm);
    switch (old_state) {
    case ST_IDLE:
    case ST_COMMAND:
	debug("%s: %s, ignoring data cmd 0x%02x=%s", __FUNCTION__, state2str(old_state), cmd, cmdstr(cmd));
	/* Silently ignore the command to send. */
	/* TODO: Would it be better to queue it and send it later? */
	break;
    case ST_CONNECTED:
	fsm_trans_data(fsm, ST_CONNECTED, cmd, data, len);
	fsm_step(fsm);
	break;
    }
}


static lcm_fsm_t lcm_fsm;


static
void fsm_init(void)
{
    lcm_fsm.state = ST_IDLE;
    lcm_fsm.next_state = -1;
    lcm_fsm.action_type = ACTION_UNINITIALIZED;
}



/* Send a command frame to the TCM board */
static
void raw_send_cmd_frame(lcm_cmd_t cmd)
{
    // lcm_receive_check();
    char cmd_buf[3];
    cmd_buf[0] = LCM_FRAME_MASK;
    cmd_buf[1] = cmd;
    cmd_buf[2] = LCM_FRAME_MASK;
    debug("%s sending cmd frame cmd=0x%02x=%s", __FUNCTION__, cmd, cmdstr(cmd));
    debug_data(" TX ", cmd_buf, 3);
    drv_generic_serial_write(cmd_buf, 3);
    usleep(100);
#if 0
    usleep(100000);
    switch (cmd) {
    case CMD_ACK:
	//case CMD_CONFIRM:
    case CMD_NACK:
	lcm_receive_check();
	break;
    default:
	if (1) {
	    int i;
	    for (i = 0; i < 20; i++) {
		usleep(100000);
		if (lcm_receive_check()) {
		    break;
		}
	    }
	}
	break;
    }
#endif
}


/* Send a data frame to the TCM board */
static
void raw_send_data_frame(lcm_cmd_t cmd, const char *data, const unsigned int len)
{
    unsigned int di;		/* data index */
    unsigned int fi;		/* frame index */
    static char frame[32];
    uint16_t crc = 0;

    frame[0] = LCM_FRAME_MASK;

    frame[1] = cmd;
    crc = CRC16(frame[1], crc);

    frame[2] = HI8(len);
    crc = CRC16(frame[2], crc);

    frame[3] = LO8(len);
    crc = CRC16(frame[3], crc);

#define APPEND(value)				       \
    do {					       \
	const unsigned char v = (value);	       \
	if ((v == LCM_FRAME_MASK) || (v == LCM_ESC)) { \
	    frame[fi++] = LCM_ESC;		       \
	}					       \
	frame[fi++] = v;			       \
	crc = CRC16(v, crc);			       \
    } while (0)

#define APPEND_NOCRC(value)			       \
    do {					       \
	const unsigned char v = (value);	       \
	if ((v == LCM_FRAME_MASK) || (v == LCM_ESC)) { \
	    frame[fi++] = LCM_ESC;		       \
	}					       \
	frame[fi++] = v;			       \
    } while (0)

    for (fi = 4, di = 0; di < len; di++) {
	APPEND(data[di]);
    }

    APPEND_NOCRC(HI8(crc));
    APPEND_NOCRC(LO8(crc));

    frame[fi++] = LCM_FRAME_MASK;

    debug_data(" TXD ", frame, fi);
    drv_generic_serial_write(frame, fi);

#undef APPEND

    usleep(500);
}


static
void lcm_send_cmd(lcm_cmd_t cmd)
{
    fsm_send(&lcm_fsm, cmd);
}


static
void lcm_event_callback(event_flags_t flags, void *data)
{
    lcm_fsm_t *fsm = (lcm_fsm_t *) data;
    debug("%s: flags=%d, data=%p", __FUNCTION__, flags, data);
    if (flags & EVENT_READ) {
	static uint8_t rxbuf[32];
	const int readlen = drv_generic_serial_poll((void *) rxbuf, sizeof(rxbuf));
	if (readlen <= 0) {
	    debug("%s Received no data", __FUNCTION__);
	} else {
	    debug("%s RECEIVED %d bytes", __FUNCTION__, readlen);
	    debug_data(" RX ", rxbuf, readlen);
	    fsm_handle_bytes(fsm, rxbuf, readlen);
	}
    }
}


static int drv_TeakLCM_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    const int fd = drv_generic_serial_open(section, Name, 0);
    if (fd < 0)
	return -1;

    return fd;
}


static int drv_TeakLCM_close(int fd)
{
    if (fd >= 0) {
	event_del(fd);
    }

    /* close whatever port you've opened */
    drv_generic_serial_close();

    return 0;
}


/* shadow buffer */
static
char *shadow;


static void debug_shadow(const char *prefix)
{
    debug_data_int(prefix, shadow, DCOLS * DROWS, 20);
}


static
void flush_shadow(void)
{
    debug("%s called", __FUNCTION__);
    debug_shadow(" shadow ");
    usleep(50000);
    fsm_send_data(&lcm_fsm, CMD_PRINT1, &shadow[DCOLS * 0], DCOLS);
    usleep(50000);
    fsm_send_data(&lcm_fsm, CMD_PRINT2, &shadow[DCOLS * 1], DCOLS);
    usleep(50000);
}


/* text mode displays only */
static
void drv_TeakLCM_clear(void)
{
    /* do whatever is necessary to clear the display */
    memset(shadow, ' ', DROWS * DCOLS);
    flush_shadow();
}


/* text mode displays only */
static void drv_TeakLCM_write(const int row, const int col, const char *data, int len)
{
    debug("%s row=%d col=%d len=%d data=\"%s\"", __FUNCTION__, row, col, len, data);

    memcpy(&shadow[DCOLS * row + col], data, len);

    debug_shadow(" shadow ");

    fsm_send_data(&lcm_fsm, (row == 0) ? CMD_PRINT1 : CMD_PRINT2, &shadow[DCOLS * row], DCOLS);
}


static
void try_reset(void)
{
    debug("%s called", __FUNCTION__);
    fsm_init();
    raw_send_cmd_frame(CMD_RESET);
}


#if 0
static
void repeat_connect_to_display_callback(void *data)
{
    static int already_called = 0;
    if (!already_called) {
	debug("%s(%p): called", __FUNCTION__, data);

	/* reset & initialize display */
	try_reset();
	already_called = 1;
    } else {
	debug("%s(%p): already called, ignoring", __FUNCTION__, data);
    }
}
#endif


static
int global_fd = -1;


static
void initial_connect_to_display_callback(void *data)
{
    debug("%s(%p): called", __FUNCTION__, data);

    debug("Calling event_add for fd=%d", global_fd);
    int ret = event_add(lcm_event_callback, &lcm_fsm, global_fd, 1, 0, 1);
    debug("event_add result: %d", ret);

    /* reset & initialize display */
    try_reset();
}


/* start text mode display */
static int drv_TeakLCM_start(const char *section)
{
    int rows = -1, cols = -1;
    char *s;

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
    shadow = malloc(DROWS * DCOLS);
    memset(shadow, ' ', DROWS * DCOLS);

    /* open communication with the display */
    global_fd = drv_TeakLCM_open(section);
    if (global_fd < 0) {
	return -1;
    }
    debug("%s: %s opened", Name, __FUNCTION__);

    /* read initial garbage data */
    static uint8_t rxbuf[32];
    const int readlen = drv_generic_serial_poll((void *) rxbuf, sizeof(rxbuf));
    if (readlen >= 0) {
	debug_data(" initial RX garbage ", rxbuf, readlen);
    }

    /* We need to do a delayed connect */
    int timer_res = timer_add(initial_connect_to_display_callback, NULL, 10 /*ms */ , 1);
    debug("timer_add for connect callback result: %d", timer_res);

    debug("%s: %s done", Name, __FUNCTION__);
    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/


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
int drv_TeakLCM_list(void)
{
    printf("TeakLCM driver");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_TeakLCM_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ret;

    info("%s: %s (quiet=%d)", Name, "$Rev$", quiet);

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 0;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = -1;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_TeakLCM_write;

    /* start display */
    if ((ret = drv_TeakLCM_start(section)) != 0)
	return ret;

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register plugins */

    return 0;
}


/* close driver & display */
/* use this function for a text display */
int drv_TeakLCM_quit(const int quiet)
{

    info("%s: shutting down. (quiet=%d)", Name, quiet);

    drv_generic_text_quit();

    /* clear display */
    drv_TeakLCM_clear();

    lcm_send_cmd(LCM_DISPLAY_OFF);
    // lcm_send_cmd_frame(LCM_BACKLIGHT_OFF);
    lcm_send_cmd(CMD_DISCONNECT);

    /* FIXME: consume final ack frame */
    usleep(100000);

    debug("closing connection");
    drv_TeakLCM_close(global_fd);

    return (0);
}


/* use this one for a text display */
DRIVER drv_TeakLCM = {
    .name = Name,
    .list = drv_TeakLCM_list,
    .init = drv_TeakLCM_init,
    .quit = drv_TeakLCM_quit,
};


/*
 * Local Variables:
 * c-basic-offset: 4
 * End:
 */
