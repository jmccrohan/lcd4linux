/* $Id: drv_SamsungSPF 975 2009-01-18 11:16:20Z michael $
   * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/drv_SamsungSPF.c $
   *
   * sample lcd4linux driver
   *
   * Copyright (C) 2012 Sascha Plazar <sascha@plazar.de>
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

// Drivername for verbose output
static char Name[] = "SamsungSPF";

#define USB_HDR_LEN 12

struct SPFdev {
    const char type[64];
    const int vendorID;
    struct {
	const int storageMode;
	const int monitorMode;
    } productID;
    const unsigned int xRes;
    const unsigned int yRes;
};

static struct SPFdev spfDevices[] = {
    {
     .type = "SPF-75H",
     .vendorID = 0x04e8,
     .productID = {0x200e, 0x200f},
     .xRes = 800,
     .yRes = 480,
     },
    {
     .type = "SPF-85H",
     .vendorID = 0x04e8,
     .productID = {0x2012, 0x2013},
     .xRes = 800,
     .yRes = 600,
     },
    {
     .type = "SPF-107H",
     .vendorID = 0x04e8,
     .productID = {0x2027, 0x2028},
     .xRes = 1024,
     .yRes = 600,
     },
};

static int numFrames = sizeof(spfDevices) / sizeof(spfDevices[0]);

struct usb_device *myDev;
usb_dev_handle *myDevHandle;
struct SPFdev *myFrame;

typedef struct {
    unsigned char R, G, B;
} RGB;

static struct {
    RGB *buf;
    int dirty;
    int fbsize;
} image;

static struct {
    unsigned char *buf;
    long int size;
} jpegImage;
