/* $Id: processor.h,v 1.1 2000/03/22 07:33:50 reinelt Exp $
 *
 * main data processing
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: processor.h,v $
 * Revision 1.1  2000/03/22 07:33:50  reinelt
 *
 * FAQ added
 * new modules 'processor.c' contains all data processing
 *
 */

#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

void process_init (void);
void process (int smooth);

#endif
