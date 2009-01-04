/* $Id$
 * $URL$
 *
 * plugin for DVB status
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
 * exported functions:
 *
 * int plugin_init_dvb (void)
 *  adds dvb() function
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <asm/types.h>

#ifdef HAVE_LINUX_DVB_FRONTEND_H
#include <linux/dvb/frontend.h>
#else
#warning linux/dvb/frontend.h not found: using hardcoded ioctl definitions
#define FE_READ_BER                _IOR('o', 70, __u32)
#define FE_READ_SIGNAL_STRENGTH    _IOR('o', 71, __u16)
#define FE_READ_SNR                _IOR('o', 72, __u16)
#define FE_READ_UNCORRECTED_BLOCKS _IOR('o', 73, __u32)
#endif

#include "debug.h"
#include "plugin.h"
#include "hash.h"

static char *frontend = "/dev/dvb/adapter0/frontend0";

static HASH DVB;

static int get_dvb_stats(void)
{
    static int ioc_disable = 0;

    int age;
    int fd;
    unsigned short snr, sig;
    unsigned long ber, ucb;
    char val[16];

    /* reread every 1000 msec only */
    age = hash_age(&DVB, NULL);
    if (age > 0 && age <= 1000)
	return 0;

    /* open frontend */
    fd = open(frontend, O_RDONLY);
    if (fd == -1) {
	error("open(%s) failed: %s", frontend, strerror(errno));
	return -1;
    }

    sig = 0;
    if ((ioc_disable & 0x01) == 0 && ioctl(fd, FE_READ_SIGNAL_STRENGTH, &sig) != 0) {
	error("ioctl(FE_READ_SIGNAL_STRENGTH) failed: %s", strerror(errno));
	if (errno == ENOSYS) {
	    ioc_disable |= 0x01;
	    error("ioctl(FE_READ_SIGNAL_STRENGTH) disabled.");
	}
    }

    snr = 0;
    if ((ioc_disable & 0x02) == 0 && ioctl(fd, FE_READ_SNR, &snr) != 0) {
	error("ioctl(FE_READ_SNR) failed: %s", strerror(errno));
	if (errno == ENOSYS) {
	    ioc_disable |= 0x02;
	    error("ioctl(FE_READ_SNR) disabled.");
	}
    }

    ber = 0;
    if ((ioc_disable & 0x04) == 0 && ioctl(fd, FE_READ_BER, &ber) != 0) {
	error("ioctl(FE_READ_BER) failed: %s", strerror(errno));
	if (errno == ENOSYS) {
	    ioc_disable |= 0x04;
	    error("ioctl(FE_READ_BER) disabled.");
	}
    }

    ucb = 0;
    if ((ioc_disable & 0x08) == 0 && ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &ucb) != 0) {
	error("ioctl(FE_READ_UNCORRECTED_BLOCKS) failed: %s", strerror(errno));
	if (errno == ENOSYS) {
	    ioc_disable |= 0x08;
	    error("ioctl(FE_READ_UNCORRECTED_BLOCKS) disabled.");
	}
    }

    close(fd);

    snprintf(val, sizeof(val), "%f", sig / 65535.0);
    hash_put(&DVB, "signal_strength", val);

    snprintf(val, sizeof(val), "%f", snr / 65535.0);
    hash_put(&DVB, "snr", val);

    snprintf(val, sizeof(val), "%lu", ber);
    hash_put(&DVB, "ber", val);

    snprintf(val, sizeof(val), "%lu", ucb);
    hash_put(&DVB, "uncorrected_blocks", val);

    return 0;
}


static void my_dvb(RESULT * result, RESULT * arg1)
{
    char *val;

    if (get_dvb_stats() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    val = hash_get(&DVB, R2S(arg1), NULL);
    if (val == NULL)
	val = "";

    SetResult(&result, R_STRING, val);
}


int plugin_init_dvb(void)
{
    hash_create(&DVB);
    AddFunction("dvb", 1, my_dvb);
    return 0;
}


void plugin_exit_dvb(void)
{
    hash_destroy(&DVB);
}
