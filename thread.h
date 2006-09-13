/* $Id: thread.h,v 1.8 2006/09/13 20:04:57 entropy Exp $
 *
 * thread handling (mutex, shmem, ...)
 *
 * Copyright (C) 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * parts of this code are based on the old XWindow driver which is
 * Copyright (C) 2000 Herbert Rosmanith <herp@wildsau.idv.uni-linz.ac.at>
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
 * Revision 1.8  2006/09/13 20:04:57  entropy
 * threads change argv[0] to their thread name, for a neat 'ps' output
 *
 * Revision 1.7  2006/07/12 21:01:41  reinelt
 * thread_destroy, minor cleanups
 *
 * Revision 1.6  2005/05/08 04:32:45  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.5  2005/01/18 06:30:24  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.4  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
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


extern int   thread_argc;
extern char **thread_argv;

int mutex_create(void);
void mutex_lock(const int semid);
void mutex_unlock(const int semid);
void mutex_destroy(const int semid);

int shm_create(void **buffer, const int size);
void shm_destroy(const int shmid, const void *buffer);

int thread_create(const char *name, void (*thread) (void *data), void *data);
int thread_destroy(const int pid);

#endif
