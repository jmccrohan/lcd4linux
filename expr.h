/* $Id: expr.h,v 1.1 2004/01/05 11:57:38 reinelt Exp $
 *
 * expr ('y*') functions
 * This is only a workaround to make the Evaluator usable until
 * it's fully integrated into lcd4linux.
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: expr.h,v $
 * Revision 1.1  2004/01/05 11:57:38  reinelt
 * added %y tokens to make the Evaluator useable
 *
 */

#ifndef _EXPR_H
#define _EXPR_H_

#define EXPRS 9
#define EXPR_TXT_LEN 256

#ifdef IN_EXPR
  #define EXTERN extern
#else
  #define EXTERN
#endif

EXTERN struct { char s[EXPR_TXT_LEN]; double val; } expr[EXPRS+1];

int Expr (int index, char txt[EXPR_TXT_LEN], double *val);


#endif
