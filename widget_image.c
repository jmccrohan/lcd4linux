/* $Id$
 * $URL$
 *
 * image widget handling
 *
 * Copyright (C) 2006 Michael Reinelt <michael@reinelt.co.at>
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

#ifdef HAVE_GD_GD_H
#include <gd/gd.h>
#else
#ifdef HAVE_GD_H
#include <gd.h>
#else
#error "gd.h not found!"
#error "cannot compile image widget"
#endif
#endif

#if GD2_VERS != 2
#error "lcd4linux requires libgd version 2"
#error "cannot compile image widget"
#endif

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "property.h"
#include "timer.h"
#include "widget.h"
#include "widget_image.h"
#include "rgb.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static void widget_image_render(const char *Name, WIDGET_IMAGE * Image)
{
    int x, y;
    int inverted;
    gdImagePtr gdImage;

    /* clear bitmap */
    if (Image->bitmap) {
	int i;
	for (i = 0; i < Image->height * Image->width; i++) {
	    RGBA empty = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };
	    Image->bitmap[i] = empty;
	}
    }

    /* reload image only on first call or on explicit reload request */
    if (Image->gdImage == NULL || P2N(&Image->reload)) {

	char *file;
	FILE *fd;

	/* free previous image */
	if (Image->gdImage) {
	    gdImageDestroy(Image->gdImage);
	    Image->gdImage = NULL;
	}

	file = P2S(&Image->file);
	if (file == NULL || file[0] == '\0') {
	    error("Warning: Image %s has no file", Name);
	    return;
	}

	fd = fopen(file, "rb");
	if (fd == NULL) {
	    error("Warning: Image %s: fopen(%s) failed: %s", Name, file, strerror(errno));
	    return;
	}

	Image->gdImage = gdImageCreateFromPng(fd);
	fclose(fd);

	if (Image->gdImage == NULL) {
	    error("Warning: Image %s: CreateFromPng(%s) failed!", Name, file);
	    return;
	}

    }

    /* maybe resize bitmap */
    gdImage = Image->gdImage;
    if (gdImage->sx > Image->width) {
	Image->width = gdImage->sx;
	free(Image->bitmap);
	Image->bitmap = NULL;
    }
    if (gdImage->sy > Image->height) {
	Image->height = gdImage->sy;
	free(Image->bitmap);
	Image->bitmap = NULL;
    }
    if (Image->bitmap == NULL && Image->width > 0 && Image->height > 0) {
	int i = Image->width * Image->height * sizeof(Image->bitmap[0]);
	Image->bitmap = malloc(i);
	if (Image->bitmap == NULL) {
	    error("Warning: Image %s: malloc(%d) failed: %s", Name, i, strerror(errno));
	    return;
	}
	for (i = 0; i < Image->height * Image->width; i++) {
	    RGBA empty = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };
	    Image->bitmap[i] = empty;
	}
    }


    /* finally really render it */
    inverted = P2N(&Image->inverted);
    if (P2N(&Image->visible)) {
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
		if (inverted) {
		    Image->bitmap[i].R = 255 - Image->bitmap[i].R;
		    Image->bitmap[i].G = 255 - Image->bitmap[i].G;
		    Image->bitmap[i].B = 255 - Image->bitmap[i].B;
		}
	    }
	}
    }
}


static void widget_image_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_IMAGE *Image = W->data;

    /* process the parent only */
    if (W->parent == NULL) {

	/* evaluate properties */
	property_eval(&Image->file);
	property_eval(&Image->update);
	property_eval(&Image->reload);
	property_eval(&Image->visible);
	property_eval(&Image->inverted);

	/* render image into bitmap */
	widget_image_render(W->name, Image);

    }

    /* finally, draw it! */
    if (W->class->draw)
	W->class->draw(W);

    /* add a new one-shot timer */
    if (P2N(&Image->update) > 0) {
	timer_add(widget_image_update, Self, P2N(&Image->update), 1);
    }
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

	/* load properties */
	property_load(section, "file", NULL, &Image->file);
	property_load(section, "update", "100", &Image->update);
	property_load(section, "reload", "0", &Image->reload);
	property_load(section, "visible", "1", &Image->visible);
	property_load(section, "inverted", "0", &Image->inverted);

	/* sanity checks */
	if (!property_valid(&Image->file)) {
	    error("Warning: widget %s has no file", section);
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
		if (Image->gdImage) {
		    gdImageDestroy(Image->gdImage);
		    Image->gdImage = NULL;
		}
		free(Image->bitmap);
		property_free(&Image->file);
		property_free(&Image->update);
		property_free(&Image->reload);
		property_free(&Image->visible);
		property_free(&Image->inverted);
		free(Self->data);
		Self->data = NULL;
	    }
	}
    }

    return 0;

}



WIDGET_CLASS Widget_Image = {
    .name = "image",
    .type = WIDGET_TYPE_XY,
    .init = widget_image_init,
    .draw = NULL,
    .quit = widget_image_quit,
};
