/* $Id: widget_image.c,v 1.1 2006/01/22 09:16:11 reinelt Exp $
 *
 * image widget handling
 *
 * Copyright (C) 2006 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: widget_image.c,v $
 * Revision 1.1  2006/01/22 09:16:11  reinelt
 * Image Widget framework added
 *
 */

/* 
 * exported functions:
 *
 * WIDGET_CLASS Widget_Image
 *   the image widget
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "evaluator.h"
#include "timer.h"
#include "widget.h"
#include "widget_image.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

void widget_image_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_IMAGE *Image = W->data;
    RESULT result = { 0, 0, 0, NULL };

    /* process the parent only */
    if (W->parent == NULL) {

	/* evaluate expressions */
	Image->update = 0;
	if (Image->update_tree != NULL) {
	    Eval(Image->update_tree, &result);
	    Image->update = R2N(&result);
	    if (Image->update < 0)
		Image->update = 0;
	    DelResult(&result);
	}

	Image->visible = 1;
	if (Image->visible_tree != NULL) {
	    Eval(Image->visible_tree, &result);
	    Image->visible = R2N(&result);
	    if (Image->visible < 0)
		Image->visible = 0;
	    DelResult(&result);
	}
    }

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

    /* add a new one-shot timer */
    timer_add(widget_image_update, Self, Image->update, 1);

}



int widget_image_init(WIDGET * Self)
{
    char *section;
    WIDGET_IMAGE *Image;

    /* re-use the parent if one exists */
    if (Self->parent == NULL) {

	/* prepare config section */
	/* strlen("Widget:")=7 */
	section = malloc(strlen(Self->name) + 8);
	strcpy(section, "Widget:");
	strcat(section, Self->name);

	Image = malloc(sizeof(WIDGET_IMAGE));
	memset(Image, 0, sizeof(WIDGET_IMAGE));

	/* get raw expressions (we evaluate them ourselves) */
	Image->update_expr = cfg_get_raw(section, "update", NULL);
	Image->visible_expr = cfg_get_raw(section, "visible", NULL);

	/* compile'em */
	Compile(Image->update_expr, &Image->update_tree);
	Compile(Image->visible_expr, &Image->visible_tree);

	/* sanity check */
	if (Image->update_expr == NULL || *Image->update_expr == '\0') {
	    error("Image %s has no update, using '100'", Self->name);
	    Image->update_expr = "100";
	}

	free(section);
	Self->data = Image;

    } else {

	/* re-use the parent */
	Self->data = Self->parent->data;

    }

    /* just do it! */
    widget_image_update(Self);

    return 0;
}


int widget_image_quit(WIDGET * Self)
{
    if (Self) {
	/* do not deallocate child widget! */
	if (Self->parent == NULL) {
	    if (Self->data) {
		WIDGET_IMAGE *Image = Self->data;
		DelTree(Image->update_tree);
		DelTree(Image->visible_tree);
		if (Image->file)
		    free(Image->file);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }

    return 0;

}



WIDGET_CLASS Widget_Image = {
  name:"image",
  type:WIDGET_TYPE_VIS,
  init:widget_image_init,
  draw:NULL,
  quit:widget_image_quit,
};
