/* $Id: thread.h,v 1.3 2004/04/08 10:48:25 reinelt Exp $
 *
 * thread handling (mutex, shmem, ...)
 *
 * Copyright 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * parts of this code are based on the old XWindow driver which is
 * Copyright 2000 Herbert Rosmanith <herp@wildsau.idv.uni-linz.ac.at>
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
 * $Log: thread.h,v $
 * Revision 1.3  2004/04/08 10:48:25  reinelt
 * finished plugin_exec
 * modified thread handling
 * added '%x' format to qprintf (hexadecimal)
 *
 * Revision 1.2  2004/03/20 07:31:33  reinelt
 * support for HD66712 (which has a different RAM layout)
 * further threading development
 *
 * Revision 1.1  2004/03/19 06:37:47  reinelt
 * asynchronous thread handling started
 *
 */

#ifndef _THREAD_H_
#define _THREAD_H_

int  mutex_create  (void);
void mutex_lock    (int semid);
void mutex_unlock  (int semid);
void mutex_destroy (int semid);

int  shm_create    (void **buffer, int size);
void shm_destroy   (int shmid, void *buffer) ;

int thread_create (char *name, void (*thread)(void *data), void *data);

#endif
