/* $Id: display.c,v 1.33 2002/09/11 05:16:33 reinelt Exp $
 *
 * framework for device drivers
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: display.c,v $
 * Revision 1.33  2002/09/11 05:16:33  reinelt
 * added Cwlinux driver
 *
 * Revision 1.32  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.31  2002/08/17 13:10:23  reinelt
 * USBLCD driver added
 *
 * Revision 1.30  2002/04/29 11:00:28  reinelt
 *
 * added Toshiba T6963 driver
 * added ndelay() with nanosecond resolution
 *
 * Revision 1.29  2001/09/10 13:55:53  reinelt
 * M50530 driver
 *
 * Revision 1.28  2001/03/16 16:40:17  ltoetsch
 * implemented time bar
 *
 * Revision 1.27  2001/03/15 14:25:05  ltoetsch
 * added unread/total news
 *
 * Revision 1.26  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.25  2001/03/09 13:08:11  ltoetsch
 * Added Text driver
 *
 * Revision 1.24  2001/03/01 11:08:16  reinelt
 *
 * reworked configure to allow selection of drivers
 *
 * Revision 1.23  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.22  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.21  2000/11/28 16:46:11  reinelt
 *
 * first try to support display of SIN router
 *
 * Revision 1.20  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.19  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.18  2000/05/02 23:07:48  herp
 * Crystalfontz initial coding
 *
 * Revision 1.17  2000/05/02 06:05:00  reinelt
 *
 * driver for 3Com Palm Pilot added
 *
 * Revision 1.16  2000/04/28 05:19:55  reinelt
 *
 * first release of the Beckmann+Egle driver
 *
 * Revision 1.15  2000/04/12 08:05:45  reinelt
 *
 * first version of the HD44780 driver
 *
 * Revision 1.14  2000/04/03 17:31:52  reinelt
 *
 * suppress welcome message if display is smaller than 20x2
 * change lcd4linux.ppm to 32 pixel high so KDE won't stretch the icon
 *
 * Revision 1.13  2000/03/30 16:46:57  reinelt
 *
 * configure now handles '--with-x' and '--without-x' correct
 *
 * Revision 1.12  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.11  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.10  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.9  2000/03/22 15:36:21  reinelt
 *
 * added '-l' switch (list drivers)
 * generic pixmap driver added
 * X11 Framework done
 *
 * Revision 1.8  2000/03/19 08:41:28  reinelt
 *
 * documentation available! README, README.MatrixOrbital, README.Drivers
 * added Skeleton.c as a starting point for new drivers
 *
 * Revision 1.7  2000/03/18 08:07:04  reinelt
 *
 * vertical bars implemented
 * bar compaction improved
 * memory information implemented
 *
 * Revision 1.6  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.5  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.4  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 * Revision 1.3  2000/03/10 10:49:53  reinelt
 *
 * MatrixOrbital driver finished
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 */

/* 
 * exported functions:
 *
 * lcd_list (void)
 *   lists all available drivers to stdout
 *
 * lcd_init (char *driver)
 *    initializes the named driver
 *
 * lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars, int *gpos)
 *    queries the attributes of the selected driver
 *
 * lcd_clear ()
 *    clears the display
 *
 * int lcd_put (int row, int col, char *text)
 *    writes text at row, col
 *
 * int lcd_bar (int type, int row, int col, int max, int len1, int len2)
 *    draws a specified bar at row, col with len
 *
 * int lcd_gpo (int num, int val)
 *    sets GPO #num to val
 *
 * int lcd_flush (void)
 *    flushes the framebuffer to the display
 *
 * int lcd_quit (void)
 *    de-initializes the driver
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"

extern LCD BeckmannEgle[];
extern LCD Crystalfontz[];
extern LCD Cwlinux[];
extern LCD HD44780[];
extern LCD M50530[];
extern LCD T6963[];
extern LCD USBLCD[];
extern LCD MatrixOrbital[];
extern LCD PalmPilot[];
extern LCD Raster[];
extern LCD SIN[];
extern LCD Skeleton[];
extern LCD XWindow[];
extern LCD Text[];

FAMILY Driver[] = {
#ifdef WITH_BECKMANNEGLE
  { "Beckmann+Egle", BeckmannEgle },
#endif
#ifdef WITH_CRYSTALFONTZ
  { "Crystalfontz", Crystalfontz },
#endif
#ifdef WITH_CWLINUX
  { "Cwlinux", Cwlinux },
#endif
#ifdef WITH_HD44780
  { "HD 44780 based", HD44780 },
#endif
#ifdef WITH_M50530
  { "M50530 based", M50530 },
#endif
#ifdef WITH_T6963
  { "T6963 based", T6963 },
#endif
#ifdef WITH_USBLCD
  { "USBLCD", USBLCD },
#endif
#ifdef WITH_MATRIXORBITAL
  { "Matrix Orbital", MatrixOrbital },
#endif
#ifdef WITH_PALMPILOT
  { "3Com Palm Pilot", PalmPilot },
#endif
#if defined (WITH_PNG) || defined(WITH_PPM)
  { "Raster", Raster },
#endif
#ifdef WITH_SIN
  { "SIN Router", SIN },
#endif
#ifdef WITH_SKELETON
  { "Skeleton", Skeleton },
#endif
#ifdef WITH_X11
  { "X Window System", XWindow },
#endif
#ifdef WITH_TEXT
  { "ncurses Text", Text },
#endif
  { NULL }
};


static LCD *Lcd = NULL;

int lcd_list (void)
{
  int i, j;

  printf ("available display drivers:");
  
  for (i=0; Driver[i].name; i++) {
    printf ("\n   %-16s:", Driver[i].name);
    for (j=0; Driver[i].Model[j].name; j++) {
      printf (" %s", Driver[i].Model[j].name);
    }
  }
  printf ("\n");
  return 0;
}

int lcd_init (char *driver)
{
  int i, j;
  for (i=0; Driver[i].name; i++) {
    for (j=0; Driver[i].Model[j].name; j++) {
      if (strcmp (Driver[i].Model[j].name, driver)==0) {
	Lcd=&Driver[i].Model[j];
	if (Lcd->init==NULL) return 0;
	return Lcd->init(Lcd);
      }
    }
  }
  error ("lcd_init(%s) failed: no such display", driver);
  return -1;
}

int lcd_query (int *rows, int *cols, int *xres, int *yres, int *bars, int *gpos)
{
  if (Lcd==NULL)
    return -1;
  
  if (rows) *rows=Lcd->rows;
  if (cols) *cols=Lcd->cols;
  if (xres) *xres=Lcd->xres;
  if (yres) *yres=Lcd->yres;
  if (bars) *bars=Lcd->bars;
  if (gpos) *gpos=Lcd->gpos;

  return 0;
}

int lcd_clear (void)
{
  if (Lcd->clear==NULL) return 0;
  return Lcd->clear();
}

int lcd_put (int row, int col, char *text)
{
  if (row<1 || row>Lcd->rows) return -1;
  if (col<1 || col>Lcd->cols) return -1;
  if (Lcd->put==NULL) return 0;
  return Lcd->put(row-1, col-1, text);
}

int lcd_bar (int type, int row, int col, int max, int len1, int len2)
{
  if (row<1 || row>Lcd->rows) return -1;
  if (col<1 || col>Lcd->cols) return -1;
  if (!(type & (BAR_H2 | BAR_V2 | BAR_T))) len2=len1;
  if (type & BAR_LOG) {
    len1=(double)max*log(len1+1)/log(max); 
    if (!(type & BAR_T))
      len2=(double)max*log(len2+1)/log(max); 
  }
  if (Lcd->put==NULL) return 0;
  return Lcd->bar (type & BAR_HV, row-1, col-1, max, len1, len2);
}

int lcd_gpo (int num, int val)
{
  if (num<1 || num>Lcd->gpos) return -1;
  if (Lcd->gpo==NULL) return 0;
  return Lcd->gpo(num-1, val);
}

int lcd_flush (void)
{
  if (Lcd->flush==NULL) return 0;
  return Lcd->flush();
}

int lcd_quit (void)
{
  if (Lcd->quit==NULL) return 0;
  return Lcd->quit();
}
