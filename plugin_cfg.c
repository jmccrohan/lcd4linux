/* $Id$
 * $URL$
 *
 * plugin for config file access
 *
 * Copyright (C) 2003, 2004 Michael Reinelt <michael@reinelt.co.at>
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

/* 
 * exported functions:
 *
 * int plugin_init_cfg (void)
 *  adds cfg() function for config access
 *  initializes variables from the config file
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "evaluator.h"
#include "plugin.h"
#include "cfg.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static void load_variables(void)
{
    char *section = "Variables";
    char *list, *l, *p;
    char *expression;
    void *tree;
    RESULT result = { 0, 0, 0, NULL };

    list = cfg_list(section);
    l = list;
    while (l != NULL) {
	while (*l == '|')
	    l++;
	if ((p = strchr(l, '|')) != NULL)
	    *p = '\0';
	if (strchr(l, '.') != NULL || strchr(l, ':') != 0) {
	    error("ignoring variable '%s' from %s: structures not allowed", l, cfg_source());
	} else {
	    expression = cfg_get_raw(section, l, "");
	    if (expression != NULL && *expression != '\0') {
		tree = NULL;
		if (Compile(expression, &tree) == 0 && Eval(tree, &result) == 0) {
		    SetVariable(l, &result);
		    debug("Variable %s = '%s' (%g)", l, R2S(&result), R2N(&result));
		    DelResult(&result);
		} else {
		    error("error evaluating variable '%s' from %s", list, cfg_source());
		}
		DelTree(tree);
	    }
	}
	l = p ? p + 1 : NULL;
    }
    free(list);

}


static void my_cfg(RESULT * result, const int argc, RESULT * argv[])
{
    int i, len;
    char *value;
    char *buffer;

    /* calculate key length */
    len = 0;
    for (i = 0; i < argc; i++) {
	len += strlen(R2S(argv[i])) + 1;
    }

    /* allocate key buffer */
    buffer = malloc(len + 1);

    /* prepare key buffer */
    *buffer = '\0';
    for (i = 0; i < argc; i++) {
	strcat(buffer, ".");
	strcat(buffer, R2S(argv[i]));
    }

    /* buffer starts with '.', so cut off first char */
    value = cfg_get("", buffer + 1, "");

    /* store result */
    SetResult(&result, R_STRING, value);

    /* free buffer again */
    free(buffer);

    free(value);
}


int plugin_init_cfg(void)
{
    /* load "Variables" section from cfg */
    load_variables();

    /* register plugin */
    AddFunction("cfg", -1, my_cfg);

    return 0;
}

void plugin_exit_cfg(void)
{
    /* empty */
}
