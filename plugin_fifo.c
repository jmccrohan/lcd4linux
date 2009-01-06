/* $Id$
 * $URL$
 *
 * plugin template
 *
 * Copyright (C) 2008 Michael Vogt <michu@neophob.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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

/* 
 * Quick fifo hack for lcd4linux
 * 
 * most code is ripped ...
 *
 */

/* define the include files you need */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"
#include "cfg.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define FIFO_BUFFER_SIZE 80

typedef struct _FifoData {
    char *path;
    int input;
    int created;
} FifoData;

static char Section[] = "Plugin:FIFO";
static FifoData fd;
static char msg[FIFO_BUFFER_SIZE];
static char fifopath[1024];


static void configure_fifo(void)
{
    char *s;
    memset(fifopath, 0, 1024);
    s = cfg_get(Section, "fifopath", "/tmp/lcd4linux.fifo");
    if (*s == '\0') {
	info("[FIFO] empty '%s.fifopath' entry from %s, assuming '/tmp/lcd4linux.fifo'", Section, cfg_source());
	strcpy(fifopath, "/tmp/lcd4linux.fifo");
    } else {
	strcpy(fifopath, s);
	info("[FIFO] read '%s.fifopath', value is '%s'", Section, fifopath);
    }
    free(s);
}


static void removeFifo()
{
    debug("Removing FIFO \"%s\"\n", fd.path);
    if (unlink(fd.path) < 0) {
	error("Could not remove FIFO \"%s\": %s\n", fd.path, strerror(errno));
	return;
    }
    fd.created = 0;
}


static void closeFifo()
{
    struct stat st;
    if (fd.input >= 0) {
	close(fd.input);
	fd.input = -1;
    }
    if (fd.created && (stat(fd.path, &st) == 0))
	removeFifo(fd);
}

static int makeFifo()
{
    if (mkfifo(fd.path, 0666) < 0) {
	error("Couldn't create FIFO \"%s\": %s\n", fd.path, strerror(errno));
	return -1;
    }
    fd.created = 1;
    return 0;
}


static int checkFifo()
{
    struct stat st;
    if (stat(fd.path, &st) < 0) {
	if (errno == ENOENT) {

	    /* Path doesn't exist */
	    return makeFifo(fd);
	}
	error("Failed to stat FIFO \"%s\": %s\n", fd.path, strerror(errno));
	return -1;
    }
    if (!S_ISFIFO(st.st_mode)) {
	error("\"%s\" already exists, but is not a FIFO\n", fd.path);
	return -1;
    }
    return 0;
}


static int openFifo()
{
    if (checkFifo() < 0)
	return -1;
    fd.input = open(fd.path, O_RDONLY | O_NONBLOCK);
    if (fd.input < 0) {
	error("Could not open FIFO \"%s\" for reading: %s\n", fd.path, strerror(errno));
	closeFifo();
	return -1;
    }
    return 0;
}


static void startFifo(void)
{
    static int started = 0;

    if (started)
	return;
    
    started = 1;
    
    configure_fifo();
    fd.path = fifopath;
    fd.input = -1;
    fd.created = 0;
    openFifo();

    /* ignore broken pipe */
    signal(SIGPIPE, SIG_IGN);

    memset(msg, 0, FIFO_BUFFER_SIZE);

}


static void fiforead(RESULT * result)
{
    char buf[FIFO_BUFFER_SIZE];
    unsigned int i;
    int bytes = 1;

    startFifo();

    memset(buf, 0, FIFO_BUFFER_SIZE);
    strcat(buf, "ERROR");

    if (checkFifo() == 0) {
	memset(buf, 0, FIFO_BUFFER_SIZE);

	while (bytes > 0 && errno != EINTR) {
	    bytes = read(fd.input, buf, FIFO_BUFFER_SIZE);
	}

	if (bytes < 0 || (errno > 0 && errno != EAGAIN)) {
	    error("[FIFO] Error %i: %s", errno, strerror(errno));
	} else {
	    if (strlen(buf) > 0) {
		strcpy(msg, buf);
	    }
	    for (i = 0; i < strlen(buf); i++) {
		if (msg[i] < 0x20)
		    msg[i] = ' ';
	    }
	}
    }
    /* store result */
    SetResult(&result, R_STRING, msg);
}


/* plugin initialization */
int plugin_init_fifo(void)
{
    AddFunction("fifo::read", 0, fiforead);
    return 0;
}


void plugin_exit_fifo(void)
{
    /* close filedescriptors */
    closeFifo();
}
