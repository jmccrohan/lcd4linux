/* $Id$
 * $URL$
 *
 * GPO widget handling
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
 * $Log: widget_gpo.h,v $
 * Revision 1.1  2005/12/18 16:18:36  reinelt
 * GPO's added again
 *
 */


#ifndef _WIDGET_GPO_H_
#define _WIDGET_GPO_H_

typedef struct WIDGET_GPO {
    char *expression;		/* expression that delivers the value */
    void *tree;			/* pre-compiled expression that delivers the value */
    int update;			/* update interval (msec) */
    int num;			/* GPO number */
    int val;			/* GPO value */
} WIDGET_GPO;


extern WIDGET_CLASS Widget_GPO;

#endif
