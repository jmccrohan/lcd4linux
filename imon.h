/* $Id: imon.h,v 1.2 2004/01/06 22:33:14 reinelt Exp $
 *
 * imond/telmond data processing
 *
 * Copyright 2003 Nico Wallmeier <nico.wallmeier@post.rwth-aachen.de>
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
 * $Log: imon.h,v $
 * Revision 1.2  2004/01/06 22:33:14  reinelt
 * Copyright statements cleaned up
 *
 * Revision 1.1  2003/10/12 06:08:28  nicowallmeier
 * imond/telmond support
 *
 */

#ifndef _IMON_H_
#define _IMON_H_

#define CHANNELS 9

struct imonchannel {
  int rate_in, rate_out, max_in, max_out; 
  char status[8], phone[17], ip[16], otime[11], charge[10], dev[6];
};

struct imon { 
  int cpu; 
  char date[11], time[9];
};

struct telmon { 
  char number[256], msn[256], time[9], date[11];
};

int Imon(struct imon *i, int cpu, int datetime);
int ImonCh(int index, struct imonchannel *ch, int token_usage[]);
char* ImonVer();

int Telmon(struct telmon *t);

#endif
