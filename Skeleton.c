/* $Id: Skeleton.c,v 1.1 2000/03/19 08:41:28 reinelt Exp $
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
 * struct DISPLAY Skeleton[]
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "cfg.h"
#include "display.h"

static DISPLAY Display;

int Skel_clear (void)
{
  return 0;
}

int Skel_init (DISPLAY *Self)
{
  Display=*Self;

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

int Skel_flush (void)
{
  return 0;
}


DISPLAY Skeleton[] = {
  { "Skeleton", 4, 20, 5, 8, BAR_L | BAR_R, Skel_init, Skel_clear, Skel_put, Skel_bar, Skel_flush },
  { "" }
};
