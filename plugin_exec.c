/* $Id: plugin_exec.c,v 1.12 2006/07/12 21:01:41 reinelt Exp $
 *
 * plugin for external processes
 *
 * Copyright (C) 2004 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * based on the old 'exec' client which is
 * Copyright (C) 2001 Leopold Tötsch <lt@toetsch.at>
 *
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
 * $Log: plugin_exec.c,v $
 * Revision 1.12  2006/07/12 21:01:41  reinelt
 * thread_destroy, minor cleanups
 *
 * Revision 1.11  2006/07/12 20:47:51  reinelt
 * indent
 *
 * Revision 1.10  2006/07/12 20:45:30  reinelt
 * G15 and thread patch by Anton
 *
 * Revision 1.9  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.8  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.7  2004/09/24 21:41:00  reinelt
 * new driver for the BWCT USB LCD interface board.
 *
 * Revision 1.6  2004/06/26 12:05:00  reinelt
 *
 * uh-oh... the last CVS log message messed up things a lot...
 *
 * Revision 1.5  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.4  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.3  2004/06/17 06:23:43  reinelt
 *
 * hash handling rewritten to solve performance issues
 *
 * Revision 1.2  2004/04/08 10:48:25  reinelt
 * finished plugin_exec
 * modified thread handling
 * added '%x' format to qprintf (hexadecimal)
 *
 * Revision 1.1  2004/03/20 11:49:40  reinelt
 * forgot to add plugin_exec.c ...
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_exec (void)
 *  adds functions to start external pocesses
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"
#include "cfg.h"
#include "thread.h"
#include "qprintf.h"


#define NUM_THREADS 16
#define SHM_SIZE 4096

typedef struct {
    int delay;
    int mutex;
    pid_t pid;
    int shmid;
    char *cmd;
    char *key;
    char *ret;
} EXEC_THREAD;

static EXEC_THREAD Thread[NUM_THREADS];
static int max_thread = -1;

static HASH EXEC;


/* x^0 + x^5 + x^12 */
#define CRCPOLY 0x8408

static unsigned short CRC(const char *s)
{
    int i;
    unsigned short crc;

    /* seed value */
    crc = 0xffff;

    while (*s != '\0') {
	crc ^= *s++;
	for (i = 0; i < 8; i++)
	    crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY : 0);
    }
    return crc;
}


static void exec_thread(void *data)
{
    EXEC_THREAD *Thread = (EXEC_THREAD *) data;
    FILE *pipe;
    char buffer[SHM_SIZE];
    int len;

    /* use a safe path */
    putenv("PATH=/usr/local/bin:/usr/bin:/bin");

    /* forever... */
    while (1) {
	pipe = popen(Thread->cmd, "r");
	if (pipe == NULL) {
	    error("exec error: could not run pipe '%s': %s", Thread->cmd, strerror(errno));
	    len = 0;
	} else {
	    len = fread(buffer, 1, SHM_SIZE - 1, pipe);
	    if (len <= 0) {
		error("exec error: could not read from pipe '%s': %s", Thread->cmd, strerror(errno));
		len = 0;
	    }
	    pclose(pipe);
	}

	/* force trailing zero */
	buffer[len] = '\0';

	/* remove trailing CR/LF */
	while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
	    buffer[--len] = '\0';
	}

	/* lock shared memory */
	mutex_lock(Thread->mutex);
	/* write data */
	strncpy(Thread->ret, buffer, SHM_SIZE);
	/* unlock shared memory */
	mutex_unlock(Thread->mutex);
	usleep(Thread->delay);
    }
}


static void destroy_exec_thread(const int n)
{
    thread_destroy(Thread[n].pid);

    if (Thread[n].mutex != 0)
	mutex_destroy(Thread[n].mutex);
    if (Thread[n].cmd)
	free(Thread[n].cmd);
    if (Thread[n].key)
	free(Thread[n].key);
    if (Thread[n].ret)
	shm_destroy(Thread[n].shmid, Thread[n].ret);

    Thread[n].delay = 0;
    Thread[n].mutex = 0;
    Thread[n].pid = 0;
    Thread[n].shmid = 0;
    Thread[n].cmd = NULL;
    Thread[n].key = NULL;
    Thread[n].ret = NULL;
}


static int create_exec_thread(const char *cmd, const char *key, const int delay)
{
    char name[10];

    if (max_thread >= NUM_THREADS) {
	error("cannot create exec thread <%s>: thread buffer full!", cmd);
	return -1;
    }

    max_thread++;
    Thread[max_thread].delay = delay;
    Thread[max_thread].mutex = mutex_create();
    Thread[max_thread].pid = -1;
    Thread[max_thread].cmd = strdup(cmd);
    Thread[max_thread].key = strdup(key);
    Thread[max_thread].ret = NULL;

    /* create communication buffer */
    Thread[max_thread].shmid = shm_create((void **) &Thread[max_thread].ret, SHM_SIZE);

    /* catch error */
    if (Thread[max_thread].shmid < 0) {
	error("cannot create exec thread <%s>: shared memory allocation failed!", cmd);
	destroy_exec_thread(max_thread--);
	return -1;
    }

    /* create thread */
    qprintf(name, sizeof(name), "exec-%s", key);
    Thread[max_thread].pid = thread_create(name, exec_thread, &Thread[max_thread]);

    /* catch error */
    if (Thread[max_thread].pid < 0) {
	error("cannot create exec thread <%s>: fork failed?!", cmd);
	destroy_exec_thread(max_thread--);
	return -1;
    }

    return 0;
}


static int do_exec(const char *cmd, const char *key, int delay)
{
    int i, age;

    age = hash_age(&EXEC, key);

    if (age < 0) {
	hash_put(&EXEC, key, "");
	/* first-time call: create thread */
	if (delay < 10) {
	    error("exec(%s): delay %d is too short! using 10 msec", cmd, delay);
	    delay = 10;
	}
	if (create_exec_thread(cmd, key, 1000 * delay)) {
	    return -1;
	}
	return 0;
    }

    /* reread every 10 msec only */
    if (age > 0 && age <= 10)
	return 0;

    /* find thread */
    for (i = 0; i <= max_thread; i++) {
	if (strcmp(key, Thread[i].key) == 0) {
	    /* lock shared memory */
	    mutex_lock(Thread[i].mutex);
	    /* copy data */
	    hash_put(&EXEC, key, Thread[i].ret);
	    /* unlock shared memory */
	    mutex_unlock(Thread[i].mutex);
	    return 0;
	}
    }

    error("internal error: could not find thread exec-%s", key);
    return -1;
}

static void my_exec(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char *cmd, key[5], *val;
    int delay;

    cmd = R2S(arg1);
    delay = (int) R2N(arg2);

    qprintf(key, sizeof(key), "%x", CRC(cmd));

    if (do_exec(cmd, key, delay) < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    val = hash_get(&EXEC, key, NULL);
    if (val == NULL)
	val = "";

    SetResult(&result, R_STRING, val);
}


int plugin_init_exec(void)
{
    hash_create(&EXEC);
    AddFunction("exec", 2, my_exec);
    return 0;
}


void plugin_exit_exec(void)
{
    int i;

    for (i = 0; i <= max_thread; i++) {
	destroy_exec_thread(i);
    }

    hash_destroy(&EXEC);
}
