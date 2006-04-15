/* $Id: widget_image.c,v 1.7 2006/04/15 05:22:52 reinelt Exp $
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
 * Revision 1.7  2006/04/15 05:22:52  reinelt
 * mpd plugin from Stefan Kuhne
 *
 * Revision 1.6  2006/04/09 14:17:50  reinelt
 * autoconf/library fixes, image and graphic display inversion
 *
 * Revision 1.5  2006/02/25 13:36:33  geronet
 * updated indent.sh, applied coding style
 *
 * Revision 1.4  2006/02/19 07:20:54  reinelt
 * image support nearly finished
 *
 * Revision 1.3  2006/02/08 04:55:05  reinelt
 * moved widget registration to drv_generic_graphic
 *
 * Revision 1.2  2006/01/23 06:17:18  reinelt
 * timer widget added
 *
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
#include <errno.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "evaluator.h"
#include "timer.h"
#include "widget.h"
#include "widget_image.h"
#include "rgb.h"

#ifdef HAVE_GD_GD_H
#include <gd/gd.h>
#define WITH_GD
#else
#ifdef HAVE_GD_H
#include <gd.h>
#define WITH_GD
#endif
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


#ifdef WITH_GD
static void widget_image_render(const char *Name, WIDGET_IMAGE * Image)
{
    int x, y;
    FILE *fd;
    gdImagePtr gdImage = NULL;

    /* clear bitmap */
    if (Image->bitmap) {
	int i;
	for (i = 0; i < Image->height * Image->width; i++) {
	  RGBA empty = { R: 0x00, G: 0x00, B: 0x00, A:0x00 };
	    Image->bitmap[i] = empty;
	}
    }

    if (Image->file == NULL || Image->file[0] == '\0') {
	error("Warning: Image %s has no file", Name);
	return;
    }

    fd = fopen(Image->file, "rb");
    if (fd == NULL) {
	error("Warning: Image %s: fopen(%s) failed: %s", Name, Image->file, strerror(errno));
	return;
    }

    gdImage = gdImageCreateFromPng(fd);
    fclose(fd);

    if (fd == NULL) {
	error("Warning: Image %s: CreateFromPng(%s) failed!", Name, Image->file);
	return;
    }

    /* maybe resize bitmap */
    if (gdImage->sx > Image->width) {
	Image->width = gdImage->sx;
	if (Image->bitmap)
	    free(Image->bitmap);
	Image->bitmap = NULL;
    }
    if (gdImage->sy > Image->height) {
	Image->height = gdImage->sy;
	if (Image->bitmap)
	    free(Image->bitmap);
	Image->bitmap = NULL;
    }
    if (Image->bitmap == NULL && Image->width > 0 && Image->height > 0) {
	int i = Image->width * Image->height * sizeof(Image->bitmap[0]);
	Image->bitmap = malloc(i);
	if (Image->bitmap == NULL) {
	    error("Warning: Image %s: malloc(%d) failed!", Name, i, strerror(errno));
	    return;
	}
	for (i = 0; i < Image->height * Image->width; i++) {
	  RGBA empty = { R: 0x00, G: 0x00, B: 0x00, A:0x00 };
	    Image->bitmap[i] = empty;
	}
    }

    /* finally really render it */
    if (Image->visible) {
	for (x = 0; x < gdImage->sx; x++) {
	    for (y = 0; y < gdImage->sy; y++) {
		int p = gdImageGetTrueColorPixel(gdImage, x, y);
		int a = gdTrueColorGetAlpha(p);
		int i = y * Image->width + x;
		Image->bitmap[i].R = gdTrueColorGetRed(p);
		Image->bitmap[i].G = gdTrueColorGetGreen(p);
		Image->bitmap[i].B = gdTrueColorGetBlue(p);
		/* GD's alpha is 0 (opaque) to 127 (tranparanet) */
		/* our alpha is 0 (transparent) to 255 (opaque) */
		Image->bitmap[i].A = (a == 127) ? 0 : 255 - 2 * a;
		if (Image->inverted) {
		    Image->bitmap[i].R = 255 - Image->bitmap[i].R;
		    Image->bitmap[i].G = 255 - Image->bitmap[i].G;
		    Image->bitmap[i].B = 255 - Image->bitmap[i].B;
		}
	    }
	}
    }
}
#endif


static void widget_image_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_IMAGE *Image = W->data;
    RESULT result = { 0, 0, 0, NULL };

    /* process the parent only */
    if (W->parent == NULL) {

	/* evaluate expressions */
	if (Image->file) {
	    free(Image->file);
	    Image->file = NULL;
	}
	if (Image->file_tree != NULL) {
	    Eval(Image->file_tree, &result);
	    Image->file = strdup(R2S(&result));
	    DelResult(&result);
	}

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
	    Image->visible = Image->visible > 0;
	    DelResult(&result);
	}

	Image->inverted = 0;
	if (Image->inverted_tree != NULL) {
	    Eval(Image->inverted_tree, &result);
	    Image->inverted = R2N(&result);
	    Image->inverted = Image->inverted > 0;
	    DelResult(&result);
	}
#ifdef WITH_GD
	/* render image into bitmap */
	widget_image_render(W->name, Image);
#endif

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

	/* initial size */
	Image->width = 0;
	Image->height = 0;
	Image->bitmap = NULL;
	Image->file = NULL;

	/* get raw expressions (we evaluate them ourselves) */
	Image->file_expr = cfg_get_raw(section, "file", NULL);
	Image->update_expr = cfg_get_raw(section, "update", NULL);
	Image->visible_expr = cfg_get_raw(section, "visible", NULL);
	Image->inverted_expr = cfg_get_raw(section, "inverted", NULL);

	/* sanity checks */
	if (Image->file_expr == NULL || *Image->file_expr == '\0') {
	    error("Warning: Image %s has no file", Self->name);
	}
	if (Image->update_expr == NULL || *Image->update_expr == '\0') {
	    error("Image %s has no update, using '100'", Self->name);
	    Image->update_expr = "100";
	}

	/* compile'em */
	Compile(Image->file_expr, &Image->file_tree);
	Compile(Image->update_expr, &Image->update_tree);
	Compile(Image->visible_expr, &Image->visible_tree);
	Compile(Image->inverted_expr, &Image->inverted_tree);

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
		if (Image->bitmap)
		    free(Image->bitmap);
		if (Image->file)
		    free(Image->file);
		DelTree(Image->file_tree);
		DelTree(Image->update_tree);
		DelTree(Image->visible_tree);
		DelTree(Image->inverted_tree);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }

    return 0;

}



WIDGET_CLASS Widget_Image = {
  name:"image",
  type:WIDGET_TYPE_XY,
  init:widget_image_init,
  draw:NULL,
  quit:widget_image_quit,
};
