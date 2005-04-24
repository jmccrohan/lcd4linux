/* $Id: drv.c,v 1.29 2005/04/24 04:33:46 reinelt Exp $
 *
 * new framework for display drivers
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
 *
 * $Log: drv.c,v $
 * Revision 1.29  2005/04/24 04:33:46  reinelt
 * driver for TREFON USB LCD's added
 *
 * Revision 1.28  2005/02/24 07:06:48  reinelt
 * SimpleLCD driver added
 *
 * Revision 1.27  2005/01/30 06:43:22  reinelt
 * driver for LCD-Linux finished
 *
 * Revision 1.26  2005/01/22 22:57:57  reinelt
 * LCD-Linux driver added
 *
 * Revision 1.25  2005/01/18 06:30:22  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.24  2005/01/15 13:10:15  reinelt
 * LCDTerm driver added
 *
 * Revision 1.23  2004/09/24 21:41:00  reinelt
 * new driver for the BWCT USB LCD interface board.
 *
 * Revision 1.22  2004/08/29 13:03:41  reinelt
 *
 * added RouterBoard driver
 *
 * Revision 1.21  2004/06/26 12:04:59  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.20  2004/06/26 09:27:20  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.19  2004/06/20 10:09:54  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.18  2004/06/06 06:51:59  reinelt
 *
 * do not display end splash screen if quiet=1
 *
 * Revision 1.17  2004/06/02 10:09:22  reinelt
 *
 * splash screen for HD44780
 *
 * Revision 1.16  2004/06/02 09:41:19  reinelt
 *
 * prepared support for startup splash screen
 *
 * Revision 1.15  2004/05/31 16:39:06  reinelt
 *
 * added NULL display driver (for debugging/profiling purposes)
 * added backlight/contrast initialisation for matrixOrbital
 * added Backlight initialisation for Cwlinux
 *
 * Revision 1.14  2004/05/28 13:51:42  reinelt
 *
 * ported driver for Beckmann+Egle Mini-Terminals
 * added 'flags' parameter to serial_init()
 *
 * Revision 1.13  2004/05/26 11:37:36  reinelt
 *
 * Curses driver ported.
 *
 * Revision 1.12  2004/05/26 05:03:27  reinelt
 *
 * MilfordInstruments driver ported
 *
 * Revision 1.11  2004/05/25 19:54:15  reinelt
 *
 * 'make distcheck' bugs fixed
 * release number changed to 0.10.0-RC1
 *
 * Revision 1.10  2004/05/25 14:26:29  reinelt
 *
 * added "Image" driver (was: Raster.c) for PPM and PNG creation
 * fixed some glitches in the X11 driver
 *
 * Revision 1.9  2004/02/24 05:55:04  reinelt
 *
 * X11 driver ported
 *
 * Revision 1.8  2004/02/15 21:43:43  reinelt
 * T6963 driver nearly finished
 * framework for graphic displays done
 * i2c_sensors patch from Xavier
 * some more old generation files removed
 *
 * Revision 1.7  2004/02/15 08:22:47  reinelt
 * ported USBLCD driver to NextGeneration
 * added drv_M50530.c (I forgot yesterday, sorry)
 * removed old drivers M50530.c and USBLCD.c
 *
 * Revision 1.6  2004/02/14 11:56:17  reinelt
 * M50530 driver ported
 * changed lots of 'char' to 'unsigned char'
 *
 * Revision 1.5  2004/01/27 06:34:14  reinelt
 * Cwlinux driver portet to NextGeneration (compiles, but not tested!)
 *
 * Revision 1.4  2004/01/21 12:36:19  reinelt
 * Crystalfontz NextGeneration driver added
 *
 * Revision 1.3  2004/01/20 15:32:49  reinelt
 * first version of Next Generation HD44780 (untested! but it compiles...)
 * some cleanup in the other drivers
 *
 * Revision 1.2  2004/01/10 10:20:22  reinelt
 * new MatrixOrbital changes
 *
 * Revision 1.1  2004/01/09 17:03:07  reinelt
 * initiated transfer to new driver architecture
 * new file 'drv.c' will someday replace 'display.c'
 * new file 'drv_MatrixOrbital.c' will replace 'MatrixOrbital.c'
 * due to this 'soft' transfer lcd4linux should stay usable during the switch
 * (at least I hope so)
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
 * drv_query (int *rows, int *cols, int *xres, int *yres, int *bars, int *gpos)
 *    queries the attributes of the selected driver
 *
 * drv_clear ()
 *    clears the display
 *
 * int drv_put (int row, int col, char *text)
 *    writes text at row, col
 *
 * int drv_bar (int type, int row, int col, int max, int len1, int len2)
 *    draws a specified bar at row, col with len
 *
 * int drv_icon (int num, int seq, int row, int col)
 *    draws icon #num sequence #seq at row, col
 *
 * int drv_gpo (int num, int val)
 *    sets GPO #num to val
 *
 * int drv_flush (void)
 *    flushes the framebuffer to the display
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
extern DRIVER drv_HD44780;
extern DRIVER drv_Image;
extern DRIVER drv_LCDLinux;
extern DRIVER drv_LCDTerm;
extern DRIVER drv_M50530;
extern DRIVER drv_MatrixOrbital;
extern DRIVER drv_MilfordInstruments;
extern DRIVER drv_NULL;
extern DRIVER drv_RouterBoard;
extern DRIVER drv_SimpleLCD;
extern DRIVER drv_T6963;
extern DRIVER drv_Trefon;
extern DRIVER drv_USBLCD;
extern DRIVER drv_X11;

/* output file for Image driver
 * has to be defined here because it's referenced
 * even if the raster driver is not included!
 */
char *output=NULL;

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
#ifdef WITH_CWLINUX
  &drv_Cwlinux,
#endif
#ifdef WITH_CURSES
  &drv_Curses,
#endif
#ifdef WITH_HD44780
  &drv_HD44780,
#endif
#if defined (WITH_PNG) || defined(WITH_PPM)
  &drv_Image,
#endif
#ifdef WITH_LCDLINUX
  &drv_LCDLinux,
#endif
#ifdef WITH_LCDTERM
  &drv_LCDTerm,
#endif
#ifdef WITH_M50530
  &drv_M50530,
#endif
#ifdef WITH_MATRIXORBITAL
  &drv_MatrixOrbital,
#endif
#ifdef WITH_MILINST
  &drv_MilfordInstruments,
#endif
#ifdef WITH_NULL
  &drv_NULL,
#endif
#ifdef WITH_ROUTERBOARD
  &drv_RouterBoard,
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
#ifdef WITH_USBLCD
  &drv_USBLCD,
#endif
#ifdef WITH_X11
  &drv_X11,
#endif

  NULL,
};


static DRIVER *Drv = NULL;


int drv_list (void)
{
  int i;

  printf ("available display drivers:");
  
  for (i = 0; Driver[i]; i++) {
    printf ("\n   %-20s: ", Driver[i]->name);
    if (Driver[i]->list) Driver[i]->list();
  }
  printf ("\n");
  return 0;
}


int drv_init (const char *section, const char *driver, const int quiet)
{
  int i;
  for (i = 0; Driver[i]; i++) {
    if (strcmp (Driver[i]->name, driver) == 0) {
      Drv = Driver[i];
      if (Drv->init == NULL) return 0;
      return Drv->init(section, quiet);
    }
  }
  error ("drv_init(%s) failed: no such driver", driver);
  return -1;
}


int drv_quit (const int quiet)
{
  if (Drv->quit == NULL) return 0;
  return Drv->quit(quiet);
}
