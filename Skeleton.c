/* $Id: Skeleton.c,v 1.11 2003/09/13 06:45:43 reinelt Exp $
 *
 * skeleton driver for new display modules
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
 * $Log: Skeleton.c,v $
 * Revision 1.11  2003/09/13 06:45:43  reinelt
 * icons for all remaining drivers
 *
 * Revision 1.10  2003/08/17 12:11:58  reinelt
 * framework for icons prepared
 *
 * Revision 1.9  2003/08/17 08:25:30  reinelt
 * preparations for liblcd4linux; minor bugs in SIN.c and Skeleton.c
 *
 * Revision 1.8  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.7  2001/03/09 13:08:11  ltoetsch
 * Added Text driver
 *
 * Revision 1.6  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.5  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.4  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.3  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.2  2000/03/22 07:33:50  reinelt
 *
 * FAQ added
 * new modules 'processor.c' contains all data processing
 *
 * Revision 1.1  2000/03/19 08:41:28  reinelt
 *
 * documentation available! README, README.MatrixOrbital, README.Drivers
 * added Skeleton.c as a starting point for new drivers
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD Skeleton[]
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "cfg.h"
#include "display.h"
#include "bar.h"

static LCD Lcd;

int Skel_clear (int full)
{
  return 0;
}

int Skel_init (LCD *Self)
{
  Lcd=*Self;

  error ("Skeleton: This driver does not drive anything!");
  return -1;

  Skel_clear(1);
  return 0;
}

int Skel_put (int row, int col, char *text)
{
  return 0;
}

int Skel_bar (int type, int row, int col, int max, int len1, int len2)
{
  return 0;
}

int Skel_icon (int num, int seq, int row, int col)
{
  return 0;
}

int Skel_gpo (int num, int val)
{
  return 0;
}

int Skel_flush (void)
{
  return 0;
}

int Skel_quit (void)
{
  info("Skeleton: we shut down now.");
  return 0;
}


LCD Skeleton[] = {
  { name: "Skeleton",
    rows:  4,
    cols:  20,
    xres:  5,
    yres:  8,
    bars:  BAR_L|BAR_R,
    icons: 0,
    gpos:  0,
    init:  Skel_init,
    clear: Skel_clear,
    put:   Skel_put,
    bar:   Skel_bar,
    icon:  Skel_icon,
    gpo:   Skel_gpo,
    flush: Skel_flush, 
    quit:  Skel_quit },
  { NULL }
};
