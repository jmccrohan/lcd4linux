/* $Id: display.c,v 1.2 2000/03/06 06:04:06 reinelt Exp $
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
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>

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
  return 0;
}

int lcd_put (int x, int y, char *text)
{
  return 0;
}

int lcd_bar (int type, int x, int y, int max, int len1, int len2)
{
  return 0;
}

int lcd_flush (void)
{
  return 0;
}

void main (void) {
  
  lcd_init ("junk");
  lcd_init ("LCD2041");

}
