/* $Id: rgb.c,v 1.1 2006/01/30 05:47:38 reinelt Exp $
 *
 * generic color handling
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: rgb.c,v $
 * Revision 1.1  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 */


#include <stdlib.h>
#include <string.h>

#include "rgb.h"


int color2RGBA(const char *color, RGBA * C)
{
    char *e;
    unsigned long l;

    if (color == NULL || *color == '\0') {
	return -1;
    }

    l = strtoul(color, &e, 16);
    if (e != NULL && *e != '\0') {
	return -1;
    }

    if (strlen(color) == 8) {
	/* RGBA */
	C->R = (l >> 24) & 0xff;
	C->G = (l >> 16) & 0xff;
	C->B = (l >> 8) & 0xff;
	C->A = (l >> 0) & 0xff;
    } else {
	/* RGB */
	C->R = (l >> 16) & 0xff;
	C->G = (l >> 8) & 0xff;
	C->B = l & 0xff;
	C->A = 0xff;
    }
    return 0;
}
