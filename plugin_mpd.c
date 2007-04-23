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
	    //add comment
	    if (query) {
		char *myarg;
		myarg = strdup(R2S(query));
		value = strcat(value, myarg);
		free(myarg);
	    }
	}
	mpd_freeInfoEntity(mpd.entity);
    }

    disconnect(mpd);

    /* store result */
    SetResult(&result, R_STRING, value);

    free(value);
}


static void title(RESULT * result)
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


static void album(RESULT * result)
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

#define _mpd_dummy				000
#define _mpd_status_get_elapsed_song_time 	001
#define _mpd_status_get_bitrate			002
#define _mpd_status_get_total_song_time         003
#define _mpd_player_get_repeat                  004
#define _mpd_player_get_random			005

void error_callback(MpdObj * mi, int errorid, char *msg, void *userdata)
{
    printf("Error %i: '%s'\n", errorid, msg);
}

static int mpd_get(int function)
{
    int ret = -1;
    MpdObj *mi = NULL;

    mi = mpd_new("localhost", 6600, NULL);
    mpd_signal_connect_error(mi, (ErrorCallback) error_callback, NULL);
    mpd_set_connection_timeout(mi, 5);

    if (!mpd_connect(mi)) {
	switch (function) {
	case _mpd_dummy:
	    ret = 1;
	    break;
	case _mpd_status_get_elapsed_song_time:
	    ret = mpd_status_get_elapsed_song_time(mi);
	    break;
	case _mpd_status_get_bitrate:
	    ret = mpd_status_get_bitrate(mi);
	    break;
	case _mpd_status_get_total_song_time:
	    ret = mpd_status_get_total_song_time(mi);
	    break;
	case _mpd_player_get_repeat:
	    ret = mpd_player_get_repeat(mi);
	    break;
	case _mpd_player_get_random:
	    ret = mpd_player_get_random(mi);
	    break;
	}

	mpd_disconnect(mi);
	mpd_free(mi);
    }
    return ret;
}

static void elapsedTime(RESULT * result)
{
    char *value = " ";

    int playTime = mpd_get(_mpd_status_get_elapsed_song_time);

    if (playTime != -1) {
	char myTime[6];
	memset(myTime, 0, 6);
	int minutes = (int) (playTime / 60);
	int seconds = (int) (playTime % 60);
	sprintf(myTime, "%02d:%02d", minutes, seconds);

	value = strdup(myTime);
    }
    // store result     
    SetResult(&result, R_STRING, value);
}

static void elapsedTimeSec(RESULT * result)
{
    int playTime = mpd_get(_mpd_status_get_elapsed_song_time);
    double d = 0.0;

    if (playTime != -1)
	d = playTime;

    // store result     
    SetResult(&result, R_NUMBER, &d);
}

static void totalTime(RESULT * result)
{
    char *value = " ";

    int totTime = mpd_get(_mpd_status_get_total_song_time);
    if (totTime != -1) {
	char myTime[6];
	memset(myTime, 0, 6);
	int minutes = (int) (totTime / 60);
	int seconds = (int) (totTime % 60);
	sprintf(myTime, "%02d:%02d", minutes, seconds);

	value = strdup(myTime);
    } else
	value = strdup("ERROR");
    // store result     
    SetResult(&result, R_STRING, value);
}

static void totalTimeSec(RESULT * result)
{
    int totTime = mpd_get(_mpd_status_get_total_song_time);
    double d = 0.0;

    if (totTime != -1)
	d = totTime;

    // store result     
    SetResult(&result, R_NUMBER, &d);
}

static void bitRate(RESULT * result)
{
    char *value = "";

    int rate = mpd_get(_mpd_status_get_bitrate);

    if (rate != -1) {
	char rateStr[4];
	memset(rateStr, 0, 4);
	sprintf(rateStr, "%03d", rate);

	value = strdup(rateStr);
    }
    // store result     
    SetResult(&result, R_STRING, value);
}

static void getRepeat(RESULT * result)
{
    char *value = " ";

    int rep = mpd_get(_mpd_player_get_repeat);

    if (rep != -1) {
	if (rep)
	    value = strdup("REP");
	// else value = strdup("   ");          
    }
    // store result     
    SetResult(&result, R_STRING, value);
}


static void getRandom(RESULT * result)
{
    char *value = " ";

    int ran = mpd_get(_mpd_player_get_random);

    if (ran != -1) {
	if (ran)
	    value = strdup("RND");
	// else value = strdup("   ");          
    }
    // store result     
    SetResult(&result, R_STRING, value);
}

static void getRepRand(RESULT * result)
{
    char *value = " ";

    int ran = mpd_get(_mpd_player_get_random);
    int rep = mpd_get(_mpd_player_get_repeat);

    if (ran != -1 && rep != -1) {
	char str[9];
	if (rep)
	    sprintf(str, "REP/");
	else
	    sprintf(str, "---/");
	if (ran)
	    sprintf(str, "%sRND", str);
	else
	    sprintf(str, "%s---", str);
	value = strdup(str);
    }
    // store result     
    SetResult(&result, R_STRING, value);
}

int plugin_init_mpd(void)
{
    /* Check for File */
    if (mpd_get(_mpd_dummy) != 1) {
	error("Error: Cannot connect to MPD! Is MPD started?");
	return -1;
    }

    AddFunction("mpd::artist", 1, artist);
    AddFunction("mpd::title", 0, title);
    AddFunction("mpd::album", 0, album);
    AddFunction("mpd::totalTime", 0, totalTime);
    AddFunction("mpd::totalTimeSec", 0, totalTimeSec);
    AddFunction("mpd::elapsedTime", 0, elapsedTime);
    AddFunction("mpd::elapsedTimeSec", 0, elapsedTimeSec);
    AddFunction("mpd::bitRate", 0, bitRate);
    AddFunction("mpd::getRepeat", 0, getRepeat);
    AddFunction("mpd::getRandom", 0, getRandom);
    AddFunction("mpd::getRepRand", 0, getRepRand);
    return 0;
}


void plugin_exit_mpd(void)
{
    /* empty */
}
