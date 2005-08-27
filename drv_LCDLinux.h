/* hd44780.h
 *
 * $Id: drv_LCDLinux.h,v 1.6 2005/08/27 07:02:25 reinelt Exp $
 *
 * LCD-Linux:
 * Driver for HD44780 compatible displays connected to the parallel port.
 * 
 * HD44780 header file.
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

#ifndef HD44780_H
#define HD44780_H

#include <linux/lcd-linux.h>

#define HD44780_VERSION		"0.9.0"		/* Version number */
#define HD44780_STRING		"hd44780"


/* IOCTLs */
#include <asm/ioctl.h>
#define IOCTL_RAW_CMD	_IOW(LCD_MAJOR, 0, unsigned char *)

/* flags */
#define HD44780_CHECK_BF	0x0001	/* Do busy flag checking */
#define HD44780_4BITS_BUS	0x0002	/* Set the bus length to 4 bits */
#define HD44780_5X10_FONT	0x0004	/* Use 5x10 dots fonts */



/*** HD44780 Command Set ***/

/* Clear Display*/
#define CLR_DISP	0x01	/* Clear entire display; cursor at row 0, column 0 */

/* Return Home */
#define	RET_HOME	0x02	/* Cursor at row 0, column 0; display content doesn't change */

/* Entry Mode Set */
#define DISP_SHIFT_ON	0x05	/* Shift display, not cursor after data write */
#define DISP_SHIFT_OFF	0x04	/* Shift cursor, not display after data write */
#define CURS_INC	0x06	/* Shift on the right after data read/write */
#define CURS_DEC	0x04	/* Shift on the left after data read/write */

/* Display on/off Control */
#define BLINK_ON	0x09	/* Cursor blinking on */
#define BLINK_OFF	0x08	/* Cursor blinking off */
#define CURS_ON		0x0a	/* Display Cursor */
#define CURS_OFF	0x08	/* Hide Cursor */
#define DISP_ON		0x0c	/* Turn on display updating */
#define DISP_OFF	0x08	/* Freeze display content */

/* Cursor or Display Shift */
#define SHIFT_RIGHT	0x14	/* Shift on the right */
#define SHIFT_LEFT	0x10	/* Shift on the left */
#define SHIFT_DISP	0x18	/* Shift display */
#define SHIFT_CURS	0x10	/* Shift cursor */

/* Function Set */
#define FONT_5X10	0x24	/* Select 5x10 dots font */
#define FONT_5X8	0x20	/* Select 5x8 dots font */
#define DISP_2_LINES	0x28	/* Select 2 lines display (only 5x8 font allowed) */
#define DISP_1_LINE	0x20	/* Select 1 line display */
#define BUS_8_BITS	0x30	/* Set 8 data bits */
#define BUS_4_BITS	0x20	/* Set 4 data bits */

/* Set CGRAM Address */
#define CGRAM_IO	0x40	/* Access the CGRAM */

/* Set DDRAM Address */
#define DDRAM_IO	0x80	/* Access the DDRAM */

#endif /* HD44780 included */
