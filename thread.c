/* $Id: thread.c,v 1.1 2004/03/19 06:37:47 reinelt Exp $
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
 * $Log: thread.c,v $
 * Revision 1.1  2004/03/19 06:37:47  reinelt
 * asynchronous thread handling started
 *
 */

/* 
 * exported functions:
 * 
 * Fixme: document me!
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "debug.h"
#include "thread.h"


#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* glibc 2.1 requires defining semun ourselves */
#ifdef _SEM_SEMUN_UNDEFINED
union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
  struct seminfo *__buf;
};
#endif


int mutex_create (void)
{
  int semid;
  union semun semun;
  
  semid = semget(IPC_PRIVATE, 1, 0);
  if (semid == -1) {
    error ("fatal error: semget() failed: %s", strerror(errno));
    return -1;
  }
  semun.val = 1;
  semctl(semid, 0, SETVAL, semun);
  
  return semid;
}


void mutex_lock (int semid) 
{
  struct sembuf sembuf;
  sembuf.sem_num =  0;
  sembuf.sem_op  = -1;
  sembuf.sem_flg =  0;
  semop (semid, &sembuf, 1);
}


void mutex_unlock (int semid) 
{
  struct sembuf sembuf;
  sembuf.sem_num = 0;
  sembuf.sem_op  = 1;
  sembuf.sem_flg = 0;
  semop (semid, &sembuf, 1);
}


void mutex_destroy (int semid)
{
  union semun arg;
  semctl(semid, 0, IPC_RMID, arg);
}


int shm_create (void **buffer, int size) 
{
  int shmid;
  
  shmid = shmget(IPC_PRIVATE, size, SHM_R|SHM_W);
  if (shmid==-1) {
    error ("fatal error: shmget() failed: %s", strerror(errno));
    return -1;
  }

  *buffer = shmat(shmid, NULL, 0);
  if (*buffer == NULL) {
    error ("fatal error: shmat() failed: %s", strerror(errno));
    return -1;
  }
  
  return shmid;
}


void shm_destroy (int shmid) 
{
  shmctl(shmid, IPC_RMID, NULL);
}


int thread_create (void (*thread)(void))
{
  /*
  ppid=getpid();

  switch(async_updater_pid=fork()) {
  case -1:
    error ("X11: fork() failed: %s", strerror(errno));
    return -1;

  case 0:
    async_update();
    error ("X11: async_update failed");
    kill(ppid,SIGTERM);
    exit(-1);
    
  default:
    break;
  }

  signal(SIGCHLD,quit_updater);
  atexit(quit_updater);
  */
  return 0;
}
