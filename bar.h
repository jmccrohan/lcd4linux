/* $Id: bar.h,v 1.2 2002/08/19 07:36:29 reinelt Exp $
 *
 * generic bar handling
 *
 * Copyright 2002 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: bar.h,v $
 * Revision 1.2  2002/08/19 07:36:29  reinelt
 *
 * finished bar.c, USBLCD is the first driver that uses the generic bar functions
 *
 * Revision 1.1  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 *
 */

#ifndef _BAR_H_
#define _BAR_H_

#define BAR_L   (1<<0)
#define BAR_R   (1<<1)
#define BAR_U   (1<<2)
#define BAR_D   (1<<3)
#define BAR_H2  (1<<4)
#define BAR_V2  (1<<5)
#define BAR_LOG (1<<6)
#define BAR_T   (1<<7)

#define BAR_H (BAR_L | BAR_R)
#define BAR_V (BAR_U | BAR_D | BAR_T)
#define BAR_HV (BAR_H | BAR_V)


typedef struct {
  int len1;
  int len2;
  int type;
  int segment;
} BAR;

typedef struct {
  int len1;
  int len2;
  int type;
  int used;
  int ascii;
} SEGMENT;


int  bar_init (int rows, int cols, int xres, int yres, int chars);
void bar_clear(void);
void bar_add_segment(int len1, int len2, int type, int ascii);
int  bar_draw (int type, int row, int col, int max, int len1, int len2);
int  bar_process (int(*defchar)(int ascii, char *matrix));
int  bar_peek (int row, int col);

#endif
