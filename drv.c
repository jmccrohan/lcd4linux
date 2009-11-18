/* $Id$
 * $URL$
 *
 * new framework for display drivers
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
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
 * drv_list (void)
 *   lists all available drivers to stdout
 *
 * drv_init (char *driver)
 *    initializes the named driver
 *
 * int drv_quit (void)
 *    de-initializes the driver
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "cfg.h"
#include "drv.h"

extern DRIVER drv_BeckmannEgle;
extern DRIVER drv_BWCT;
extern DRIVER drv_Crystalfontz;
extern DRIVER drv_Curses;
extern DRIVER drv_Cwlinux;
extern DRIVER drv_D4D;
extern DRIVER drv_EA232graphic;
extern DRIVER drv_G15;
extern DRIVER drv_GLCD2USB;
extern DRIVER drv_HD44780;
extern DRIVER drv_Image;
extern DRIVER drv_IRLCD;
extern DRIVER drv_LCD2USB;
extern DRIVER drv_LCDLinux;
extern DRIVER drv_LCDTerm;
extern DRIVER drv_LEDMatrix;
extern DRIVER drv_LPH7508;
extern DRIVER drv_LUIse;
extern DRIVER drv_LW_ABP;
extern DRIVER drv_M50530;
extern DRIVER drv_MatrixOrbital;
extern DRIVER drv_MatrixOrbitalGX;
extern DRIVER drv_MilfordInstruments;
extern DRIVER drv_Noritake;
extern DRIVER drv_NULL;
extern DRIVER drv_Pertelian;
extern DRIVER drv_PHAnderson;
extern DRIVER drv_PICGraphic;
extern DRIVER drv_picoLCD;
extern DRIVER drv_picoLCDGraphic;
extern DRIVER drv_RouterBoard;
extern DRIVER drv_Sample;
extern DRIVER drv_st2205;
extern DRIVER drv_serdisplib;
extern DRIVER drv_ShuttleVFD;
extern DRIVER drv_SimpleLCD;
extern DRIVER drv_T6963;
extern DRIVER drv_Trefon;
extern DRIVER drv_ula200;
extern DRIVER drv_USBHUB;
extern DRIVER drv_USBLCD;
extern DRIVER drv_vnc;
extern DRIVER drv_WincorNixdorf;
extern DRIVER drv_X11;

/* output file for Image driver
 * has to be defined here because it's referenced
 * even if the raster driver is not included!
 */
char *output = NULL;

DRIVER *Driver[] = {
#ifdef WITH_BECKMANNEGLE
    &drv_BeckmannEgle,
#endif
#ifdef WITH_BWCT
    &drv_BWCT,
#endif
#ifdef WITH_CRYSTALFONTZ
    &drv_Crystalfontz,
#endif
#ifdef WITH_CURSES
    &drv_Curses,
#endif
#ifdef WITH_CWLINUX
    &drv_Cwlinux,
#endif
#ifdef WITH_D4D
    &drv_D4D,
#endif
#ifdef WITH_EA232graphic
    &drv_EA232graphic,
#endif
#ifdef WITH_G15
    &drv_G15,
#endif
#ifdef WITH_GLCD2USB
    &drv_GLCD2USB,
#endif
#ifdef WITH_HD44780
    &drv_HD44780,
#endif
#if (defined(WITH_PNG) && defined(WITH_GD)) || defined(WITH_PPM)
    &drv_Image,
#endif
#ifdef WITH_IRLCD
    &drv_IRLCD,
#endif
#ifdef WITH_LCD2USB
    &drv_LCD2USB,
#endif
#ifdef WITH_LCDLINUX
    &drv_LCDLinux,
#endif
#ifdef WITH_LCDTERM
    &drv_LCDTerm,
#endif
#ifdef WITH_LEDMATRIX
    &drv_LEDMatrix,
#endif
#ifdef WITH_LPH7508
    &drv_LPH7508,
#endif
#ifdef WITH_LUISE
    &drv_LUIse,
#endif
#ifdef WITH_LW_ABP
    &drv_LW_ABP,
#endif
#ifdef WITH_M50530
    &drv_M50530,
#endif
#ifdef WITH_MATRIXORBITAL
    &drv_MatrixOrbital,
#endif
#ifdef WITH_MATRIXORBITALGX
    &drv_MatrixOrbitalGX,
#endif
#ifdef WITH_MILINST
    &drv_MilfordInstruments,
#endif
#ifdef WITH_NORITAKE
    &drv_Noritake,
#endif
#ifdef WITH_NULL
    &drv_NULL,
#endif
#ifdef WITH_PERTELIAN
    &drv_Pertelian,
#endif
#ifdef WITH_PHANDERSON
    &drv_PHAnderson,
#endif
#ifdef WITH_PICGRAPHIC
    &drv_PICGraphic,
#endif
#ifdef WITH_PICOLCD
    &drv_picoLCD,
#endif
#ifdef WITH_PICOLCDGRAPHIC
    &drv_picoLCDGraphic,
#endif
#ifdef WITH_ROUTERBOARD
    &drv_RouterBoard,
#endif
#ifdef WITH_SAMPLE
    &drv_Sample,
#endif
#ifdef WITH_ST2205
    &drv_st2205,
#endif
#ifdef WITH_SHUTTLEVFD
    &drv_ShuttleVFD,
#endif
#ifdef WITH_SERDISPLIB
    &drv_serdisplib,
#endif
#ifdef WITH_SIMPLELCD
    &drv_SimpleLCD,
#endif
#ifdef WITH_T6963
    &drv_T6963,
#endif
#ifdef WITH_TREFON
    &drv_Trefon,
#endif
#ifdef WITH_ULA200
    &drv_ula200,
#endif
#ifdef WITH_USBHUB
    &drv_USBHUB,
#endif
#ifdef WITH_USBLCD
    &drv_USBLCD,
#endif
#ifdef WITH_VNC
    &drv_vnc,
#endif
#ifdef WITH_WINCORNIXDORF
    &drv_WincorNixdorf,
#endif
#ifdef WITH_X11
    &drv_X11,
#endif

    NULL,
};


static DRIVER *Drv = NULL;


/* maybe we need this */
extern int drv_SD_list_verbose(void);


int drv_list(void)
{
    int i;

    printf("available display drivers:");

    for (i = 0; Driver[i]; i++) {
	printf("\n   %-20s: ", Driver[i]->name);
	if (Driver[i]->list)
	    Driver[i]->list();
    }
    printf("\n");

#ifdef WITH_SERDISPLIB
    printf("\n");
    drv_SD_list_verbose();
#endif

    return 0;
}


int drv_init(const char *section, const char *driver, const int quiet)
{
    int i;
    for (i = 0; Driver[i]; i++) {
	if (strcmp(Driver[i]->name, driver) == 0) {
	    Drv = Driver[i];
	    if (Drv->init == NULL)
		return 0;
	    return Drv->init(section, quiet);
	}
    }
    error("drv_init(%s) failed: no such driver", driver);
    return -1;
}


int drv_quit(const int quiet)
{
    if (Drv->quit == NULL)
	return 0;
    return Drv->quit(quiet);
}
