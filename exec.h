/* $Id: exec.h,v 1.1 2001/03/07 18:10:21 ltoetsch Exp $
 *
 * exec ('x*') functions
 *
 * Copyright 2001 by Leopold Tötsch (lt@toetsch.at)
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
 * $Log: exec.h,v $
 * Revision 1.1  2001/03/07 18:10:21  ltoetsch
 * added e(x)ec commands
 *
 *
 */

#ifndef _EXEC_H
#define _EXEC_H_

#define EXECS 9
#define EXEC_WAIT 2
#define EXEC_TXT_LEN 256

int Exec (int index, char txt[EXEC_TXT_LEN]);

#endif
