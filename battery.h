/* $Id: battery.h,v 1.2 2001/03/02 11:04:08 reinelt Exp $
 *
 * APM and ACPI specific functions
 *
 * Copyright 2001 by Leo Tötsch (lt@toetsch.at)
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
 * $Log: battery.h,v $
 * Revision 1.2  2001/03/02 11:04:08  reinelt
 *
 * cosmetic cleanups (comment headers)
 *
 */

#ifndef _BATTERY_H_
#define _BATTERY_H_

int Battery ( int *perc, int *stat, double *dur);

#endif
