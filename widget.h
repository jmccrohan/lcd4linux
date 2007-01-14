/* $Id$
 * $URL$
 *
 * generic widget handling
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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


#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "rgb.h"


struct WIDGET;			/* forward declaration */


typedef struct WIDGET_CLASS {
    char *name;
    int type;
    int (*init) (struct WIDGET * Self);
    int (*draw) (struct WIDGET * Self);
    int (*find) (struct WIDGET * Self, void *needle);
    int (*quit) (struct WIDGET * Self);
} WIDGET_CLASS;


typedef struct WIDGET {
    char *name;
    WIDGET_CLASS *class;
    struct WIDGET *parent;
    RGBA fg_color;
    RGBA bg_color;
    int fg_valid;
    int bg_valid;
    int layer;
    int row;
    int col;
    void *data;
} WIDGET;


#define WIDGET_TYPE_RC 1
#define WIDGET_TYPE_XY 2
#define WIDGET_TYPE_GPO 3
#define WIDGET_TYPE_TIMER 4
#define WIDGET_TYPE_KEYPAD 5


int widget_register(WIDGET_CLASS * widget);
void widget_unregister(void);
int widget_add(const char *name, const int type, const int layer, const int row, const int col);
WIDGET *widget_find(int type, void *needle);
int widget_color(const char *section, const char *name, const char *key, RGBA * C);

#endif
