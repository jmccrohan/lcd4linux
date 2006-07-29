/* $Id: drv_generic_i2c.c,v 1.6 2006/07/29 21:04:43 lfcorreia Exp $
 *
 * generic driver helper for i2c displays
 *
 * Copyright (C) 2005 Luis Correia <lfcorreia@users.sf.net>
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
 * $Log: drv_generic_i2c.c,v $
 * Revision 1.6  2006/07/29 21:04:43  lfcorreia
 * Better error handling, add proper I2C SLAVE device detection (not 100% finished)
 *
 * Revision 1.5  2005/06/01 12:50:25  reinelt
 * ifdef'ed unused function to avoid compiler warning
 *
 * Revision 1.4  2005/05/31 20:42:55  lfcorreia
 * new file: lcd4linux_i2c.h
 * avoid the problems detecting the proper I2C kernel include files
 *
 * rearrange all the other autoconf stuff to remove I2C detection
 *
 * new method by Paul Kamphuis to write to the I2C device
 *
 * Revision 1.3  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.2  2005/04/05 05:12:57  reinelt
 * i2c patch from Paul (still does not work here :-(
 *
 * Revision 1.1  2005/03/28 19:39:23  reinelt
 * HD44780/I2C patch from Luis merged (still does not work for me)
 *
 *
 */

    /*
       DISCLAIMER!!!!

       The following code is WORK IN PROGRESS, since it basicly 'works for us...'

       (C) 2005 Paul Kamphuis & Luis Correia

       We have removed all of the delays from this code, as the I2C bus is slow enough...
       (maximum possible speed is 100KHz only)

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

#include "lcd4linux_i2c.h"
#include "debug.h"
#include "qprintf.h"
#include "cfg.h"
#include "udelay.h"
#include "drv_generic_i2c.h"


static char *Driver = "";
static char *Section = "";
static int i2c_device;


static void my_i2c_smbus_write_byte_data(const int device, const unsigned char val)
{
    struct i2c_smbus_ioctl_data args;
    union i2c_smbus_data data;
    args.read_write = I2C_SMBUS_WRITE;
    args.command = val;
    args.size = I2C_SMBUS_BYTE_DATA;
    data.byte = val;
    args.data = &data;
    if (ioctl(device, I2C_SMBUS, &args) < 0) {
	info("I2C: device %s IOCTL failed !\n", device);
    }
}

/* unused, ifdef'ed away to avoid compiler warning */
#if 0
static void my_i2c_smbus_read_byte_data(const int device, const unsigned char data)
{
    struct i2c_smbus_ioctl_data args;
    args.read_write = I2C_SMBUS_READ;
    args.command = data;
    args.size = I2C_SMBUS_BYTE;
    args.data = 0;
    ioctl(device, I2C_SMBUS, &args);
}
#endif


int drv_generic_i2c_open(const char *section, const char *driver)
{
    int dev;
    char *bus, *device;
    udelay_init();
    Section = (char *) section;
    Driver = (char *) driver;
    bus = cfg_get(Section, "Port", NULL);
    device = cfg_get(Section, "Device", NULL);
    dev = atoi(device);
    info("%s: initializing I2C bus %s", Driver, bus);
    if ((i2c_device = open(bus, O_WRONLY)) < 0) {
	error("%s: I2C bus %s open failed !\n", Driver, bus);
	goto exit_error;
    }
    info("%s: selecting slave device 0x%x", Driver, dev);
    if (ioctl(i2c_device, I2C_SLAVE, dev) < 0) {
	error("%s: error selecting slave device 0x%x\n", Driver, dev);
	goto exit_error;
    }

    info("%s: initializing I2C slave device 0x%x", Driver, dev);
    if (i2c_smbus_write_quick(i2c_device, I2C_SMBUS_WRITE) < 0 ){
	error("%s: error initializing device 0x%x\n", Driver, dev);
	close(i2c_device);
		}

    return 0;

  exit_error:
    free(bus);
    free(device);
		close(i2c_device);
		return -1;
}


unsigned char drv_generic_i2c_wire(const char *name, const char *deflt)
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
	error("%s: unknown signal <%s> for wire <%s>", Driver, s, name);
	error("%s: should be DB0..7 or GND", Driver);
	return 0xff;
    }
    free(s);
    if (w == 0) {
	info("%s: wiring: [DISPLAY:%s]<==>[i2c:GND]", Driver, name);
    } else {
	info("%s: wiring: [DISPLAY:%s]<==>[i2c:DB%d]", Driver, name, w);
    }
    w = 1 << w;
    return w;
}


void drv_generic_i2c_data(const unsigned char data)
{
    my_i2c_smbus_write_byte_data(i2c_device, data);
}


void drv_generic_i2c_command(const unsigned char command, const unsigned char *data, const unsigned char length)
{
    i2c_smbus_write_block_data(i2c_device, command, length, (unsigned char *) data);
}
