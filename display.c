/* $Id: display.c,v 1.3 2000/03/10 10:49:53 reinelt Exp $
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
 * Revision 1.3  2000/03/10 10:49:53  reinelt
 *
 * MatrixOrbital driver finished
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "display.h"

extern DISPLAY MatrixOrbital[];

FAMILY Driver[] = {
  { "MatrixOrbital", MatrixOrbital },
  { "" }
};


static DISPLAY *Display = NULL;

int lcd_init (char *display)
{
  int i, j;
  for (i=0; Driver[i].name[0]; i++) {
    for (j=0; Driver[i].Display[j].name[0]; j++) {
      if (strcmp (Driver[i].Display[j].name, display)==0) {
	Display=&Driver[i].Display[j];
	return Display->init(Display);
      }
    }
  }
  fprintf (stderr, "lcd_init(%s) failed: no such display\n", display);
  return -1;
}

int lcd_clear (void)
{
  return Display->clear();
}

int lcd_put (int row, int col, char *text)
{
  if (row<1 || row>Display->rows) return -1;
  if (col<1 || col>Display->cols) return -1;
  return Display->put(row-1, col-1, text);
}

int lcd_bar (int type, int row, int col, int max, int len1, int len2)
{
  if (row<1 || row>Display->rows) return -1;
  if (col<1 || col>Display->cols) return -1;
  return Display->bar(type, row-1, col-1, max, len1, len2);
}

int lcd_flush (void)
{
  return Display->flush();
}

