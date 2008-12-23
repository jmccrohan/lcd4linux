/* $Id$
 * $URL$
 *
 * generic driver helper for serial and parport access
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <michael@reinelt.co.at>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#ifdef HAVE_SYS_IO_H
#include <sys/io.h>
#define WITH_OUTB
#else
#ifdef HAVE_ASM_IO_H
#include <asm/io.h>
#define WITH_OUTB
#endif
#endif

#if defined (HAVE_LINUX_PARPORT_H) && defined (HAVE_LINUX_PPDEV_H)
#define WITH_PPDEV
#include <linux/parport.h>
#include <linux/ppdev.h>
#else
#define PARPORT_CONTROL_STROBE    0x1
#define PARPORT_CONTROL_AUTOFD    0x2
#define PARPORT_CONTROL_INIT      0x4
#define PARPORT_CONTROL_SELECT    0x8
#define PARPORT_STATUS_ERROR      0x8
#define PARPORT_STATUS_SELECT     0x10
#define PARPORT_STATUS_PAPEROUT   0x20
#define PARPORT_STATUS_ACK        0x40
#define PARPORT_STATUS_BUSY       0x80
#endif

/* these signals are inverted by hardware on the parallel port */
#define PARPORT_CONTROL_INVERTED (PARPORT_CONTROL_STROBE | PARPORT_CONTROL_SELECT | PARPORT_CONTROL_AUTOFD)

#if !defined(WITH_OUTB) && !defined(WITH_PPDEV)
#error neither outb() nor ppdev() possible
#error cannot compile parallel port driver
#endif

#include "debug.h"
#include "qprintf.h"
#include "cfg.h"
#include "udelay.h"
#include "drv_generic_parport.h"


static char *Driver = "";
static char *Section = "";
static unsigned short Port = 0;
static char *PPdev = NULL;

/* Any bits set here will have their logic inverted at the parallel port */
static unsigned char inverted_control_bits = 0;

/* initial value taken from linux/parport_pc.c */
static unsigned char ctr = 0xc;

#ifdef WITH_PPDEV
static int PPfd = -1;
#endif


int drv_generic_parport_open(const char *section, const char *driver)
{
    char *s, *e;

    Section = (char *) section;
    Driver = (char *) driver;

    udelay_init();

#ifndef WITH_PPDEV
    error("The files include/linux/parport.h and/or include/linux/ppdev.h");
    error("were missing at compile time. Even if your system supports");
    error("ppdev, it will not be used.");
    error("You *really* should install these files and recompile LCD4linux!");
#endif

    s = cfg_get(Section, "Port", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Port' entry from %s", Driver, Section, cfg_source());
	return -1;
    }

    PPdev = NULL;
    if ((Port = strtol(s, &e, 0)) == 0 || *e != '\0') {
#ifdef WITH_PPDEV
	Port = 0;
	PPdev = s;
#else
	error("%s: bad %s.Port '%s' from %s", Driver, Section, s, cfg_source());
	free(s);
	return -1;
#endif
    }
#ifdef WITH_PPDEV

    if (PPdev) {
	info("%s: using ppdev %s", Driver, PPdev);
	PPfd = open(PPdev, O_RDWR);
	if (PPfd == -1) {
	    error("%s: open(%s) failed: %s", Driver, PPdev, strerror(errno));
	    return -1;
	}
#if 0
	/* PPEXCL fails if someone else uses the port (e.g. lp.ko) */
	if (ioctl(PPfd, PPEXCL)) {
	    info("%s: ioctl(%s, PPEXCL) failed: %s", PPdev, Driver, strerror(errno));
	    info("%s: could not get exclusive access to %s.", Driver, PPdev);
	} else {
	    info("%s: got exclusive access to %s.", Driver, PPdev);
	}
#endif

	if (ioctl(PPfd, PPCLAIM)) {
	    error("%s: ioctl(%s, PPCLAIM) failed: %d %s", Driver, PPdev, errno, strerror(errno));
	    return -1;
	}
    } else
#endif

    {
	error("using raw port 0x%x (deprecated!)", Port);
	error("You *really* should change your setup and use ppdev!");
	if ((Port + 3) <= 0x3ff) {
	    if (ioperm(Port, 3, 1) != 0) {
		error("%s: ioperm(0x%x) failed: %s", Driver, Port, strerror(errno));
		return -1;
	    }
	} else {
	    if (iopl(3) != 0) {
		error("%s: iopl(1) failed: %s", Driver, strerror(errno));
		return -1;
	    }
	}
    }
    return 0;
}


int drv_generic_parport_close(void)
{
#ifdef WITH_PPDEV
    if (PPdev) {
	debug("closing ppdev %s", PPdev);
	if (ioctl(PPfd, PPRELEASE)) {
	    error("%s: ioctl(%s, PPRELEASE) failed: %s", Driver, PPdev, strerror(errno));
	}
	if (close(PPfd) == -1) {
	    error("%s: close(%s) failed: %s", Driver, PPdev, strerror(errno));
	    return -1;
	}
	free(PPdev);
    } else
#endif
    {
	debug("closing raw port 0x%x", Port);
	if ((Port + 3) <= 0x3ff) {
	    if (ioperm(Port, 3, 0) != 0) {
		error("%s: ioperm(0x%x) failed: %s", Driver, Port, strerror(errno));
		return -1;
	    }
	} else {
	    if (iopl(0) != 0) {
		error("%s: iopl(0) failed: %s", Driver, strerror(errno));
		return -1;
	    }
	}
    }

    return 0;
}


static unsigned char drv_generic_parport_signal_ctrl(const char *name, const char *signal)
{
    unsigned char wire;
    int invert = 0;

    /* Prefixing a signal name with '-' inverts the logic */
    if (signal[0] == '-') {
	invert = 1;
	signal++;
    }

    if (strcasecmp(signal, "STROBE") == 0) {
	wire = PARPORT_CONTROL_STROBE;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:STROBE (Pin  1)%s", Driver, name, invert ? " [inverted]" : "");
    } else if (strcasecmp(signal, "AUTOFD") == 0) {
	wire = PARPORT_CONTROL_AUTOFD;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:AUTOFD (Pin 14)%s", Driver, name, invert ? " [inverted]" : "");
    } else if (strcasecmp(signal, "INIT") == 0) {
	wire = PARPORT_CONTROL_INIT;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:INIT   (Pin 16)%s", Driver, name, invert ? " [inverted]" : "");
    } else if (strcasecmp(signal, "SLCTIN") == 0) {
	wire = PARPORT_CONTROL_SELECT;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:SLCTIN (Pin 17)%s", Driver, name, invert ? " [inverted]" : "");
    } else if (strcasecmp(signal, "SELECT") == 0) {
	wire = PARPORT_CONTROL_SELECT;
	error("%s: SELECT is deprecated. Please use SLCTIN instead!", Driver);
	info("%s: wiring: DISPLAY:%-9s - PARPORT:SLCTIN (Pin 17)%s", Driver, name, invert ? " [inverted]" : "");
    } else if (strcasecmp(signal, "GND") == 0) {
	wire = 0;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:GND", Driver, name);
    } else {
	error("%s: unknown signal <%s> for control line <%s>", Driver, signal, name);
	error("%s: should be STROBE, AUTOFD, INIT, SLCTIN or GND", Driver);
	return 0xff;
    }

    if (invert) {
	inverted_control_bits |= wire;
    }

    return wire;
}


unsigned char drv_generic_parport_wire_ctrl(const char *name, const char *deflt)
{
    unsigned char wire;
    char key[256];
    char *val;

    qprintf(key, sizeof(key), "Wire.%s", name);
    val = cfg_get(Section, key, deflt);

    wire = drv_generic_parport_signal_ctrl(name, val);

    free(val);

    return wire;
}


unsigned char drv_generic_parport_hardwire_ctrl(const char *name, const char *signal)
{
    unsigned char wire;
    char key[256];
    char *val;

    qprintf(key, sizeof(key), "Wire.%s", name);
    val = cfg_get(Section, key, "");

    /* maybe warn the user */
    if (*val != '\0' && strcasecmp(signal, val) != 0) {
	error("%s: ignoring configured signal <%s> for control line <%s>", Driver, val, name);
    }
    free(val);

    wire = drv_generic_parport_signal_ctrl(name, signal);

    return wire;
}


static unsigned char drv_generic_parport_signal_status(const char *name, const char *signal)
{
    unsigned char wire;

    if (strcasecmp(signal, "ERROR") == 0) {
	wire = PARPORT_STATUS_ERROR;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:ERROR    (Pin 15)", Driver, name);
    } else if (strcasecmp(signal, "SELECT") == 0) {
	wire = PARPORT_STATUS_SELECT;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:SELECT   (Pin 13)", Driver, name);
    } else if (strcasecmp(signal, "PAPEROUT") == 0) {
	wire = PARPORT_STATUS_PAPEROUT;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:PAPEROUT (Pin 12)", Driver, name);
    } else if (strcasecmp(signal, "ACK") == 0) {
	wire = PARPORT_STATUS_ACK;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:ACK      (Pin 10)", Driver, name);
    } else if (strcasecmp(signal, "BUSY") == 0) {
	wire = PARPORT_STATUS_BUSY;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:BUSY     (Pin 11)", Driver, name);
    } else if (strcasecmp(signal, "GND") == 0) {
	wire = 0;
	info("%s: wiring: DISPLAY:%-9s - PARPORT:GND", Driver, name);
    } else {
	error("%s: unknown signal <%s> for status line <%s>", Driver, signal, name);
	error("%s: should be ERROR, SELECT, PAPEROUT, ACK, BUSY or GND", Driver);
	return 0xff;
    }

    return wire;
}


unsigned char drv_generic_parport_wire_status(const char *name, const char *deflt)
{
    unsigned char wire;
    char key[256];
    char *val;

    qprintf(key, sizeof(key), "Wire.%s", name);
    val = cfg_get(Section, key, deflt);

    wire = drv_generic_parport_signal_status(name, val);

    free(val);

    return wire;
}


unsigned char drv_generic_parport_wire_data(const char *name, const char *deflt)
{
    unsigned char w;
    char wire[256];
    char *s;

    qprintf(wire, sizeof(wire), "Wire.%s", name);
    s = cfg_get(Section, wire, deflt);
    if (strlen(s) == 3 && strncasecmp(s, "DB", 2) == 0 && s[2] >= '0' && s[2] <= '7') {
	w = s[2] - '0';
    } else if (strcasecmp(s, "GND") == 0) {
	w = 0;
    } else {
	error("%s: unknown signal <%s> for data line <%s>", Driver, s, name);
	error("%s: should be DB0..7 or GND", Driver);
	return 0xff;
    }
    free(s);
    if (w == 0) {
	info("%s: wiring: DISPLAY:%-9s - PARPORT:GND", Driver, name);
    } else {
	info("%s: wiring: DISPLAY:%-9s - PARPORT:DB%d (Pin %2d)", Driver, name, w, w + 2);
    }

    w = 1 << w;

    return w;
}


void drv_generic_parport_direction(const int direction)
{
#ifdef WITH_PPDEV
    if (PPdev) {
	ioctl(PPfd, PPDATADIR, &direction);
    } else
#endif
    {
	/* code stolen from linux/parport_pc.h */
	ctr = (ctr & ~0x20) ^ (direction ? 0x20 : 0x00);
	outb(ctr, Port + 2);
    }
}


unsigned char drv_generic_parport_status(void)
{
    unsigned char mask =
	PARPORT_STATUS_ERROR | PARPORT_STATUS_SELECT | PARPORT_STATUS_PAPEROUT | PARPORT_STATUS_ACK |
	PARPORT_STATUS_BUSY;

    unsigned char data;

#ifdef WITH_PPDEV
    if (PPdev) {
	ioctl(PPfd, PPRSTATUS, &data);
    } else
#endif
    {
	data = inb(Port + 1);
    }

    /* clear unused bits */
    data &= mask;

    return data;
}


void drv_generic_parport_control(const unsigned char mask, const unsigned char value)
{
    unsigned char val;

    /* any signal affected? */
    /* Note: this may happen in case a signal is hardwired to GND */
    if (mask == 0)
	return;

    /* Strobe, Select and AutoFeed are inverted! */
    val = mask & (value ^ PARPORT_CONTROL_INVERTED ^ inverted_control_bits);

#ifdef WITH_PPDEV
    if (PPdev) {
	struct ppdev_frob_struct frob;
	frob.mask = mask;
	frob.val = val;
	ioctl(PPfd, PPFCONTROL, &frob);
    } else
#endif
    {
	/* code stolen from linux/parport_pc.h */
	ctr = (ctr & ~mask) ^ val;
	outb(ctr, Port + 2);
    }
}


void drv_generic_parport_toggle(const unsigned char bits, const int level, const unsigned long delay)
{
    unsigned char value1, value2;

    /* any signal affected? */
    /* Note: this may happen in case a signal is hardwired to GND */
    if (bits == 0)
	return;

    /* prepare value */
    value1 = level ? bits : 0;
    value2 = level ? 0 : bits;

    /* Strobe, Select and AutoFeed are inverted! */
    value1 = bits & (value1 ^ PARPORT_CONTROL_INVERTED ^ inverted_control_bits);
    value2 = bits & (value2 ^ PARPORT_CONTROL_INVERTED ^ inverted_control_bits);

#ifdef WITH_PPDEV
    if (PPdev) {
	struct ppdev_frob_struct frob;
	frob.mask = bits;

	/* rise */
	frob.val = value1;
	ioctl(PPfd, PPFCONTROL, &frob);

	/* pulse width */
	ndelay(delay);

	/* lower */
	frob.val = value2;
	ioctl(PPfd, PPFCONTROL, &frob);

    } else
#endif
    {
	/* rise */
	ctr = (ctr & ~bits) ^ value1;
	outb(ctr, Port + 2);

	/* pulse width */
	ndelay(delay);

	/* lower */
	ctr = (ctr & ~bits) ^ value2;
	outb(ctr, Port + 2);
    }
}


void drv_generic_parport_data(const unsigned char data)
{
#ifdef WITH_PPDEV
    if (PPdev) {
	ioctl(PPfd, PPWDATA, &data);
    } else
#endif
    {
	outb(data, Port);
    }
}

unsigned char drv_generic_parport_read(void)
{
    unsigned char data;

#ifdef WITH_PPDEV
    if (PPdev) {
	ioctl(PPfd, PPRDATA, &data);
    } else
#endif
    {
	data = inb(Port);
    }
    return data;
}


void drv_generic_parport_debug(void)
{
    unsigned char control;

#ifdef WITH_PPDEV
    if (PPdev) {
	ioctl(PPfd, PPRCONTROL, &control);
    } else
#endif
    {
	control = ctr;
    }

    debug("%cSTROBE %cAUTOFD %cINIT %cSLCTIN",
	  control & PARPORT_CONTROL_STROBE ? '-' : '+',
	  control & PARPORT_CONTROL_AUTOFD ? '-' : '+', control & PARPORT_CONTROL_INIT ? '+' : '-',
	  control & PARPORT_CONTROL_SELECT ? '-' : '+');

}
