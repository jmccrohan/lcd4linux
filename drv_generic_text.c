/* $Id$
 * $URL$
 *
 * generic driver helper for text-based displays
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
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
 * exported variables:
 *
 * extern int CHARS, CHAR0;    number of user-defineable characters, ASCII of first char
 * extern int ICONS;           number of user-defineable characters reserved for icons
 * extern int GOTO_COST;       number of bytes a goto command requires
 * extern int INVALIDATE;      re-send a modified userdefined char?
 *
 *
 * these functions must be implemented by the real driver:
 *
 * void (*drv_generic_text_real_write)(int row, int col, unsigned char *data, int len);
 *  writes a text of specified length at position (row, col)
 *
 * void (*drv_generic_text_real_defchar)(int ascii, unsigned char *buffer);
 *  defines the bitmap of a user-defined character
 *
 *
 * exported fuctions:
 *
 * int drv_generic_text_init (char *section, char *driver);
 *   initializes the generic text driver
 *
 * int drv_generic_text_draw (WIDGET *W);
 *   renders Text widget into framebuffer
 *   calls drv_generic_text_real_write()
 *
 * int drv_generic_text_icon_init (void);
 *   initializes the generic icon driver
 *   
 * int drv_generic_text_icon_draw (WIDGET *W);
 *   renders Icon widget into framebuffer
 *   calls drv_generic_text_real_write() and drv_generic_text_real_defchar()
 *
 * int drv_generic_text_bar_init (int single_segments);
 *   initializes the generic icon driver
 *
 * void drv_generic_text_bar_add_segment (int val1, int val2, DIRECTION dir, int ascii);
 *   adds a 'fixed' character to the bar-renderer
 *
 * int drv_generic_text_bar_draw (WIDGET *W);
 *   renders Bar widget into framebuffer
 *   calls drv_generic_text_real_write() and drv_generic_text_real_defchar()
 *
 * int drv_generic_text_quit (void);
 *   closes the generic text driver
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
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic.h"
#include "drv_generic_text.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


typedef struct {
    int val1;
    int val2;
    DIRECTION dir;
    STYLE style;
    int segment;
    int invalid;
} BAR;

typedef struct {
    int val1;
    int val2;
    DIRECTION dir;
    STYLE style;
    int used;
    int ascii;
} SEGMENT;

static char *Section = NULL;
static char *Driver = NULL;

int CHARS = 0;			/* number of user-defineable characters */
int CHAR0 = 0;			/* ASCII of first user-defineable char */
int ICONS = 0;			/* number of user-defineable characters reserved for icons */

int GOTO_COST = 0;		/* number of bytes a goto command requires */
int INVALIDATE = 0;		/* re-send a modified userdefined char? */


void (*drv_generic_text_real_write) () = NULL;
void (*drv_generic_text_real_defchar) () = NULL;


static char *LayoutFB = NULL;
static char *DisplayFB = NULL;
static int *L2D = NULL;

static int Single_Segments = 0;

static int nSegment = 0;
static int fSegment = 0;
static SEGMENT Segment[128];
static BAR *BarFB = NULL;


/****************************************/
/*** generic Framebuffer stuff        ***/
/****************************************/

static void drv_generic_text_resizeFB(int rows, int cols)
{
    char *newFB;
    int *newL2D;
    BAR *newBar;
    int i, row, col;

    /* Layout FB is large enough */
    if (rows <= LROWS && cols <= LCOLS)
	return;

    /* get maximum values */
    if (rows < LROWS)
	rows = LROWS;
    if (cols < LCOLS)
	cols = LCOLS;

    /* allocate new Layout FB */
    newFB = malloc(cols * rows * sizeof(*newFB));
    memset(newFB, ' ', rows * cols * sizeof(*newFB));

    /* transfer contents */
    if (LayoutFB != NULL) {
	for (row = 0; row < LROWS; row++) {
	    for (col = 0; col < LCOLS; col++) {
		newFB[row * cols + col] = LayoutFB[row * LCOLS + col];
	    }
	}
	free(LayoutFB);
    }
    LayoutFB = newFB;

    /* allocate new transformer */
    newL2D = malloc(cols * rows * sizeof(*newL2D));

    /* intitialize translator */
    for (row = 0; row < rows; row++) {
	for (col = 0; col < cols; col++) {
	    newL2D[row * cols + col] = row * DCOLS + col;
	}
    }

    /* transfer transformer */
    if (L2D != NULL) {
	for (row = 0; row < LROWS; row++) {
	    for (col = 0; col < LCOLS; col++) {
		newL2D[row * cols + col] = L2D[row * LCOLS + col];
	    }
	}
	free(L2D);
    }
    L2D = newL2D;

    /* resize Bar buffer */
    if (BarFB) {

	newBar = malloc(rows * cols * sizeof(BAR));

	for (i = 0; i < rows * cols; i++) {
	    newBar[i].val1 = -1;
	    newBar[i].val2 = -1;
	    newBar[i].dir = 0;
	    newBar[i].segment = -1;
	    newBar[i].invalid = 0;
	}

	/* transfer contents */
	for (row = 0; row < LROWS; row++) {
	    for (col = 0; col < LCOLS; col++) {
		newBar[row * cols + col] = BarFB[row * LCOLS + col];
	    }
	}

	free(BarFB);
	BarFB = newBar;
    }

    LCOLS = cols;
    LROWS = rows;
}


static void drv_generic_text_blit(const int row, const int col, const int height, const int width)
{
    int lr, lc;			/* layout  row/col */
    int dr, dc;			/* display row/col */
    int p1, p2;			/* start/end positon of changed area */
    int eq;			/* counter for equal contents */

    /* loop over layout rows */
    for (lr = row; lr < LROWS && lr < row + height; lr++) {
	/* transform layout to display row */
	dr = lr;
	/* sanity check */
	if (dr < 0 || dr >= DROWS)
	    continue;
	/* loop over layout cols */
	for (lc = col; lc < LCOLS && lc < col + width; lc++) {
	    /* transform layout to display column */
	    dc = lc;
	    /* sanity check */
	    if (dc < 0 || dc >= DCOLS)
		continue;
	    /* find start of difference */
	    if (DisplayFB[dr * DCOLS + dc] == LayoutFB[lr * LCOLS + lc])
		continue;
	    /* find end of difference */
	    for (p1 = dc, p2 = p1, eq = 0, lc++; lc < LCOLS && lc < col + width; lc++) {
		/* transform layout to display column */
		dc = lc;
		/* sanity check */
		if (dc < 0 || dc >= DCOLS)
		    continue;
		if (DisplayFB[dr * DCOLS + dc] == LayoutFB[lr * LCOLS + lc]) {
		    if (++eq > GOTO_COST)
			break;
		} else {
		    p2 = dc;
		    eq = 0;
		}
	    }
	    /* send to display */
	    memcpy(DisplayFB + dr * DCOLS + p1, LayoutFB + lr * LCOLS + p1, p2 - p1 + 1);
	    if (drv_generic_text_real_write)
		drv_generic_text_real_write(dr, p1, DisplayFB + dr * DCOLS + p1, p2 - p1 + 1);
	}
    }
}


/****************************************/
/*** generic text handling            ***/
/****************************************/

int drv_generic_text_init(const char *section, const char *driver)
{

    Section = (char *) section;
    Driver = (char *) driver;

    /* init display framebuffer */
    DisplayFB = (char *) malloc(DCOLS * DROWS * sizeof(*DisplayFB));
    memset(DisplayFB, ' ', DROWS * DCOLS * sizeof(*DisplayFB));

    /* init layout framebuffer */
    LROWS = 0;
    LCOLS = 0;
    LayoutFB = NULL;
    L2D = NULL;
    drv_generic_text_resizeFB(DROWS, DCOLS);

    /* sanity check */
    if (DisplayFB == NULL || LayoutFB == NULL || L2D == NULL) {
	error("%s: framebuffer could not be allocated: malloc() failed", Driver);
	return -1;
    }

    /* init generic driver & register plugins */
    drv_generic_blit = drv_generic_text_blit;
    drv_generic_init();

    return 0;
}


/* say hello to the user */
int drv_generic_text_greet(const char *msg1, const char *msg2)
{
    int i;
    int flag = 0;

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


    for (i = 0; line1[i]; i++) {
	if (strlen(line1[i]) <= (unsigned) DCOLS) {
	    if (drv_generic_text_real_write)
		drv_generic_text_real_write(0, (DCOLS - strlen(line1[i])) / 2, line1[i], strlen(line1[i]));
	    flag = 1;
	    break;
	}
    }

    if (DROWS >= 2) {
	for (i = 0; line2[i]; i++) {
	    if (strlen(line2[i]) <= (unsigned) DCOLS) {
		if (drv_generic_text_real_write)
		    drv_generic_text_real_write(1, (DCOLS - strlen(line2[i])) / 2, line2[i], strlen(line2[i]));
		flag = 1;
		break;
	    }
	}
    }

    if (msg1 && DROWS >= 3) {
	int len = strlen(msg1);
	if (len <= DCOLS) {
	    if (drv_generic_text_real_write)
		drv_generic_text_real_write(2, (DCOLS - len) / 2, msg1, len);
	    flag = 1;
	}
    }

    if (msg2 && DROWS >= 4) {
	int len = strlen(msg2);
	if (len <= DCOLS) {
	    if (drv_generic_text_real_write)
		drv_generic_text_real_write(3, (DCOLS - len) / 2, msg2, len);
	    flag = 1;
	}
    }

    return flag;
}


int drv_generic_text_draw(WIDGET * W)
{
    WIDGET_TEXT *Text = W->data;
    char *txt;
    int row, col, len;

    row = W->row;
    col = W->col;
    txt = Text->buffer;
    len = strlen(txt);

    /* maybe grow layout framebuffer */
    drv_generic_text_resizeFB(row + 1, col + len);

    /* transfer new text into layout buffer */
    memcpy(LayoutFB + row * LCOLS + col, txt, len);

    /* blit it */
    drv_generic_text_blit(row, col, 1, len);

    return 0;
}


int drv_generic_text_quit(void)
{

    if (DisplayFB) {
	free(DisplayFB);
	DisplayFB = NULL;
    }

    if (LayoutFB) {
	free(LayoutFB);
	LayoutFB = NULL;
    }

    if (L2D) {
	free(L2D);
	L2D = NULL;
    }

    if (BarFB) {
	free(BarFB);
	BarFB = NULL;
    }

    widget_unregister();

    return (0);
}


/****************************************/
/*** generic icon handling            ***/
/****************************************/

int drv_generic_text_icon_init(void)
{
    if (cfg_number(Section, "Icons", 0, 0, CHARS, &ICONS) < 0)
	return -1;
    if (ICONS > 0) {
	info("%s: reserving %d of %d user-defined characters for icons", Driver, ICONS, CHARS);
    }
    return 0;
}


int drv_generic_text_icon_draw(WIDGET * W)
{
    static int icon_counter = 0;
    WIDGET_ICON *Icon = W->data;
    int row, col;
    int visible;
    int invalidate = 0;
    unsigned char ascii;

    row = W->row;
    col = W->col;

    /* maybe grow layout framebuffer */
    drv_generic_text_resizeFB(row + 1, col + 1);

    /* icon deactivated? */
    if (Icon->ascii == -2)
	return 0;

    /* ASCII already assigned? */
    if (Icon->ascii == -1) {
	if (icon_counter >= ICONS) {
	    error("cannot process icon '%s': out of icons", W->name);
	    Icon->ascii = -2;
	    return -1;
	}
	icon_counter++;
	Icon->ascii = CHAR0 + CHARS - icon_counter;
    }

    /* Icon visible? */
    visible = P2N(&Icon->visible) > 0;

    /* maybe redefine icon */
    if (Icon->curmap != Icon->prvmap && visible) {
	Icon->prvmap = Icon->curmap;
	if (drv_generic_text_real_defchar)
	    drv_generic_text_real_defchar(Icon->ascii, Icon->bitmap + YRES * Icon->curmap);
	invalidate = INVALIDATE;
    }

    /* use blank if invisible */
    ascii = visible ? Icon->ascii : ' ';

    /* transfer icon into layout buffer */
    LayoutFB[row * LCOLS + col] = ascii;

    /* ugly invalidation: change display FB to a wrong value so blit() will really send it */
    if (invalidate) {
	DisplayFB[row * DCOLS + col] = ~ascii;
    }

    /* blit it */
    drv_generic_text_blit(row, col, 1, 1);

    return 0;

}


/****************************************/
/*** generic bar handling             ***/
/****************************************/

static void drv_generic_text_bar_clear(void)
{
    int i;

    for (i = 0; i < LROWS * LCOLS; i++) {
	BarFB[i].val1 = -1;
	BarFB[i].val2 = -1;
	BarFB[i].dir = 0;
	BarFB[i].style = 0;
	BarFB[i].segment = -1;
	BarFB[i].invalid = 0;
    }

    for (i = 0; i < nSegment; i++) {
	Segment[i].used = 0;
    }
}


int drv_generic_text_bar_init(const int single_segments)
{
    if (BarFB)
	free(BarFB);

    if ((BarFB = malloc(LROWS * LCOLS * sizeof(BAR))) == NULL) {
	error("bar buffer allocation failed: out of memory");
	return -1;
    }

    Single_Segments = single_segments;

    nSegment = 0;
    fSegment = 0;

    drv_generic_text_bar_clear();

    return 0;
}


void drv_generic_text_bar_add_segment(const int val1, const int val2, const DIRECTION dir, const int ascii)
{
    Segment[fSegment].val1 = val1;
    Segment[fSegment].val2 = val2;
    Segment[fSegment].dir = dir;
    if (val1 == 0 && val2 == 0)
	Segment[fSegment].style = 0;
    else
	Segment[fSegment].style = 255;
    Segment[fSegment].used = 0;
    Segment[fSegment].ascii = ascii;

    fSegment++;
    nSegment = fSegment;
}


static void drv_generic_text_bar_create_bar(int row, int col, const DIRECTION dir, STYLE style, int len, int val1,
					    int val2)
{
    int rev = 0, max;
    if (style)
	BarFB[row * LCOLS + col].style = STYLE_FIRST;

    switch (dir) {
    case DIR_WEST:
	max = len * XRES;
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_EAST:
	while (len > 0 && col < LCOLS) {
	    BarFB[row * LCOLS + col].dir = dir;
	    BarFB[row * LCOLS + col].segment = -1;
	    if (style && BarFB[row * LCOLS + col].style == 0)
		BarFB[row * LCOLS + col].style = style;
	    if (val1 >= XRES) {
		BarFB[row * LCOLS + col].val1 = rev ? 0 : XRES;
		val1 -= XRES;
	    } else {
		BarFB[row * LCOLS + col].val1 = rev ? XRES - val1 : val1;
		val1 = 0;
	    }
	    if (val2 >= XRES) {
		BarFB[row * LCOLS + col].val2 = rev ? 0 : XRES;
		val2 -= XRES;
	    } else {
		BarFB[row * LCOLS + col].val2 = rev ? XRES - val2 : val2;
		val2 = 0;
	    }
	    len--;
	    col++;
	}
	if (style)
	    BarFB[row * LCOLS + col - 1].style = STYLE_LAST;
	break;

    case DIR_NORTH:
	max = len * YRES;
	val1 = max - val1;
	val2 = max - val2;
	rev = 1;

    case DIR_SOUTH:
	while (len > 0 && row < LROWS) {
	    BarFB[row * LCOLS + col].dir = dir;
	    BarFB[row * LCOLS + col].segment = -1;
	    if (val1 >= YRES) {
		BarFB[row * LCOLS + col].val1 = rev ? 0 : YRES;
		val1 -= YRES;
	    } else {
		BarFB[row * LCOLS + col].val1 = rev ? YRES - val1 : val1;
		val1 = 0;
	    }
	    if (val2 >= YRES) {
		BarFB[row * LCOLS + col].val2 = rev ? 0 : YRES;
		val2 -= YRES;
	    } else {
		BarFB[row * LCOLS + col].val2 = rev ? YRES - val2 : val2;
		val2 = 0;
	    }
	    len--;
	    row++;
	}
	break;
    }
}


static void drv_generic_text_bar_create_segments(void)
{
    int i, j, n;
    int res, l1, l2;

    /* find first unused segment */
    for (i = fSegment; i < nSegment && Segment[i].used; i++);

    /* pack unused segments */
    for (j = i + 1; j < nSegment; j++) {
	if (Segment[j].used)
	    Segment[i++] = Segment[j];
    }
    nSegment = i;

    /* create needed segments */
    for (n = 0; n < LROWS * LCOLS; n++) {
	if (BarFB[n].dir == 0)
	    continue;
	res = BarFB[n].dir & (DIR_EAST | DIR_WEST) ? XRES : YRES;
	for (i = 0; i < nSegment; i++) {
	    l1 = Segment[i].val1;
	    if (l1 > res)
		l1 = res;
	    l2 = Segment[i].val2;
	    if (l2 > res)
		l2 = res;

	    /* same value */
	    if (l1 == BarFB[n].val1 && l2 == BarFB[n].val2) {
		/* empty block, only style is interesting */
		if (l1 == 0 && l2 == 0 && Segment[i].style == BarFB[n].style)
		    break;
		/* full block, style doesn't matter */
		if (l1 == res && l2 == res)
		    break;
		/* half upper block */
		if (l1 == res && l2 == 0 && Segment[i].style == BarFB[n].style)
		    break;
		/* half lower block */
		if (l1 == 0 && l2 == res && Segment[i].style == BarFB[n].style)
		    break;
		/* same style, same direction */
		if (Segment[i].style == BarFB[n].style && Segment[i].dir & BarFB[n].dir)
		    break;
#if 0
		/* hollow style, val(1,2) == 1, like '[' */
		if (l1 == 1 && l2 == 1 && Segment[i].style == STYLE_FIRST && BarFB[n].style == STYLE_HOLLOW)
		    break;
		/* hollow style, val(1,2) == 1, like ']' */
		if (l1 == 1 && l2 == 1 && Segment[i].style == STYLE_LAST && BarFB[n].style == STYLE_HOLLOW)
		    break;
#endif
	    }
	}
	if (i == nSegment) {
	    nSegment++;
	    Segment[i].val1 = BarFB[n].val1;
	    Segment[i].val2 = BarFB[n].val2;
	    Segment[i].dir = BarFB[n].dir;
	    Segment[i].style = BarFB[n].style;
	    Segment[i].used = 0;
	    Segment[i].ascii = -1;
	}
	BarFB[n].segment = i;
    }
}


static int drv_generic_text_bar_segment_error(const int i, const int j)
{
    int res;
    int i1, i2, j1, j2;

    if (i == j)
	return 65535;
    if (!(Segment[i].dir & Segment[j].dir))
	return 65535;
    if (Segment[i].style != Segment[j].style)
	return 65535;

    res = Segment[i].dir & (DIR_EAST | DIR_WEST) ? XRES : YRES;

    i1 = Segment[i].val1;
    if (i1 > res)
	i1 = res;
    i2 = Segment[i].val2;
    if (i2 > res)
	i2 = res;
    j1 = Segment[j].val1;
    if (j1 > res)
	j1 = res;
    j2 = Segment[j].val2;
    if (j2 > res)
	j2 = res;

    /* do not replace empty with non-empty */
    if (i1 == 0 && j1 != 0)
	return 65535;
    if (i2 == 0 && j2 != 0)
	return 65535;

    /* do not replace full with non-full */
    if (i1 == res && j1 < res)
	return 65535;
    if (i2 == res && j2 < res)
	return 65535;

    /* do not replace start line */
    /* but only if there are at least some chars available */
    if (CHARS - ICONS > 0) {
	if (i1 == 1 && j1 != 1 && i2 > 0)
	    return 65535;
	if (i2 == 1 && j2 != 1 && j1 > 0)
	    return 65535;
    }

    /* do not replace equal length with non-equal length */
    if (i1 == i2 && j1 != j2)
	return 65535;

    return (i1 - j1) * (i1 - j1) + (i2 - j2) * (i2 - j2);
}


static void drv_generic_text_bar_pack_segments(void)
{
    int i, j, n, min;
    int pack_i, pack_j;
    int pass1 = 1;
    int error[nSegment][nSegment];

    if (nSegment <= fSegment + CHARS - ICONS) {
	return;
    }

    for (i = 0; i < nSegment; i++) {
	for (j = 0; j < nSegment; j++) {
	    error[i][j] = drv_generic_text_bar_segment_error(i, j);
	    // debug ("[%d][%d] = %d", i, j, error[i][j]);
	}
    }

    while (nSegment > fSegment + CHARS - ICONS) {

	min = 65535;
	pack_i = -1;
	pack_j = -1;
	for (i = fSegment; i < nSegment; i++) {
	    if (pass1 && Segment[i].used)
		continue;
	    for (j = 0; j < nSegment; j++) {
		if (error[i][j] < min) {
		    min = error[i][j];
		    pack_i = i;
		    pack_j = j;
		}
	    }
	}
	if (pack_i == -1) {
	    if (pass1) {
		pass1 = 0;
		continue;
	    } else {
		error("unable to compact bar characters");
		error("nSegment=%d fSegment=%d CHARS=%d ICONS=%d", nSegment, fSegment, CHARS, ICONS);
		error("Segment[0].val1=%d val2=%d", Segment[0].val1, Segment[0].val2);
		error("Segment[1].val1=%d val2=%d", Segment[1].val1, Segment[1].val2);
		error("Segment[2].val1=%d val2=%d", Segment[2].val1, Segment[2].val2);
		nSegment = CHARS - ICONS;
		break;
	    }
	}
#if 0
	debug("pack_segment: n=%d i=%d j=%d min=%d", nSegment, pack_i, pack_j, min);
	debug("Pack_segment: i1=%d i2=%d j1=%d j2=%d\n", Segment[pack_i].val1, Segment[pack_i].val2,
	      Segment[pack_j].val1, Segment[pack_j].val2);
#endif

	nSegment--;
	Segment[pack_i] = Segment[nSegment];

	for (i = 0; i < nSegment; i++) {
	    error[pack_i][i] = error[nSegment][i];
	    error[i][pack_i] = error[i][nSegment];
	}

	for (n = 0; n < LROWS * LCOLS; n++) {
	    if (BarFB[n].segment == pack_i)
		BarFB[n].segment = pack_j;
	    if (BarFB[n].segment == nSegment)
		BarFB[n].segment = pack_i;
	}
    }
}


static void drv_generic_text_bar_define_chars(void)
{
    int c, i, j;
    unsigned char buffer[8];

    for (i = fSegment; i < nSegment; i++) {
	if (Segment[i].used)
	    continue;
	if (Segment[i].ascii != -1)
	    continue;
	for (c = 0; c < CHARS - ICONS; c++) {
	    for (j = fSegment; j < nSegment; j++) {
		if (Segment[j].ascii == c)
		    break;
	    }
	    if (j == nSegment)
		break;
	}
	Segment[i].ascii = c;
	switch (Segment[i].dir) {
	case DIR_WEST:
	    if (Segment[i].style) {
		buffer[0] = 255;
		buffer[7] = 255;
		if (Segment[i].style & (STYLE_FIRST | STYLE_LAST))
		    for (j = 1; j < 7; j++) {
			buffer[j] |= Segment[i].style & STYLE_FIRST ? 16 : 1;
		    }
	    }
	    break;
	case DIR_EAST:
	    for (j = 0; j < 4; j++) {
		buffer[j] = 255 << (XRES - Segment[i].val1);
		buffer[j + 4] = 255 << (XRES - Segment[i].val2);
	    }
	    if (Segment[i].style) {
		buffer[0] = 255;
		buffer[7] = 255;
		if (Segment[i].style & (STYLE_FIRST | STYLE_LAST))
		    for (j = 1; j < 7; j++) {
			buffer[j] |= Segment[i].style & STYLE_FIRST ? 16 : 1;
		    }
	    }
	    break;
	case DIR_NORTH:
	    for (j = 0; j < Segment[i].val1; j++) {
		buffer[7 - j] = (1 << XRES) - 1;
	    }
	    for (; j < YRES; j++) {
		buffer[7 - j] = 0;
	    }
	    break;
	case DIR_SOUTH:
	    for (j = 0; j < Segment[i].val1; j++) {
		buffer[j] = (1 << XRES) - 1;
	    }
	    for (; j < YRES; j++) {
		buffer[j] = 0;
	    }
	    break;
	}
	if (drv_generic_text_real_defchar)
	    drv_generic_text_real_defchar(CHAR0 + c, buffer);

	/* maybe invalidate framebuffer */
	if (INVALIDATE) {
	    for (j = 0; j < LROWS * LCOLS; j++) {
		if (BarFB[j].segment == i) {
		    BarFB[j].invalid = 1;
		}
	    }
	}
    }
}


int drv_generic_text_bar_draw(WIDGET * W)
{
    WIDGET_BAR *Bar = W->data;
    int row, col, len, res, max, val1, val2;
    int c, n, s;
    DIRECTION dir;
    STYLE style;

    row = W->row;
    col = W->col;
    dir = Bar->direction;
    style = Bar->style;
    len = Bar->length;

    /* maybe grow layout framebuffer */
    /* bars *always* grow heading North or East! */
    if (dir & (DIR_EAST | DIR_WEST)) {
	drv_generic_text_resizeFB(row + 1, col + len);
    } else {
	drv_generic_text_resizeFB(row + 1, col + 1);
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

    if (Single_Segments)
	val2 = val1;

    /* create this bar */
    drv_generic_text_bar_create_bar(row, col, dir, style, len, val1, val2);

    /* process all bars */
    drv_generic_text_bar_create_segments();
    drv_generic_text_bar_pack_segments();
    drv_generic_text_bar_define_chars();

    /* reset usage flags */
    for (s = 0; s < nSegment; s++) {
	Segment[s].used = 0;
    }

    /* set usage flags */
    for (n = 0; n < LROWS * LCOLS; n++) {
	if ((s = BarFB[n].segment) != -1)
	    Segment[s].used = 1;
    }

    /* transfer bars into layout buffer */
    for (row = 0; row < LROWS; row++) {
	for (col = 0; col < LCOLS; col++) {
	    n = row * LCOLS + col;
	    s = BarFB[n].segment;
	    if (s == -1)
		continue;
	    c = Segment[s].ascii;
	    if (c == -1)
		continue;
	    if (s >= fSegment)
		c += CHAR0;	/* ascii offset for user-defineable chars */
	    LayoutFB[n] = c;
	    /* maybe invalidate display framebuffer */
	    if (BarFB[n].invalid) {
		BarFB[n].invalid = 0;
		/* ugly invalidation: change display FB to a wrong value so blit() will really send it */
		DisplayFB[row * DCOLS + col] = ~LayoutFB[n];
	    }
	}
    }

    /* blit whole layout FB */
    drv_generic_text_blit(0, 0, LROWS, LCOLS);


    return 0;

}
