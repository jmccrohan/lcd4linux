/* $Id: property.c,v 1.1 2006/08/13 09:53:10 reinelt Exp $
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
 *
 * $Log: property.c,v $
 * Revision 1.1  2006/08/13 09:53:10  reinelt
 * dynamic properties added (used by 'style' of text widget)
 *
 */


/* 
 * exported functions:
 *
 * void property_load (const char *section, const char *name, const char *defval, PROPERTY *prop)
 *   initializes and loads a property from the config file and pre-compiles it
 *
 * void property_free (PROPERTY *prop)
 *   frees all property allocations
 *
 *
 * double P2N(PROPERTY * prop)
 *   returns a (already evaluated) property as number
 *
 * char *P2S(PROPERTY * prop)
 *   returns a (already evaluated) property as string
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "evaluator.h"
#include "property.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void property_load(const char *section, const char *name, const char *defval, PROPERTY * prop)
{
    /* initialize structure */
    prop->name = NULL;
    prop->expression = NULL;
    prop->compiled = NULL;
    DelResult(&prop->result);

    /* remember the name */
    prop->name = strdup(name);

    /* load expression from config, but do not evaluate it */
    prop->expression = cfg_get_raw(section, name, defval);

    /* pre-compile the expression */
    Compile(prop->expression, &prop->compiled);

}


void property_eval(PROPERTY * prop)
{
    if (prop->compiled != NULL) {
	DelResult(&prop->result);
	Eval(prop->compiled, &prop->result);
    }
}


double P2N(PROPERTY * prop)
{
    if (prop == NULL) {
	error("Property: internal error: NULL property");
	return 0.0;
    }
    return R2N(&prop->result);
}


char *P2S(PROPERTY * prop)
{
    if (prop == NULL) {
	error("Property: internal error: NULL property");
	return NULL;
    }
    return R2S(&prop->result);
}

void property_free(PROPERTY * prop)
{
    if (prop->name != NULL) {
	free(prop->name);
	prop->name = NULL;
    }

    if (prop->expression != NULL) {
	/* do *not* free expression */
	prop->expression = NULL;
    }

    if (prop->compiled != NULL) {
	free(prop->compiled);
	prop->compiled = NULL;
    }

    DelResult(&prop->result);
}
