/* lcd.h
 *
 * $Id: drv_LCDLinux.h,v 1.2 2005/04/30 06:02:09 reinelt Exp $
 *
 * LCD driver for HD44780 compatible displays connected to the parallel port.
 * 
 * External interface header file.
 * 
 * Copyright (C) 2004, 2005  Mattia Jona-Lasinio (mjona@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef LCD_H
#define LCD_H

#define LCD_LINUX_VERSION	"0.8.9-CVS"	/* Version number */

#define LCD_MAJOR		120	/* Major number for this device
					 * Set this to 0 for dynamic allocation
					 */

#include <linux/types.h>

struct lcd_driver {
	/* Hardware */
	unsigned short io;		/* Parport base address */
	unsigned short flags;		/* Flags (see Documentation) */

	/* Display geometry */
	unsigned short num_cntr;	/* Number of available controllers */
	unsigned short cntr_rows;	/* Rows per controller */
	unsigned short disp_cols;	/* Columns */
	unsigned short frames;		/* Framebuffer frames */

	unsigned short tabstop;		/* Length of tab character */
};

/* IOCTLs */
#include <asm/ioctl.h>
#define IOCTL_SET_PARAM		_IOW(LCD_MAJOR, 0, struct lcd_driver *)
#define IOCTL_GET_PARAM		_IOR(LCD_MAJOR, 1, struct lcd_driver *)

#define LCD_PROC_ON	0x0001		/* Enable the /proc filesystem support */
#define LCD_ETTY_ON	0x0002		/* Enable the tty support */
#define LCD_CONSOLE	0x0004		/* Enable the console support */
#define LCD_4BITS_BUS	0x0008		/* Set the bus length to 4 bits */
#define LCD_5X10_FONT	0x0010		/* Use 5x10 dots fonts */



#ifdef __KERNEL__ /* The rest is for kernel only */

#include <linux/version.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#ifndef LINUX_VERSION_CODE
#error - LINUX_VERSION_CODE undefined.
#endif

/* External interface */
int lcd_init(struct lcd_driver *);
void lcd_exit(void);
int lcd_ioctl(unsigned int, void *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
ssize_t lcd_write(const char *, size_t, unsigned int);
ssize_t lcd_read(char *, size_t, unsigned int);
#else
int lcd_write(const char *, int, unsigned int);
int lcd_read(char *, int, unsigned int);
#endif

#endif /* __KERNEL__ */

#endif /* External interface included */
