/* $Id: seti.h,v 1.1 2001/02/18 21:15:15 reinelt Exp $
 *
 * seti@home specific functions
 *
 * Copyright 2001 by Axel Ehnert <Axel@Ehnert.net>
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
 * $Log: seti.h,v $
 * Revision 1.1  2001/02/18 21:15:15  reinelt
 *
 * added setiathome client
 *
 */

#ifndef _SETI_H_
#define _SETI_H_

#define DEFSETIPOLLEXT 10

int Seti (int *perc, int *cput);

#endif
