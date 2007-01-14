/* $Id$
 * $URL$
 *
 * image widget handling
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


#ifndef _WIDGET_IMAGE_H_
#define _WIDGET_IMAGE_H_

#include "property.h"
#include "rgb.h"

typedef struct WIDGET_IMAGE {
    void *gdImage;		/* raw gd image */
    RGBA *bitmap;		/* image bitmap */
    int width, height;		/* size of the image */
    PROPERTY file;		/* image filename */
    PROPERTY update;		/* update interval */
    PROPERTY reload;		/* reload image on update? */
    PROPERTY visible;		/* image visible? */
    PROPERTY inverted;		/* image inverted? */
} WIDGET_IMAGE;

extern WIDGET_CLASS Widget_Image;

#endif
