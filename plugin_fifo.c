/* $Id$
 * $URL$
 *
 * Fifo plugin
 *
 * Copyright (C) 2008 Michael Vogt <michu@neophob.com>
 * Copyright (C) 2010 Mattia Jona-Lasinio <mjona@users.sourceforge.net>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * Configuration parameters:
 *
 * - FifoPath 'string'	: use <string> as the fifo complete file path
 *			  If absent use  /tmp/lcd4linux.fifo)
 *
 * - FifoBufSize num	: if the plugin is unable to determine the display size then
 *			  set the size of the internal buffer to <num> characters
 *			  otherwise use the display size (number of columns).
 *			  If no display size is available and no FifoBufSize parameter
  *			  is specified then arbitrarily set the internal buffer size
 *			  to 80 characters.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define FIFO_MAXPATH		256
#define FIFO_DEFAULT_PATH	/tmp/lcd4linux.fifo
#define FIFO_DEFAULT_BUFSIZE	80
#define str(s) #s
#define string(s) str(s)

struct FifoData {
	char *path;
	char *msg;
	int msglen;
	int input;
	int created;
};

static struct FifoData fd = {
	.path		= NULL,
	.msg		= NULL,
	.msglen		= -1,
	.input		= -1,
	.created	= -1,
};


static int confFifo(struct FifoData *p)
{
	char *path, *disp, *sect, *fifosect = "Plugin:FIFO";
	unsigned int pathlen;

	info("[FIFO] Reading config file '%s'", cfg_source());

	path = cfg_get(fifosect, "FifoPath", string(FIFO_DEFAULT_PATH));
	pathlen = strlen(path);
	if (pathlen == 0) {
		info("[FIFO] Invalid '%s.FifoPath' entry from '%s'. "
			"Assuming "string(FIFO_DEFAULT_PATH), fifosect, cfg_source());
		free(path);
		path = strdup(string(FIFO_DEFAULT_PATH));
		pathlen = strlen(path);
	}
	if (pathlen > FIFO_MAXPATH) {
		error("[FIFO] Error: Too long '%s.FifoPath' entry from '%s'. "
			"(MAX "string(FIFO_MAXPATH)" chars)", fifosect, cfg_source());
		free(path);
		return (-1);
	}
	info("[FIFO] Read '%s.FifoPath' value is '%s'", fifosect, path);

	disp = cfg_get(NULL, "Display", NULL);
	if (disp == NULL) {
		error("[FIFO] Error: Could not get the Display name from '%s'", cfg_source());
		free(path);
		return (-1);
	}
	if ((sect = malloc(1+strlen("Display:")+strlen(disp))) == NULL) {
		error("[FIFO] Error: Memory allocation failed");
		free(disp);
		free(path);
		return (-1);
	}
	strcpy(sect, "Display:");
	strcat(sect, disp);
	info("[FIFO] Using display '%s'.", disp);
	free(disp);

	disp = cfg_get(sect, "Size", NULL);
	if (disp != NULL) {
		info("[FIFO] Getting the buffer size from '%s.Size'", sect);
		if (sscanf(disp, "%dx%*d", &p->msglen) != 1) {
			info("[FIFO] Could not determine the display size. "
				"Assuming "string(FIFO_DEFAULT_BUFSIZE));
			p->msglen = FIFO_DEFAULT_BUFSIZE;
		}
		free(disp);
	} else {
		info("[FIFO] Could not find a '%s.Size' entry.", sect);
		if (cfg_number(fifosect, "FifoBufSize", FIFO_DEFAULT_BUFSIZE, 0, -1, &p->msglen) > 0) {
			info("[FIFO] Getting the buffer size from '%s.FifoBufSize'", fifosect);
		} else {
			info("[FIFO] Could not find a valid '%s.FifoBufSize' entry. "
				"Assuming "string(FIFO_DEFAULT_BUFSIZE), fifosect);
			p->msglen = FIFO_DEFAULT_BUFSIZE;
		}
	}
	info("[FIFO] Read buffer size is '%d'", p->msglen);
	free(sect);

	if ((p->msg = malloc(2+pathlen+p->msglen)) == NULL) {
		error("[FIFO] Error: Memory allocation failed");
		free(path);
		return (-1);
	}
	p->msg[0] = 0;
	p->path = p->msg+p->msglen+1;
	strcpy(p->path, path);
	free(path);

	return (0);
}


static int makeFifo(struct FifoData *p)
{
	struct stat st;

	if (stat(p->path, &st) < 0) {
		if (errno == ENOENT) {
			if (mkfifo(p->path, 0666) == 0) {
				p->created = 1;

				return (0);
			}
			error("Couldn't create FIFO \"%s\": %s\n", p->path, strerror(errno));

			return (-1);
		}
		error("Failed to stat FIFO \"%s\": %s\n", p->path, strerror(errno));

		return (-1);
	}

	if (! S_ISFIFO(st.st_mode)) {
		error("\"%s\" already exists, but is not a FIFO", p->path);

		return (-1);
	}

	return (0);
}


static void closeFifo(struct FifoData *p)
{
	struct stat st;

	if (p->input >= 0) {
		close(p->input);
		p->input = -1;
	}

	if ((p->created >= 0) && (stat(p->path, &st) == 0)) {
		debug("Removing FIFO \"%s\"\n", p->path);
		if (unlink(p->path) < 0) {
			error("Could not remove FIFO \"%s\": %s\n", p->path, strerror(errno));

			return;
		}
		p->created = -1;
	}

	if (p->msg) {
		free(p->msg);
		p->msg = p->path = NULL;
		p->msglen = -1;
	}
}


static int openFifo(struct FifoData *p)
{
	if (p->created < 0) {
		error("Error: FIFO \"%s\" does not exist: %s\n", p->path, strerror(errno));

		return (-1);
	}

	if ((p->input = open(p->path, O_RDONLY | O_NONBLOCK)) < 0) {
		error("Could not open FIFO \"%s\" for reading: %s\n", p->path, strerror(errno));
		closeFifo(p);

		return (-1);
	}

	return (0);
}


static int startFifo(struct FifoData *p)
{
	int res;

	if ((res = confFifo(p)))
		return (res);

	if ((res = makeFifo(p)))
		return (res);

	if ((res = openFifo(p)))
		return (res);

	/* ignore broken pipe */
	signal(SIGPIPE, SIG_IGN);

	return (res);
}


static void readFifo(struct FifoData *p)
{
	int bytes;

	bytes = read(p->input, p->msg, p->msglen);
	if (bytes == 0)
		return;

	if (bytes > 0) {
		p->msg[bytes] = 0;
		while (bytes--)
			if (p->msg[bytes] < 0x20)
				p->msg[bytes] = ' ';
	} else {
		error("[FIFO] Error %i: %s", errno, strerror(errno));
		strcpy(p->msg, "ERROR");
	}
}


static void runFifo(RESULT *result)
{
	static int state = 1;
	struct FifoData *p = &fd;
	char *s;

	switch (state) {
	case 1:
		/* Called for the first time. Set up everything. */
		state = startFifo(p);
		s = "";
		break;

	case 0:
		/* Init went fine. Now run in normal operation mode. */
		readFifo(p);
		s = p->msg;
		break;

	default:
		/* There was an error somewhere in init. Do nothing. */
		s = "ERROR";
		break;
	}

	/* Store the result */
	SetResult(&result, R_STRING, s);
}


/* plugin initialization */
int plugin_init_fifo(void)
{
	AddFunction("fifo::read", 0, runFifo);

	return (0);
}


void plugin_exit_fifo(void)
{
	closeFifo(&fd);
}
