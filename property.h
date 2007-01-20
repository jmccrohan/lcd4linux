/* $Id$
 * $URL$
 *
 * dynamic properties
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
 */


#ifndef _PROPERTY_H_
#define _PROPERTY_H_

#include "evaluator.h"


typedef struct {
    int valid;
    char *name;
    char *expression;
    void *compiled;
    RESULT result;
} PROPERTY;


void property_load(const char *section, const char *name, const char *defval, PROPERTY * prop);
int property_valid(PROPERTY * prop);
int property_eval(PROPERTY * prop);
double P2N(PROPERTY * prop);
char *P2S(PROPERTY * prop);
void property_free(PROPERTY * prop);

#endif
