/* $Id: icon.h,v 1.3 2003/09/09 05:30:34 reinelt Exp $
 *
 * generic icon and heartbeat handling
 *
 * written 2003 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: icon.h,v $
 * Revision 1.3  2003/09/09 05:30:34  reinelt
 * even more icons stuff
 *
 * Revision 1.2  2003/09/01 04:09:35  reinelt
 * icons nearly finished, but MatrixOrbital only
 *
 * Revision 1.1  2003/08/24 04:31:56  reinelt
 * icon.c icon.h added
 *
 *
 */

#ifndef _ICON_H_
#define _ICON_H_

int  icon_init (int rows, int cols, int xres, int yres, int chars, int icons, 
		void(*defchar)(int ascii, char *bitmap));
void icon_clear(void);
int  icon_draw (int num, int seq, int row, int col);
int  icon_peek (int row, int col);

#endif
