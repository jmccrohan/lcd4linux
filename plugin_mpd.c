/* $Id$
 * $URL$
 *
 * mpd informations
 *
 * Copyright (C) 2006 Stefan Kuhne <sk-privat@gmx.net>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * exported functions:
 *
 * int plugin_init_sample (void)
 *  adds various functions
 *
 */


#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "plugin.h"

#include <libmpd/libmpd.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* Struct Pointer */

struct Pointer {
    mpd_Connection *conn;
    mpd_Status *status;
    mpd_InfoEntity *entity;
};



static struct Pointer connect()
{
    char *host = "localhost";
    char *port = "6600";
    int iport;
    char *test;
    struct Pointer mpd;

    if ((test = getenv("MPD_HOST"))) {
	host = test;
    }

    if ((test = getenv("MPD_PORT"))) {
	port = test;
    }

    iport = strtol(port, &test, 10);

    if (iport < 0 || *test != '\0') {
	fprintf(stderr, "MPD_PORT \"%s\" is not a positive integer\n", port);
	exit(EXIT_FAILURE);
    }

    mpd.conn = mpd_newConnection(host, iport, 10);

    mpd_sendCommandListOkBegin(mpd.conn);
    mpd_sendStatusCommand(mpd.conn);
    mpd_sendCurrentSongCommand(mpd.conn);
    mpd_sendCommandListEnd(mpd.conn);

    if ((mpd.status = mpd_getStatus(mpd.conn)) == NULL) {
	fprintf(stderr, "%s\n", mpd.conn->errorStr);
	mpd_closeConnection(mpd.conn);
    }

    if (mpd.status->error) {
	printf("error: %s\n", mpd.status->error);
    }

    if (mpd.conn->error) {
	fprintf(stderr, "%s\n", mpd.conn->errorStr);
	mpd_closeConnection(mpd.conn);
    }

    return mpd;
}


static void disconnect(struct Pointer mpd)
{
    if (mpd.conn->error) {
	fprintf(stderr, "%s\n", mpd.conn->errorStr);
	mpd_closeConnection(mpd.conn);
    }

    mpd_finishCommand(mpd.conn);
    if (mpd.conn->error) {
	fprintf(stderr, "%s\n", mpd.conn->errorStr);
	mpd_closeConnection(mpd.conn);
    }

    mpd_freeStatus(mpd.status);
    mpd_closeConnection(mpd.conn);
}


/* function 'artist' */
/* takes one argument, a number */
/* multiplies the number by 3.0 */
/* same as 'mul2', but shorter */

static void artist(RESULT * result, RESULT * query)
{
    char *value = " ";
    struct Pointer mpd = connect();

    mpd_nextListOkCommand(mpd.conn);

    while ((mpd.entity = mpd_getNextInfoEntity(mpd.conn))) {
	mpd_Song *song = mpd.entity->info.song;

	if (mpd.entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
	    mpd_freeInfoEntity(mpd.entity);
	    continue;
	}

	if (song->artist) {
	    value = strdup(song->artist);
	}
	mpd_freeInfoEntity(mpd.entity);
    }

    disconnect(mpd);

    /* store result */
    SetResult(&result, R_STRING, value);

    free(value);
}


static void title(RESULT * result, RESULT * query)
{
    char *value = " ";
    struct Pointer mpd = connect();

    mpd_nextListOkCommand(mpd.conn);

    while ((mpd.entity = mpd_getNextInfoEntity(mpd.conn))) {
	mpd_Song *song = mpd.entity->info.song;

	if (mpd.entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
	    mpd_freeInfoEntity(mpd.entity);
	    continue;
	}

	if (song->title) {
	    value = strdup(song->title);
	}
	mpd_freeInfoEntity(mpd.entity);
    }

    disconnect(mpd);

    /* store result */
    SetResult(&result, R_STRING, value);

    free(value);
}


static void album(RESULT * result, RESULT * query)
{
    char *value = " ";
    struct Pointer mpd = connect();

    mpd_nextListOkCommand(mpd.conn);

    while ((mpd.entity = mpd_getNextInfoEntity(mpd.conn))) {
	mpd_Song *song = mpd.entity->info.song;

	if (mpd.entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
	    mpd_freeInfoEntity(mpd.entity);
	    continue;
	}

	if (song->album) {
	    value = strdup(song->album);
	}
	mpd_freeInfoEntity(mpd.entity);
    }

    disconnect(mpd);

    /* store result */
    SetResult(&result, R_STRING, value);

    free(value);
}


int plugin_init_mpd(void)
{
    AddFunction("mpd::artist", 0, artist);
    AddFunction("mpd::title", 0, title);
    AddFunction("mpd::album", 0, album);

    return 0;
}


void plugin_exit_mpd(void)
{
    /* empty */
}
