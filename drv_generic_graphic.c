/* $Id$
 * $URL$
 *
 * generic driver helper for graphic displays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <michael@reinelt.co.at>
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
 *
 * exported functions:
 *
 * int drv_generic_graphic_init (char *section, char *driver);
 *   initializes the generic graphic driver
 *
 * int drv_generic_graphic_draw (WIDGET *W);
 *   renders Text widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_icon_draw (WIDGET *W);
 *   renders Icon widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_bar_draw (WIDGET *W);
 *   renders Bar widget into framebuffer
 *   calls drv_generic_graphic_real_blit()
 *
 * int drv_generic_graphic_quit (void);
 *   closes generic graphic driver
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"
#include "layout.h"
#include "widget.h"
#include "property.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "widget_image.h"
#include "rgb.h"
#include "drv.h"
#include "drv_generic.h"
#include "drv_generic_graphic.h"
#include "font_6x8.h"
#include "font_6x8_bold.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* pixel colors */
RGBA FG_COL = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0xff };
RGBA BG_COL = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0xff };
RGBA BL_COL = {.R = 0xff,.G = 0xff,.B = 0xff,.A = 0x00 };
RGBA NO_COL = {.R = 0x00,.G = 0x00,.B = 0x00,.A = 0x00 };

static char *Section = NULL;
static char *Driver = NULL;

/* framebuffer */
static RGBA *drv_generic_graphic_FB[LAYERS] = { NULL, };

/* inverted colors */
static int INVERTED = 0;

/* must be implemented by the real driver */
void (*drv_generic_graphic_real_blit) () = NULL;


/****************************************/
/*** generic Framebuffer stuff        ***/
/****************************************/

static void drv_generic_graphic_resizeFB(int rows, int cols)
{
    RGBA *newFB;
    int i, l, row, col;

    /* Layout FB is large enough */
    if (rows <= LROWS && cols <= LCOLS)
	return;

    /* get maximum values */
    if (rows < LROWS)
	rows = LROWS;
    if (cols < LCOLS)
	cols = LCOLS;

    for (l = 0; l < LAYERS; l++) {

	/* allocate and initialize new Layout FB */
	newFB = malloc(cols * rows * sizeof(*newFB));
	for (i = 0; i < rows * cols; i++)
	    newFB[i] = NO_COL;

	/* transfer contents */
	if (drv_generic_graphic_FB[l] != NULL) {
	    for (row = 0; row < LROWS; row++) {
		for (col = 0; col < LCOLS; col++) {
		    newFB[row * cols + col] = drv_generic_graphic_FB[l][row * LCOLS + col];
		}
	    }
	    free(drv_generic_graphic_FB[l]);
	}
	drv_generic_graphic_FB[l] = newFB;
    }

    LCOLS = cols;
    LROWS = rows;

}

static void drv_generic_graphic_window(int pos, int size, int max, int *wpos, int *wsize)
{
    int p1 = pos;
    int p2 = pos + size;

    *wpos = 0;
    *wsize = 0;

    if (p1 > max || p2 < 0 || size < 1)
	return;

    if (p1 < 0)
	p1 = 0;

    if (p2 > max)
	p2 = max;

    *wpos = p1;
    *wsize = p2 - p1;
}

static void drv_generic_graphic_blit(const int row, const int col, const int height, const int width)
{
    if (drv_generic_graphic_real_blit) {
	int r, c, h, w;
	drv_generic_graphic_window(row, height, DROWS, &r, &h);
	drv_generic_graphic_window(col, width, DCOLS, &c, &w);
	if (h > 0 && w > 0) {
	    drv_generic_graphic_real_blit(r, c, h, w);
	}
    }
}

static RGBA drv_generic_graphic_blend(const int row, const int col)
{
    int l, o;
    RGBA p;
    RGBA ret;

    ret.R = BL_COL.R;
    ret.G = BL_COL.G;
    ret.B = BL_COL.B;
    ret.A = 0x00;

    /* find first opaque layer */
    /* layers below are fully covered */
    o = LAYERS - 1;
    for (l = 0; l < LAYERS; l++) {
	p = drv_generic_graphic_FB[l][row * LCOLS + col];
	if (p.A == 255) {
	    o = l;
	    break;
	}
    }

    for (l = o; l >= 0; l--) {
	p = drv_generic_graphic_FB[l][row * LCOLS + col];
	switch (p.A) {
	case 0:
	    break;
	case 255:
	    ret.R = p.R;
	    ret.G = p.G;
	    ret.B = p.B;
	    ret.A = 0xff;
	    break;
	default:
	    ret.R = (p.R * p.A + ret.R * (255 - p.A)) / 255;
	    ret.G = (p.G * p.A + ret.G * (255 - p.A)) / 255;
	    ret.B = (p.B * p.A + ret.B * (255 - p.A)) / 255;
	    ret.A = 0xff;
	}
    }
    if (INVERTED) {
	ret.R = 255 - ret.R;
	ret.G = 255 - ret.G;
	ret.B = 255 - ret.B;
    }

    return ret;
}


/****************************************/
/*** generic text handling            ***/
/****************************************/

static void drv_generic_graphic_render(const int layer, const int row, const int col, const RGBA fg, const RGBA bg,
				       const char *style, const char *txt)
{
    int c, r, x, y, len;

    /* sanity checks */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return;
    }

    len = strlen(txt);

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + YRES, col + XRES * len);

    r = row;
    c = col;

    /* render text into layout FB */
    while (*txt != '\0') {
	unsigned char *chr;

	if (strcasestr(style, "bold") != NULL) {
	    chr = Font_6x8_bold[(int) *(unsigned char *) txt];
	} else {
	    chr = Font_6x8[(int) *(unsigned char *) txt];
	}

	for (y = 0; y < YRES; y++) {
	    int mask = 1 << XRES;
	    for (x = 0; x < XRES; x++) {
		mask >>= 1;
		if (chr[y] & mask)
		    drv_generic_graphic_FB[layer][(r + y) * LCOLS + c + x] = fg;
		else
		    drv_generic_graphic_FB[layer][(r + y) * LCOLS + c + x] = bg;
	    }
	}
	c += XRES;
	txt++;
    }

    /* flush area */
    drv_generic_graphic_blit(row, col, YRES, XRES * len);

}


/* say hello to the user */
int drv_generic_graphic_greet(const char *msg1, const char *msg2)
{
    char *line1[] = { "* LCD4Linux " VERSION " *",
	"LCD4Linux " VERSION,
	"* LCD4Linux *",
	"LCD4Linux",
	"L4Linux",
	NULL
    };

    char *line2[] = { "http://lcd4linux.bulix.org",
	"lcd4linux.bulix.org",
	NULL
    };

    int i;
    int flag = 0;

    unsigned int cols = DCOLS / XRES;
    unsigned int rows = DROWS / YRES;

    for (i = 0; line1[i]; i++) {
	if (strlen(line1[i]) <= cols) {
	    drv_generic_graphic_render(0, YRES * 0, XRES * ((cols - strlen(line1[i])) / 2), FG_COL, BG_COL, "norm",
				       line1[i]);
	    flag = 1;
	    break;
	}
    }

    if (rows >= 2) {
	for (i = 0; line2[i]; i++) {
	    if (strlen(line2[i]) <= cols) {
		drv_generic_graphic_render(0, YRES * 1, XRES * ((cols - strlen(line2[i])) / 2), FG_COL, BG_COL, "norm",
					   line2[i]);
		flag = 1;
		break;
	    }
	}
    }

    if (msg1 && rows >= 3) {
	unsigned int len = strlen(msg1);
	if (len <= cols) {
	    drv_generic_graphic_render(0, YRES * 2, XRES * ((cols - len) / 2), FG_COL, BG_COL, "norm", msg1);
	    flag = 1;
	}
    }

    if (msg2 && rows >= 4) {
	unsigned int len = strlen(msg2);
	if (len <= cols) {
	    drv_generic_graphic_render(0, YRES * 3, XRES * ((cols - len) / 2), FG_COL, BG_COL, "norm", msg2);
	    flag = 1;
	}
    }

    return flag;
}


int drv_generic_graphic_draw(WIDGET * W)
{
    WIDGET_TEXT *Text = W->data;
    RGBA fg, bg;

    fg = W->fg_valid ? W->fg_color : FG_COL;
    bg = W->bg_valid ? W->bg_color : BG_COL;

    drv_generic_graphic_render(W->layer, YRES * W->row, XRES * W->col, fg, bg, P2S(&Text->style), Text->buffer);

    return 0;
}


/****************************************/
/*** generic icon handling            ***/
/****************************************/

int drv_generic_graphic_icon_draw(WIDGET * W)
{
    WIDGET_ICON *Icon = W->data;
    RGBA fg, bg;
    unsigned char *bitmap = Icon->bitmap + YRES * Icon->curmap;
    int layer, row, col;
    int x, y;
    int visible;

    layer = W->layer;
    row = YRES * W->row;
    col = XRES * W->col;

    fg = W->fg_valid ? W->fg_color : FG_COL;
    bg = W->bg_valid ? W->bg_color : BG_COL;

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + YRES, col + XRES);

    /* Icon visible? */
    visible = P2N(&Icon->visible) > 0;

    /* render icon */
    for (y = 0; y < YRES; y++) {
	int mask = 1 << XRES;
	for (x = 0; x < XRES; x++) {
	    int i = (row + y) * LCOLS + col + x;
	    mask >>= 1;
	    if (visible) {
		if (bitmap[y] & mask)
		    drv_generic_graphic_FB[layer][i] = fg;
		else
		    drv_generic_graphic_FB[layer][i] = bg;
	    } else {
		drv_generic_graphic_FB[layer][i] = BG_COL;
	    }
	}
    }

    /* flush area */
    drv_generic_graphic_blit(row, col, YRES, XRES);

    return 0;

}


/****************************************/
/*** generic bar handling             ***/
/****************************************/

int drv_generic_graphic_bar_draw(WIDGET * W)
{
    WIDGET_BAR *Bar = W->data;
    RGBA fg, bg, bar[2];
    int layer, row, col, len, res, rev, max, val1, val2;
    int x, y;
    DIRECTION dir;
    STYLE style;

    layer = W->layer;
    row = YRES * W->row;
    col = XRES * W->col;
    dir = Bar->direction;
    style = Bar->style;
    len = Bar->length;

    fg = W->fg_valid ? W->fg_color : FG_COL;
    bg = W->bg_valid ? W->bg_color : BG_COL;

    bar[0] = Bar->color_valid[0] ? Bar->color[0] : fg;
    bar[1] = Bar->color_valid[1] ? Bar->color[1] : fg;

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* maybe grow layout framebuffer */
    if (dir & (DIR_EAST | DIR_WEST)) {
	drv_generic_graphic_resizeFB(row + YRES, col + XRES * len);
    } else {
	drv_generic_graphic_resizeFB(row + YRES * len, col + XRES);
    }

    res = dir & (DIR_EAST | DIR_WEST) ? XRES : YRES;
    max = len * res;
    val1 = Bar->val1 * (double) (max);
    val2 = Bar->val2 * (double) (max);

    if (val1 < 1)
	val1 = 1;
    else if (val1 > max)
	val1 = max;

    if (val2 < 1)
	val2 = 1;
    else if (val2 > max)
	val2 = max;

    rev = 0;

    switch (dir) {
    case DIR_WEST:
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_EAST:
	for (y = 0; y < YRES; y++) {
	    int val = y < YRES / 2 ? val1 : val2;
	    RGBA bcol = y < YRES / 2 ? bar[0] : bar[1];

	    for (x = 0; x < max; x++) {
		if (x < val)
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bg : bcol;
		else
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bcol : bg;

		if (style) {
		    drv_generic_graphic_FB[layer][(row) * LCOLS + col + x] = fg;
		    drv_generic_graphic_FB[layer][(row + YRES - 1) * LCOLS + col + x] = fg;
		}
	    }
	    if (style) {
		drv_generic_graphic_FB[layer][(row + y) * LCOLS + col] = fg;
		drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + max - 1] = fg;
	    }
	}
	break;

    case DIR_NORTH:
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_SOUTH:
	for (x = 0; x < XRES; x++) {
	    int val = x < XRES / 2 ? val1 : val2;
	    for (y = 0; y < max; y++) {
		if (y < val)
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? bg : fg;
		else
		    drv_generic_graphic_FB[layer][(row + y) * LCOLS + col + x] = rev ? fg : bg;
	    }
	}
	break;
    }

    /* flush area */
    if (dir & (DIR_EAST | DIR_WEST)) {
	drv_generic_graphic_blit(row, col, YRES, XRES * len);
    } else {
	drv_generic_graphic_blit(row, col, YRES * len, XRES);
    }

    return 0;
}


/****************************************/
/*** generic image handling           ***/
/****************************************/

int drv_generic_graphic_image_draw(WIDGET * W)
{
    WIDGET_IMAGE *Image = W->data;
    int layer, row, col, width, height;
    int x, y;
    int visible;

    layer = W->layer;
    row = W->row;
    col = W->col;
    width = Image->width;
    height = Image->height;

    /* sanity check */
    if (layer < 0 || layer >= LAYERS) {
	error("%s: layer %d out of bounds (0..%d)", Driver, layer, LAYERS - 1);
	return -1;
    }

    /* if no size or no image at all, do nothing */
    if (width <= 0 || height <= 0 || Image->bitmap == NULL) {
	return 0;
    }

    /* maybe grow layout framebuffer */
    drv_generic_graphic_resizeFB(row + height, col + width);

    /* render image */
    visible = P2N(&Image->visible);
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    int i = (row + y) * LCOLS + col + x;
	    if (visible) {
		drv_generic_graphic_FB[layer][i] = Image->bitmap[y * width + x];
	    } else {
		drv_generic_graphic_FB[layer][i] = BG_COL;
	    }
	}
    }

    /* flush area */
    drv_generic_graphic_blit(row, col, height, width);

    return 0;

}


/****************************************/
/*** generic init/quit                ***/
/****************************************/

int drv_generic_graphic_init(const char *section, const char *driver)
{
    int i, l;
    char *color;
    WIDGET_CLASS wc;

    Section = (char *) section;
    Driver = (char *) driver;

    /* init layout framebuffer */
    LROWS = 0;
    LCOLS = 0;

    for (l = 0; l < LAYERS; l++)
	drv_generic_graphic_FB[l] = NULL;

    drv_generic_graphic_resizeFB(DROWS, DCOLS);

    /* sanity check */
    for (l = 0; l < LAYERS; l++) {
	if (drv_generic_graphic_FB[l] == NULL) {
	    error("%s: framebuffer could not be allocated: malloc() failed", Driver);
	    return -1;
	}
    }

    /* init generic driver & register plugins */
    drv_generic_init();

    /* set default colors */
    color = cfg_get(Section, "foreground", "000000ff");
    if (color2RGBA(color, &FG_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Driver, color);
    }
    if (color)
	free(color);

    color = cfg_get(Section, "background", "ffffff00");
    if (color2RGBA(color, &BG_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Driver, color);
    }
    if (color)
	free(color);

    color = cfg_get(Section, "basecolor", "ffffff");
    if (color2RGBA(color, &BL_COL) < 0) {
	error("%s: ignoring illegal color '%s'", Driver, color);
    }
    if (color)
	free(color);

    /* inverted display? */
    cfg_number(section, "inverted", 0, 0, 1, &INVERTED);

    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_graphic_draw;
    widget_register(&wc);

    /* register icon widget */
    wc = Widget_Icon;
    wc.draw = drv_generic_graphic_icon_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_graphic_bar_draw;
    widget_register(&wc);

    /* register image widget */
#ifdef WITH_IMAGE
    wc = Widget_Image;
    wc.draw = drv_generic_graphic_image_draw;
    widget_register(&wc);
#endif

    /* clear framebuffer but do not blit to display */
    for (l = 0; l < LAYERS; l++)
	for (i = 0; i < LCOLS * LROWS; i++)
	    drv_generic_graphic_FB[l][i] = NO_COL;

    return 0;
}


int drv_generic_graphic_clear(void)
{
    int i, l;

    for (l = 0; l < LAYERS; l++)
	for (i = 0; i < LCOLS * LROWS; i++)
	    drv_generic_graphic_FB[l][i] = NO_COL;

    drv_generic_graphic_blit(0, 0, LROWS, LCOLS);

    return 0;
}


RGBA drv_generic_graphic_rgb(const int row, const int col)
{
    return drv_generic_graphic_blend(row, col);
}


unsigned char drv_generic_graphic_gray(const int row, const int col)
{
    RGBA p = drv_generic_graphic_blend(row, col);
    return (77 * p.R + 150 * p.G + 28 * p.B) / 255;
}


unsigned char drv_generic_graphic_black(const int row, const int col)
{
    return drv_generic_graphic_gray(row, col) < 127;
}


int drv_generic_graphic_quit(void)
{
    int l;

    for (l = 0; l < LAYERS; l++) {
	if (drv_generic_graphic_FB[l]) {
	    free(drv_generic_graphic_FB[l]);
	    drv_generic_graphic_FB[l] = NULL;
	}
    }
    widget_unregister();
    return (0);
}
