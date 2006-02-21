/* $Id: widget_keypad.c,v 1.1 2006/02/21 05:50:34 reinelt Exp $
 *
 * keypad widget handling
 *
 * Copyright (C) 2006 Chris Maj <cmaj@freedomcorpse.com>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: widget_keypad.c,v $
 * Revision 1.1  2006/02/21 05:50:34  reinelt
 * keypad support from Cris Maj
 *
 *
 */

/* 
 * exported functions:
 *
 * WIDGET_CLASS Widget_Keypad
 *   the keypad widget
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cfg.h"
#include "evaluator.h"
#include "timer.h"
#include "widget.h"
#include "widget_keypad.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


void widget_keypad_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_KEYPAD *keypad = W->data;
    RESULT result = { 0, 0, 0, NULL };

    int val;

    /* evaluate expression */
    val = 0;
    if (keypad->tree != NULL) {
	Eval(keypad->tree, &result);
	val = R2N(&result);
	DelResult(&result);
    }
    keypad->val = val;

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

}


int widget_keypad_init(WIDGET * Self)
{
    char *section;
    char *c;
    WIDGET_KEYPAD *keypad;

    /* prepare config section */
    /* strlen("Widget:")=7 */
    section = malloc(strlen(Self->name) + 8);
    strcpy(section, "Widget:");
    strcat(section, Self->name);

    keypad = malloc(sizeof(WIDGET_KEYPAD));
    memset(keypad, 0, sizeof(WIDGET_KEYPAD));

    /* get raw expression (we evaluate them ourselves) */
    keypad->expression = cfg_get_raw(section, "expression", NULL);

    /* sanity check */
    if (keypad->expression == NULL || *keypad->expression == '\0') {
	error("widget %s has no expression, using '0.0'", Self->name);
	keypad->expression = "0";
    }

    /* compile expression */
    Compile(keypad->expression, &keypad->tree);

    /* state: pressed (default), released */
    c = cfg_get(section, "state", "pressed");
    if (!strcasecmp(c, "released"))
	keypad->key = KEY_RELEASED;
    else
	keypad->key = KEY_PRESSED;

    /* position: confirm (default), up, down, left, right, cancel */
    c = cfg_get(section, "position", "confirm");
    if (!strcasecmp(c, "up"))
	keypad->key += KEY_UP;
    else if (!strcasecmp(c, "down"))
	keypad->key += KEY_DOWN;
    else if (!strcasecmp(c, "left"))
	keypad->key += KEY_LEFT;
    else if (!strcasecmp(c, "right"))
	keypad->key += KEY_RIGHT;
    else if (!strcasecmp(c, "cancel"))
	keypad->key += KEY_CANCEL;
    else
	keypad->key += KEY_CONFIRM;

    free(section);
    Self->data = keypad;

    return 0;
}

int widget_keypad_find(WIDGET * Self, void *needle)
{
    WIDGET_KEYPAD *keypad;
    unsigned int *n = needle;

    if (Self) {
	if (Self->data) {
	    keypad = Self->data;
	    if (keypad->key == *n)
		return 0;
	}
    }

    return -1;
}

int widget_keypad_quit(WIDGET * Self)
{
    if (Self) {
	if (Self->data) {
	    WIDGET_KEYPAD *KEYPAD = Self->data;
	    DelTree(KEYPAD->tree);
	    free(Self->data);
	}
	Self->data = NULL;
    }
    return 0;
}



WIDGET_CLASS Widget_Keypad = {
  name:"keypad",
  type:WIDGET_TYPE_KEYPAD,
  init:widget_keypad_init,
  draw:NULL,
  find:widget_keypad_find,
  update:widget_keypad_update,
  quit:widget_keypad_quit,
};
