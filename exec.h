/* $Id: exec.h,v 1.3 2003/10/05 17:58:50 reinelt Exp $
 *
 * exec ('x*') functions
 *
 * Copyright 2001 Leopold Tötsch <lt@toetsch.at>
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
 * $Log: exec.h,v $
 * Revision 1.3  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.2  2001/03/08 15:25:38  ltoetsch
 * improved exec
 *
 * Revision 1.1  2001/03/07 18:10:21  ltoetsch
 * added e(x)ec commands
 *
 *
 */

#ifndef _EXEC_H
#define _EXEC_H_

#define EXECS 9
#define EXEC_TXT_LEN 256

#ifdef IN_EXEC
  #define EXTERN extern
#else
  #define EXTERN
#endif

EXTERN struct { char s[EXEC_TXT_LEN]; double val; } exec[EXECS+1];

int Exec (int index, char txt[EXEC_TXT_LEN], double *val);


#endif
