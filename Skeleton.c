/* $Id: Skeleton.c,v 1.7 2001/03/09 13:08:11 ltoetsch Exp $
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

static LCD Lcd;

int Skel_clear (void)
{
  return 0;
}

int Skel_init (LCD *Self)
{
  Lcd=*Self;

  error ("Skeleton: This driver does not drive anything!");
  return -1;

  Skel_clear();
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
  { "Skeleton",4,20,5,8,BAR_L|BAR_R,0,Skel_init,Skel_clear,Skel_put,Skel_bar,Skel_gpo,Skel_flush, Skel_quit },
  { NULL }
};
