/* $Id: drv.c,v 1.3 2004/01/20 15:32:49 reinelt Exp $
 *
 * new framework for display drivers
 *
 * Copyright 1999-2003 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
extern DRIVER drv_Crystalfontz;
extern DRIVER drv_Cwlinux;
extern DRIVER drv_HD44780;
extern DRIVER drv_M50530;
extern DRIVER drv_T6963;
extern DRIVER drv_USBDRIVER;
extern DRIVER drv_MatrixOrbital;
extern DRIVER drv_MilfordInstruments;
extern DRIVER drv_PalmPilot;
extern DRIVER drv_Raster;
extern DRIVER drv_XWindow;
extern DRIVER drv_Text;

// output file for Raster driver
// has to be defined here because it's referenced
// even if the raster driver is not included!
char *output=NULL;

DRIVER *Driver[] = {
#ifdef WITH_MATRIXORBITAL
  &drv_MatrixOrbital,
#endif
  /* Fixme
     #ifdef WITH_BECKMANNEGLE
     &BeckmannEgle,
     #endif
     #ifdef WITH_CRYSTALFONTZ
     &Crystalfontz,
     #endif
     #ifdef WITH_CWLINUX
     &Cwlinux,
     #endif
  */
#ifdef WITH_HD44780
  &drv_HD44780,
#endif
  /* Fixme
     #ifdef WITH_M50530
     &M50530,
     #endif
     #ifdef WITH_T6963
     &T6963,
     #endif
     #ifdef WITH_USBLCD
     &USBLCD,
     #endif
     #ifdef WITH_MATRIXORBITAL
     &MatrixOrbital,
     #endif
     #ifdef WITH_MILINST
     &MilfordInstruments,
     #endif
     #ifdef WITH_PALMPILOT
     &PalmPilot,
     #endif
     #if defined (WITH_PNG) || defined(WITH_PPM)
     &Raster,
     #endif
     #ifdef WITH_X11
     &XWindow,
     #endif
     #ifdef WITH_TEXT
     &Text,
     #endif
  */
  NULL,
};


static DRIVER *Drv = NULL;


int drv_list (void)
{
  int i;

  printf ("available display drivers:");
  
  for (i=0; Driver[i]; i++) {
    printf ("\n   %-20s: ", Driver[i]->name);
    if (Driver[i]->list) Driver[i]->list();
  }
  printf ("\n");
  return 0;
}


int drv_init (char *section, char *driver)
{
  int i;
  for (i=0; Driver[i]; i++) {
    if (strcmp (Driver[i]->name, driver)==0) {
      Drv=Driver[i];
      if (Drv->init==NULL) return 0;
      return Drv->init(section);
    }
  }
  error ("drv_init(%s) failed: no such driver", driver);
  return -1;
}


int drv_quit (void)
{
  if (Drv->quit==NULL) return 0;
  return Drv->quit();
}
