/* $Id$
 * $URL$
 *
 * mpd informations v0.82
 *
 * Copyright (C) 2006 Stefan Kuhne <sk-privat@gmx.net>
 * Copyright (C) 2007 Robert Buchholz <rbu@gentoo.org>
 * Copyright (C) 2008 Michael Vogt <michu@neophob.com>
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
 * changelog v0.5 (20.11.2007):
 *   changed: 	mpd::artist(), mpd::title(), mpd::album()
 *              init code, call only if a function is used.
 *   removed: 	"old style" functions, duplicate code
 *   fixed: 	strdup() mem leaks
 *   added:	getstate, function which return player status: play/pause/stop
 *		getVolume, funtcion which returns the mpd volume
 *		getFilename, return current filename
 *
 * 
 * changelog v0.6 (05.12.2007):
 *  changed:    -connection handling
 *              -verbose "NO TAG" messages
 *              -init code
 *  added:      -added password support (MPD_PASSWORD env variable)
 *              -mpd::getSongsInDb - how many songs are in the mpd db?
 *              -mpd::getMpdUptime - uptime of mpd
 *              -mpd::getMpdPlayTime - playtime of mpd
 *              -mpd::getMpdDbPlayTime - playtime of all songs in mpd db
 *              -basic error handler..
 *		-mpd configuration in lcd4linux.conf
 *		-formatTime* - use those functions to format time values
 *  removed:    -reprand method
 *
 *
 * changelog v0.7 (14.12.2007):
 *  changed:	-connection handling improved, do not disconnect/reconnect for each query
 *               -> uses less ressources
 *
 * changelog v0.8 (30.01.2008):
 *  changed:  -libmpd is not needed anymore, use libmpdclient.c instead
 *  fixed:    -getMpdUptime()
 *            -getMpdPlaytime()
 *            -type definition
 *  added:    -mpd::getSamplerateHz
 *            -getAudioChannels
 *
 * changelog v0.83 (26.07.2008):
 *  added:    -mpd::cmd* commands
 *
 */

/*

TODO: 
 -what happens if the db is updating? 

*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"
/* struct timeval */
#include <sys/time.h>

/* source: http://www.musicpd.org/libmpdclient.shtml */
#include "libmpd/libmpdclient.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define TIMEOUT_IN_S 10
#define ERROR_DISPLAY 5

/* current song */

static int l_totalTimeSec;
static int l_elapsedTimeSec;
static int l_bitRate;
static int l_repeatEnabled;
static int l_randomEnabled;
static int l_state;
static int l_volume;
static int l_numberOfSongs;
static unsigned long l_uptime;
static unsigned long l_playTime;
static unsigned long l_dbPlayTime;
static int l_playlistLength;
/* pos in playlist */
static int l_currentSongPos;
static unsigned int l_sampleRate;
static int l_channels;

static mpd_Song *currentSong;

/* connection information */
static char host[255];
static char pw[255];
static int iport;
static int plugin_enabled;
static int waittime;
struct timeval timestamp;

static mpd_Connection *conn;
static char Section[] = "Plugin:MPD";
static int errorcnt = 0;


static int configure_mpd(void)
{
    static int configured = 0;

    char *s;

    if (configured != 0)
	return configured;

    /* read enabled */
    if (cfg_number(Section, "enabled", 0, 0, 1, &plugin_enabled) < 1) {
	plugin_enabled = 0;
    }

    if (plugin_enabled != 1) {
	info("[MPD] WARNING: Plugin is not enabled! (set 'enabled 1' to enable this plugin)");
	configured = 1;
	return configured;
    }

    /* read server */
    s = cfg_get(Section, "server", "localhost");
    if (*s == '\0') {
	info("[MPD] empty '%s.server' entry from %s, assuming 'localhost'", Section, cfg_source());
	strcpy(host, "localhost");
    } else
	strcpy(host, s);

    free(s);

    /* read port */
    if (cfg_number(Section, "port", 6600, 1, 65536, &iport) < 1) {
	info("[MPD] no '%s.port' entry from %s using MPD's default", Section, cfg_source());
    }

    /* read minUpdateTime in ms */
    if (cfg_number(Section, "minUpdateTime", 500, 1, 10000, &waittime) < 1) {
	info("[MPD] no '%s.minUpdateTime' entry from %s using MPD's default", Section, cfg_source());
    }


    /* read password */
    s = cfg_get(Section, "password", "");
    if (*s == '\0') {
	info("[MPD] empty '%s.password' entry in %s, assuming none", Section, cfg_source());
	memset(pw, 0, sizeof(pw));
    } else {
	strcpy(pw, s);
	free(s);
    }

    debug("[MPD] connection detail: [%s:%d]", host, iport);
    configured = 1;
    return configured;
}


static int mpd_update()
{
    int ret = -1;
    struct timeval now;

    /* reread every 1000 msec only */
    gettimeofday(&now, NULL);
    int timedelta = (now.tv_sec - timestamp.tv_sec) * 1000 + (now.tv_usec - timestamp.tv_usec) / 1000;

    if (timedelta < waittime) {
	/* debug("[MPD] waittime not reached...\n"); */
	return 1;
    }

    /* check if configured */
    if (configure_mpd() < 0) {
	return -1;
    }
    /* check if connected */
    if (conn == NULL || conn->error) {
	if (conn) {
	    if (errorcnt < ERROR_DISPLAY)
		debug("[MPD] Error: [%s], try to reconnect to [%s]:[%i]\n", conn->errorStr, host, iport);
	    mpd_closeConnection(conn);
	} else
	    debug("[MPD] initialize connect to [%s]:[%i]\n", host, iport);

	conn = mpd_newConnection(host, iport, TIMEOUT_IN_S);
	if (conn->error) {
	    if (errorcnt < ERROR_DISPLAY)
		error("[MPD] connection failed, give up...");
	    if (errorcnt == ERROR_DISPLAY)
		error("[MPD] stop logging, until connection is fixed!");
	    errorcnt++;
	    gettimeofday(&timestamp, NULL);
	    return -1;
	}
	errorcnt = 0;
	debug("[MPD] connection fixed...");
    }

    mpd_Status *status = NULL;
    mpd_Stats *stats = NULL;
    mpd_InfoEntity *entity;

    mpd_sendCommandListOkBegin(conn);
    mpd_sendStatsCommand(conn);

    if (conn->error) {
	error("[MPD] error: %s", conn->errorStr);
	return -1;
    }

    mpd_sendStatusCommand(conn);
    mpd_sendCurrentSongCommand(conn);
    mpd_sendCommandListEnd(conn);

    stats = mpd_getStats(conn);
    if (stats == NULL) {
	error("[MPD] error mpd_getStats: %s", conn->errorStr);
	goto cleanup;
    }

    mpd_nextListOkCommand(conn);
    if ((status = mpd_getStatus(conn)) == NULL) {
	error("[MPD] error mpd_nextListOkCommand: %s", conn->errorStr);
	goto cleanup;
    }

    mpd_nextListOkCommand(conn);
    while ((entity = mpd_getNextInfoEntity(conn))) {
	mpd_Song *song = entity->info.song;

	if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
	    mpd_freeInfoEntity(entity);
	    continue;
	}
	if (currentSong != NULL)
	    mpd_freeSong(currentSong);

	currentSong = mpd_songDup(song);
	mpd_freeInfoEntity(entity);
    }

    l_elapsedTimeSec = status->elapsedTime;
    l_totalTimeSec = status->totalTime;
    l_repeatEnabled = status->repeat;
    l_randomEnabled = status->random;
    l_bitRate = status->bitRate;
    l_state = status->state;
    l_volume = status->volume;
    l_playlistLength = status->playlistLength;
    l_currentSongPos = status->song + 1;
    l_sampleRate = status->sampleRate;
    l_channels = status->channels;

    l_numberOfSongs = stats->numberOfSongs;
    l_uptime = stats->uptime;
    l_playTime = stats->playTime;
    l_dbPlayTime = stats->dbPlayTime;


    /* sanity checks */
    if (l_volume < 0 || l_volume > 100)
	l_volume = 0;

    if (l_bitRate < 0)
	l_bitRate = 0;

    if (l_elapsedTimeSec > l_totalTimeSec || l_elapsedTimeSec < 0)
	l_elapsedTimeSec = 0;
    ret = 0;

  cleanup:
    if (stats != NULL)
	mpd_freeStats(stats);

    if (status != NULL)
	mpd_freeStatus(status);

    if (conn->error) {
	error("[MPD] error: %s", conn->errorStr);
	return -1;
    }

    mpd_finishCommand(conn);
    if (conn->error) {
	error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	return -1;
    }

    gettimeofday(&timestamp, NULL);
    return ret;
}


static void elapsedTimeSec(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_elapsedTimeSec;
    SetResult(&result, R_NUMBER, &d);
}


static void totalTimeSec(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_totalTimeSec;
    SetResult(&result, R_NUMBER, &d);
}

static void bitRate(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_bitRate;
    SetResult(&result, R_NUMBER, &d);
}


static void getRepeatInt(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_repeatEnabled;
    SetResult(&result, R_NUMBER, &d);
}


static void getRandomInt(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_randomEnabled;
    SetResult(&result, R_NUMBER, &d);
}

/* if no tag is availabe, use filename */
static void getArtist(RESULT * result)
{
    mpd_update();
    if (currentSong != NULL) {
	if (currentSong->artist != NULL) {
	    SetResult(&result, R_STRING, currentSong->artist);
	} else {
	    if (currentSong->file != NULL)
		SetResult(&result, R_STRING, currentSong->file);
	    else
		SetResult(&result, R_STRING, "");
	}
    } else
	SetResult(&result, R_STRING, "");

}

static void getTitle(RESULT * result)
{
    mpd_update();
    if (currentSong != NULL) {
	if (currentSong->title != NULL) {
	    SetResult(&result, R_STRING, currentSong->title);
	} else
	    SetResult(&result, R_STRING, "");
    } else
	SetResult(&result, R_STRING, "");

}

static void getAlbum(RESULT * result)
{
    mpd_update();
    if (currentSong != NULL) {
	if (currentSong->album != NULL)
	    SetResult(&result, R_STRING, currentSong->album);
	else
	    SetResult(&result, R_STRING, "");
    } else
	SetResult(&result, R_STRING, "");
}

static void getFilename(RESULT * result)
{
    mpd_update();
    if (currentSong != NULL) {
	if (currentSong->file != NULL)
	    SetResult(&result, R_STRING, currentSong->file);
	else
	    SetResult(&result, R_STRING, "");
    } else
	SetResult(&result, R_STRING, "");

}

/*  
    return player state:
	0=unknown
	1=play
	2=pause
	3=stop
*/
static void getStateInt(RESULT * result)
{
    double ret;

    mpd_update();

    switch (l_state) {
    case MPD_STATUS_STATE_PLAY:
	ret = 1;
	break;
    case MPD_STATUS_STATE_PAUSE:
	ret = 2;
	break;
    case MPD_STATUS_STATE_STOP:
	ret = 3;
	break;
    default:
	ret = 0;
	break;
    }

    SetResult(&result, R_NUMBER, &ret);
}


static void getVolume(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_volume;
    /* return 0..100 or < 0 when failed */
    SetResult(&result, R_NUMBER, &d);
}

/* return the # of songs in the mpd db .. */
static void getSongsInDb(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_numberOfSongs;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdUptime(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_uptime;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdPlayTime(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_playTime;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdDbPlayTime(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_dbPlayTime;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdPlaylistLength(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_playlistLength;
    SetResult(&result, R_NUMBER, &d);
}

static void getCurrentSongPos(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_currentSongPos;
    SetResult(&result, R_NUMBER, &d);
}

static void getAudioChannels(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_channels;
    SetResult(&result, R_NUMBER, &d);
}

static void getSamplerateHz(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_sampleRate;
    SetResult(&result, R_NUMBER, &d);
}


static void nextSong()
{
    mpd_update();
    if (currentSong != NULL) {
	mpd_sendNextCommand(conn);
	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}

static void prevSong()
{
    mpd_update();
    if (currentSong != NULL) {
	mpd_sendPrevCommand(conn);
	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}

static void stopSong()
{
    mpd_update();
    if (currentSong != NULL) {
	mpd_sendStopCommand(conn);
	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}

static void pauseSong()
{
    mpd_update();
    if (currentSong != NULL) {
	if (l_state == MPD_STATUS_STATE_PAUSE) {
	    mpd_sendPauseCommand(conn, 0);
	} else {
	    mpd_sendPauseCommand(conn, 1);
	}

	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}

static void volUp()
{
    mpd_update();
    if (currentSong != NULL) {
	l_volume += 5;
	if (l_volume > 100)
	    l_volume = 100;
	mpd_sendSetvolCommand(conn, l_volume);
	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}

static void volDown()
{
    mpd_update();
    if (currentSong != NULL) {
	if (l_volume > 5)
	    l_volume -= 5;
	else
	    l_volume = 0;
	mpd_sendSetvolCommand(conn, l_volume);
	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}

static void toggleRepeat()
{
    mpd_update();
    if (currentSong != NULL) {

	l_repeatEnabled = !l_repeatEnabled;
	mpd_sendRepeatCommand(conn, l_repeatEnabled);

	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}


static void toggleRandom()
{
    mpd_update();
    if (currentSong != NULL) {

	l_randomEnabled = !l_randomEnabled;
	mpd_sendRandomCommand(conn, l_randomEnabled);

	mpd_finishCommand(conn);
	if (conn->error) {
	    error("[MPD] error mpd_finishCommand: %s", conn->errorStr);
	}
    }
}


static void formatTimeMMSS(RESULT * result, RESULT * param)
{
    long sec;
    char myTime[6] = " ";

    sec = R2N(param);

    if ((sec >= 0) && (sec < 6000)) {
	const int minutes = (int) (sec / 60);
	const int seconds = (int) (sec % 60);
	sprintf(myTime, "%02d:%02d", minutes, seconds);
    } else if (sec >= 6000) {
	strcpy(myTime, "LONG");
    } else
	strcpy(myTime, "ERROR");

    /* store result */
    SetResult(&result, R_STRING, myTime);
}

static void formatTimeDDHHMM(RESULT * result, RESULT * param)
{
    long sec;
    char myTime[16] = " ";

    sec = R2N(param);

    if (sec >= 0) {
	int days = sec / 86400;
	int hours = (sec % 86400) / 3600;
	int minutes = (sec % 3600) / 60;
	sprintf(myTime, "%dd%02dh%02dm", days, hours, minutes);	/* 3d 12:33h */
    } else
	strcpy(myTime, "ERROR");

    /* store result */
    SetResult(&result, R_STRING, myTime);
}



int plugin_init_mpd(void)
{
    int check;
    debug("[MPD] v0.83, check lcd4linux configuration file...");

    check = configure_mpd();
    if (plugin_enabled != 1)
	return 0;

    if (check)
	debug("[MPD] configured!");
    else
	debug("[MPD] error, NOT configured!");

    /* when mpd dies, do NOT exit application, ignore it! */
    signal(SIGPIPE, SIG_IGN);
    gettimeofday(&timestamp, NULL);

    AddFunction("mpd::artist", 0, getArtist);
    AddFunction("mpd::title", 0, getTitle);
    AddFunction("mpd::album", 0, getAlbum);
    AddFunction("mpd::file", 0, getFilename);
    AddFunction("mpd::totalTimeSec", 0, totalTimeSec);
    AddFunction("mpd::elapsedTimeSec", 0, elapsedTimeSec);
    AddFunction("mpd::bitRate", 0, bitRate);
    AddFunction("mpd::getSamplerateHz", 0, getSamplerateHz);
    AddFunction("mpd::getAudioChannels", 0, getAudioChannels);
    AddFunction("mpd::getRepeatInt", 0, getRepeatInt);
    AddFunction("mpd::getRandomInt", 0, getRandomInt);
    AddFunction("mpd::getStateInt", 0, getStateInt);
    AddFunction("mpd::getVolume", 0, getVolume);
    AddFunction("mpd::getSongsInDb", 0, getSongsInDb);
    AddFunction("mpd::getMpdUptime", 0, getMpdUptime);
    AddFunction("mpd::getMpdPlayTime", 0, getMpdPlayTime);
    AddFunction("mpd::getMpdDbPlayTime", 0, getMpdDbPlayTime);
    AddFunction("mpd::getMpdPlaylistLength", 0, getMpdPlaylistLength);
    AddFunction("mpd::getMpdPlaylistGetCurrentId", 0, getCurrentSongPos);

    AddFunction("mpd::cmdNextSong", 0, nextSong);
    AddFunction("mpd::cmdPrevSong", 0, prevSong);
    AddFunction("mpd::cmdStopSong", 0, stopSong);
    AddFunction("mpd::cmdTogglePauseSong", 0, pauseSong);
    AddFunction("mpd::cmdVolUp", 0, volUp);
    AddFunction("mpd::cmdVolDown", 0, volDown);
    AddFunction("mpd::cmdToggleRandom", 0, toggleRandom);
    AddFunction("mpd::cmdToggleRepeat", 0, toggleRepeat);

    AddFunction("mpd::formatTimeMMSS", 1, formatTimeMMSS);
    AddFunction("mpd::formatTimeDDHHMM", 1, formatTimeDDHHMM);

    return 0;
}


void plugin_exit_mpd(void)
{
    if (plugin_enabled == 1) {
	debug("[MPD] disconnect from mpd");
	if (currentSong != NULL)
	    mpd_freeSong(currentSong);
	mpd_closeConnection(conn);
    }
}
