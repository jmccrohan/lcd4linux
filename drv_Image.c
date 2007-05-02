/* $Id$
 * $URL$
 *
 * new style Image (PPM/PNG) Driver for LCD4Linux 
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
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
 * exported fuctions:
 *
 * struct DRIVER drv_Image
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
#include <sys/time.h>


#ifdef WITH_PNG

#ifdef HAVE_GD_GD_H
#include <gd/gd.h>
#else
#ifdef HAVE_GD_H
#include <gd.h>
#else
#error "gd.h not found!"
#error "cannot compile PNG driver"
#endif
#endif

#if GD2_VERS != 2
#error "lcd4linux requires libgd version 2"
#error "cannot compile PNG driver"
#endif

#endif


#include "debug.h"
#include "cfg.h"
#include "timer.h"
#include "qprintf.h"
#include "plugin.h"
#include "drv.h"
#include "drv_generic_graphic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static char Name[] = "Image";

static enum { PPM, PNG } Format;

static int pixel = -1;		/* pointsize in pixel */
static int pgap = 0;		/* gap between points */
static int rgap = 0;		/* row gap between lines */
static int cgap = 0;		/* column gap between characters */
static int border = 0;		/* window border */

static int dimx, dimy;		/* total window dimension in pixel */

static RGBA BC;
static RGBA *drv_IMG_FB = NULL;

static int dirty = 1;

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

#ifdef WITH_PPM
static int drv_IMG_flush_PPM(void)
{
    static int seq = 0;
    static RGBA *bitbuf = NULL;
    static unsigned char *rowbuf = NULL;
    int xsize, ysize, row, col, i;
    char path[256], tmp[256], buffer[256];
    int fd;

    xsize = 2 * border + (DCOLS / XRES - 1) * cgap + DCOLS * pixel + (DCOLS - 1) * pgap;
    ysize = 2 * border + (DROWS / YRES - 1) * rgap + DROWS * pixel + (DROWS - 1) * pgap;

    if (bitbuf == NULL) {
	if ((bitbuf = malloc(xsize * ysize * sizeof(*bitbuf))) == NULL) {
	    error("%s: malloc() failed: %s", Name, strerror(errno));
	    return -1;
	}
    }

    if (rowbuf == NULL) {
	if ((rowbuf = malloc(3 * xsize * sizeof(*rowbuf))) == NULL) {
	    error("Raster: malloc() failed: %s", strerror(errno));
	    return -1;
	}
    }

    for (i = 0; i < xsize * ysize; i++) {
	bitbuf[i] = BC;
    }

    for (row = 0; row < DROWS; row++) {
	int y = border + (row / YRES) * rgap + row * (pixel + pgap);
	for (col = 0; col < DCOLS; col++) {
	    int x = border + (col / XRES) * cgap + col * (pixel + pgap);
	    int a, b;
	    for (a = 0; a < pixel; a++)
		for (b = 0; b < pixel; b++)
		    bitbuf[y * xsize + x + a * xsize + b] = drv_IMG_FB[row * DCOLS + col];
	}
    }

    snprintf(path, sizeof(path), output, seq++);
    qprintf(tmp, sizeof(tmp), "%s.tmp", path);

    /* remove the file */
    unlink(tmp);

    /* avoid symlink security hole:  */
    /* open it with O_EXCL will fail if the file exists.  */
    /* This should not happen because we just unlinked it. */
    if ((fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0) {
	error("%s: open(%s) failed: %s", Name, tmp, strerror(errno));
	return -1;
    }

    qprintf(buffer, sizeof(buffer), "P6\n%d %d\n255\n", xsize, ysize);
    if (write(fd, buffer, strlen(buffer)) < 0) {
	error("%s: write(%s) failed: %s", Name, tmp, strerror(errno));
	return -1;
    }

    for (row = 0; row < ysize; row++) {
	int c = 0;
	for (col = 0; col < xsize; col++) {
	    RGBA p = bitbuf[row * xsize + col];
	    rowbuf[c++] = p.R;
	    rowbuf[c++] = p.G;
	    rowbuf[c++] = p.B;
	}
	if (write(fd, rowbuf, c) < 0) {
	    error("%s: write(%s) failed: %s", Name, tmp, strerror(errno));
	    break;
	}
    }

    if (close(fd) < 0) {
	error("%s: close(%s) failed: %s", Name, tmp, strerror(errno));
	return -1;
    }
    if (rename(tmp, path) < 0) {
	error("%s: rename(%s) failed: %s", Name, tmp, strerror(errno));
	return -1;
    }

    return 0;
}
#endif

#ifdef WITH_PNG
static int drv_IMG_flush_PNG(void)
{
    static int seq = 0;
    int xsize, ysize, row, col;
    char path[256], tmp[256];
    FILE *fp;
    int fd;
    gdImagePtr im;

    xsize = 2 * border + (DCOLS / XRES - 1) * cgap + DCOLS * pixel + (DCOLS - 1) * pgap;
    ysize = 2 * border + (DROWS / YRES - 1) * rgap + DROWS * pixel + (DROWS - 1) * pgap;

    im = gdImageCreateTrueColor(xsize, ysize);
    gdImageFilledRectangle(im, 0, 0, xsize, ysize, gdTrueColor(BC.R, BC.G, BC.B));

    for (row = 0; row < DROWS; row++) {
	int y = border + (row / YRES) * rgap + row * (pixel + pgap);
	for (col = 0; col < DCOLS; col++) {
	    int x = border + (col / XRES) * cgap + col * (pixel + pgap);
	    RGBA p = drv_IMG_FB[row * DCOLS + col];
	    int c = gdTrueColor(p.R, p.G, p.B);
	    gdImageFilledRectangle(im, x, y, x + pixel - 1, y + pixel - 1, c);
	}
    }

    snprintf(path, sizeof(path), output, seq++);
    qprintf(tmp, sizeof(tmp), "%s.tmp", path);

    /* remove the file */
    unlink(tmp);

    /* avoid symlink security hole:  */
    /* open it with O_EXCL will fail if the file exists.  */
    /* This should not happen because we just unlinked it. */
    if ((fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0) {
	error("%s: open(%s) failed: %s", Name, tmp, strerror(errno));
	return -1;
    }

    if ((fp = fdopen(fd, "w")) == NULL) {
	error("%s: fdopen(%s) failed: %s\n", Name, tmp, strerror(errno));
	close(fd);
	return -1;
    }

    gdImagePng(im, fp);
    gdImageDestroy(im);


    if (fclose(fp) != 0) {
	error("%s: fclose(%s) failed: %s\n", Name, tmp, strerror(errno));
	return -1;
    }

    if (rename(tmp, path) < 0) {
	error("%s: rename(%s) failed: %s\n", Name, tmp, strerror(errno));
	return -1;
    }

    return 0;
}
#endif


static void drv_IMG_flush(void)
{
    switch (Format) {
    case PPM:
#ifdef WITH_PPM
	drv_IMG_flush_PPM();
#endif
	break;
    case PNG:
#ifdef WITH_PNG
	drv_IMG_flush_PNG();
#endif
	break;
    }
}


static void drv_IMG_timer( __attribute__ ((unused))
			  void *notused)
{
    if (dirty) {
	drv_IMG_flush();
	dirty = 0;
    }
}


static void drv_IMG_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    RGBA p1 = drv_IMG_FB[r * DCOLS + c];
	    RGBA p2 = drv_generic_graphic_rgb(r, c);
	    if (p1.R != p2.R || p1.G != p2.G || p1.B != p2.B) {
		drv_IMG_FB[r * DCOLS + c] = p2;
		dirty = 1;
	    }
	}
    }
}


static int drv_IMG_start(const char *section)
{
    int i;
    char *s;

    if (output == NULL || *output == '\0') {
	error("%s: no output file specified (use -o switch)", Name);
	return -1;
    }

    /* read file format from config */
    s = cfg_get(section, "Format", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Format' entry from %s", Name, section, cfg_source());
	free(s);
	return -1;
    }

    if (strcmp(s, "PPM") == 0) {
	Format = PPM;
    } else if (strcmp(s, "PNG") == 0) {
	Format = PNG;
    } else {
	error("%s: bad %s.Format '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    /* read display size from config */
    if (sscanf(s = cfg_get(section, "Size", "120x32"), "%dx%d", &DCOLS, &DROWS) != 2 || DCOLS < 1 || DROWS < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "font", "5x8"), "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad %s.Font '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "pixel", "4+1"), "%d+%d", &pixel, &pgap) != 2 || pixel < 1 || pgap < 0) {
	error("%s: bad %s.Pixel '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (sscanf(s = cfg_get(section, "gap", "-1x-1"), "%dx%d", &cgap, &rgap) != 2 || cgap < -1 || rgap < -1) {
	error("%s: bad %s.Gap '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }
    free(s);

    if (rgap < 0)
	rgap = pixel + pgap;
    if (cgap < 0)
	cgap = pixel + pgap;

    if (cfg_number(section, "border", 0, 0, -1, &border) < 0)
	return -1;

    s = cfg_get(section, "basecolor", "000000ff");
    if (color2RGBA(s, &BC) < 0) {
	error("%s: ignoring illegal color '%s'", Name, s);
    }
    free(s);

    drv_IMG_FB = malloc(DCOLS * DROWS * sizeof(*drv_IMG_FB));
    if (drv_IMG_FB == NULL) {
	error("%s: framebuffer could not be allocated: malloc() failed", Name);
	return -1;
    }

    for (i = 0; i < DCOLS * DROWS; i++) {
	drv_IMG_FB[i] = BC;
    }

    dimx = DCOLS * pixel + (DCOLS - 1) * pgap + (DCOLS / XRES - 1) * cgap;
    dimy = DROWS * pixel + (DROWS - 1) * pgap + (DROWS / YRES - 1) * rgap;


    /* initially flush the image to a file */
    drv_IMG_flush();

    /* regularly flush the image to a file */
    /* Fixme: make 100msec configurable */
    timer_add(drv_IMG_timer, NULL, 100, 0);


    return 0;
}



/****************************************/
/***            plugins               ***/
/****************************************/

/* none at the moment... */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_IMG_list(void)
{
    printf("PPM PNG");
    return 0;
}


/* initialize driver & display */
int drv_IMG_init(const char *section, const __attribute__ ((unused))
		 int quiet)
{
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_IMG_blit;

    /* start display */
    if ((ret = drv_IMG_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    /* register plugins */
    /* none at the moment... */


    return 0;
}


/* close driver & display */
int drv_IMG_quit(const __attribute__ ((unused))
		 int quiet)
{

    info("%s: shutting down.", Name);
    drv_generic_graphic_quit();

    if (drv_IMG_FB) {
	free(drv_IMG_FB);
	drv_IMG_FB = NULL;
    }

    return (0);
}


DRIVER drv_Image = {
    .name = Name,
    .list = drv_IMG_list,
    .init = drv_IMG_init,
    .quit = drv_IMG_quit,
};
