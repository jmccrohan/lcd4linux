/* $Id: dvb.h,v 1.3 2003/10/05 17:58:50 reinelt Exp $
 *
 * DVB specific functions
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: dvb.h,v $
 * Revision 1.3  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.2  2003/08/08 23:08:38  reinelt
 * CVS Id and Log missing from dvb.{c,h}
 *
 */

#ifndef _DVB_H_
#define _DVB_H_

int DVB (double *strength, double *snr);

#endif
