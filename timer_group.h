/* $Id$
 * $URL$
 *
 * generic grouping of widget timers that have been set to the same
 * update interval, thus allowing synchronized updates
 *
 * Copyright (C) 2010 Martin Zuther <code@mzuther.de>
 * Copyright (C) 2010 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 */


#ifndef _TIMER_GROUP_H_
#define _TIMER_GROUP_H_

int timer_add_group(void (*callback) (void *data), const int interval);
int timer_remove_group(void (*callback) (void *data), const int interval);
void timer_process_group(void *data);
void timer_exit_group(void);

int timer_add_widget(void (*callback) (void *data), void *data, const int interval, const int one_shot);
int timer_remove_widget(void (*callback) (void *data), void *data);

#endif
