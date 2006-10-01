/* $Id: widget_timer.h,v 1.3 2006/10/01 11:54:38 reinelt Exp $
 *
 * timer widget handling
 *
 * Copyright (C) 2006 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: widget_timer.h,v $
 * Revision 1.3  2006/10/01 11:54:38  reinelt
 * timer widget uses properties
 *
 * Revision 1.2  2006/02/25 13:36:33  geronet
 * updated indent.sh, applied coding style
 *
 * Revision 1.1  2006/01/23 06:17:18  reinelt
 * timer widget added
 *
 */


#ifndef _WIDGET_TIMER_H_
#define _WIDGET_TIMER_H_


#include "property.h"


typedef struct WIDGET_TIMER {
    PROPERTY expression; /* main timer expression */
    PROPERTY update; /* update interval (msec) */
    PROPERTY active; /* timer active? */
} WIDGET_TIMER;

extern WIDGET_CLASS Widget_Timer;

int widget_timer_register(void);

#endif
