/* $Id$
 * $URL$
 *
 * Libvncserver driver
 * 
 * Copyright (C) 2009 Michael Vogt <michu@neophob.com>
 * Modified from sample code by:
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * struct DRIVER drv_vnc
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <rfb/rfb.h> 

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"

#include "drv_generic_graphic.h"



/* 15 frames per second (if we can) */
#define PICTURE_TIMEOUT (1.0/15.0) 

static char Name[] = "VNC";

static rfbScreenInfoPtr server;
static int xres = 320;
static int yres = 200;
static int BPP = 4;


static int drv_vnc_open(const char *Section)
{
    if (cfg_number(Section, "xres", 320, 32, 2048, &xres) < 1) {
        info("[DRV_VNC] no '%s.xres' entry from %s using default %d", Section, cfg_source(), xres);
    }                    
    if (cfg_number(Section, "yres", 200, 32, 2048, &yres) < 1) {
        info("[DRV_VNC] no '%s.yres' entry from %s using default %d", Section, cfg_source(), yres);
    }
    if (cfg_number(Section, "bpp", 4, 1, 4, &BPP) < 1) {
        info("[DRV_VNC] no '%s.bpp' entry from %s using default %d", Section, cfg_source(), BPP);
    }
    return 0;
}


static int drv_vnc_close(void)
{
    rfbShutdownServer(server, TRUE);
    free(server->frameBuffer);
    return 0;
}

static void drv_vnc_blit_it(const int row, const int col, const int height, const int width, unsigned char* buffer)
{
    int r, c, ofs;
    RGBA p;
    
    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    p = drv_generic_graphic_rgb(r, c);
	    ofs = (r * xres + c) * BPP;
	    buffer[ofs++] = p.R;
	    buffer[ofs++] = p.G;
	    buffer[ofs++] = p.B;
	    buffer[ofs  ] = 255;
	}
    }
}

static void drv_vnc_blit(const int row, const int col, const int height, const int width)
{
    if (rfbIsActive(server)) {
    //todo blit only if client are connected...
	drv_vnc_blit_it(row, col, height, width, (unsigned char *)server->frameBuffer);
        rfbMarkRectAsModified(server, 0, 0, xres, yres);
        rfbProcessEvents(server, server->deferUpdateTime*1000);
    }
}

static void doptr(int buttonMask,int x,int y,rfbClientPtr cl)
{
    //printf("doptr\n");
//    ClientData* cd=cl->clientData;
    
    if(x>=0 && y>=0 && x<xres && y<yres) {
	if(buttonMask) {
		printf("btn:%d, x:%d, y:%d\n", buttonMask, x, y);
	}
    }    
    
    rfbDefaultPtrAddEvent(buttonMask,x,y,cl);
}

static void dokey(rfbBool down,rfbKeySym key,rfbClientPtr cl)
{
    printf("dokey\n");
}

/* start graphic display */
static int drv_vnc_start(const char *section)
{
    char *s;

    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    /* open communication with the display */
    if (drv_vnc_open(section) < 0) {
	return -1;
    }

    /* you surely want to allocate a framebuffer or something... */
    server=rfbGetScreen(0, NULL, xres, yres, 8, 3, BPP);
    server->desktopName = "LCD4Linux VNC Driver";
    server->frameBuffer=(char*)malloc(xres*yres*BPP);
    server->alwaysShared=(1==1);
    server->ptrAddEvent = doptr;
//    server->kbdAddEvent = dokey;
    
    /* Initialize the server */
    rfbInitServer(server);
                
    /* set width/height */
    DROWS = yres;
    DCOLS = xres;

    return 0;
}

/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */
/* using drv_generic_gpio_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_vnc_list(void)
{
    printf("vnc server");
    return 0;
}


/* initialize driver & display */
int drv_vnc_init(const char *section, const int quiet)
{
    int ret;

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_vnc_blit;

    /* start display */
    if ((ret = drv_vnc_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    return 0;
}


/* close driver & display */
int drv_vnc_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    /* clear display */
    drv_generic_graphic_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_generic_graphic_quit();

    debug("closing connection");
    drv_vnc_close();

    return (0);
}


DRIVER drv_vnc = {
    .name = Name,
    .list = drv_vnc_list,
    .init = drv_vnc_init,
    .quit = drv_vnc_quit,
};
