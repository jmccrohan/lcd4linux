/* $Id$
 * $URL$
 *
 * timer widget handling
 *
 * Copyright (C) 2006 Michael Reinelt <michael@reinelt.co.at>
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
 */


#ifndef _WIDGET_TIMER_H_
#define _WIDGET_TIMER_H_

#include "property.h"

typedef struct WIDGET_TIMER {
    PROPERTY expression;	/* main timer expression */
    PROPERTY update;		/* update interval (msec) */
    PROPERTY active;		/* timer active? */
} WIDGET_TIMER;

extern WIDGET_CLASS Widget_Timer;

int widget_timer_register(void);

#endif
