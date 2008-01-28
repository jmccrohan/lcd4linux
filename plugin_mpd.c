/* $Id$
 * $URL$
 *
 * mpd informations v0.7
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
 * exported functions:
 *
 * int plugin_init_sample (void)
 *  adds various functions
 *
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
 */

/*

TODO: 
 -what happens if the db is updating? int mpd_status_db_is_updating() 0/1

BUGS:
 -getMpdUptime() does not update its counter
 -getMpdPlaytime() does not update its counter

*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"
/* struct timeval */
#include <sys/time.h>


/* you need libmpd to compile this plugin! http://sarine.nl/libmpd */
#include <libmpd/libmpd.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* current song */
#define TAGLEN 512
#define MAX_PATH 1024
static char *artist[TAGLEN];
static char *album[TAGLEN];
static char *title[TAGLEN];
static char *filename[MAX_PATH];
static long l_totalTimeSec;
static long l_elapsedTimeSec;
static int l_bitRate;
static int l_repeatEnabled;
static int l_randomEnabled;
static int l_state;
static int l_volume;
static int l_songsInDb;
static long l_mpdUptime;
static long l_mpdPlaytime;
static long l_mpdDbPlaytime;
static long l_mpdPlaylistLength;
static int l_currentSongPos;

/* connection information */
static char host[255];
static char pw[255];
static int iport;
static int waittime;
struct timeval timestamp;

static MpdObj *mi = NULL;
static char Section[] = "Plugin:MPD";


static void error_callback( __attribute__ ((unused)) MpdObj * mi, int errorid, char *msg, __attribute__ ((unused))
		    void *userdata)
{
    debug("[MPD] caught error %i: '%s'", errorid, msg);
}


static int configure_mpd(void)
{
    static int configured = 0;

    char *s;

    if (configured != 0)
	return configured;

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

    mi = mpd_new(host, iport, pw);
    mpd_signal_connect_error(mi, (ErrorCallback) error_callback, NULL);
    mpd_set_connection_timeout(mi, 5);

    configured = 1;
    return configured;
}


static int mpd_update()
{
    int ret = -1;
    mpd_Song *song;
    static char *notagArt = "No artist tag";
    static char *notagTit = "No title tag";
    static char *notagAlb = "No album tag";
    static char *nofilename = "No Filename";
    struct timeval now;
    int len;

    //check if configured
    if (configure_mpd() < 0) {
	return -1;
    }

    /* reread every 1000 msec only */
    gettimeofday(&now, NULL);    
    int timedelta = (now.tv_sec - timestamp.tv_sec) * 1000 + (now.tv_usec - timestamp.tv_usec) / 1000;
    if (timedelta < waittime) {
	return 1;
    }

    //debug("[MPD] time delta: %i", timedelta);

    //send password if enabled
    if (pw[0] != 0)
	mpd_send_password(mi);
    
    //check if connected
    if (mpd_check_connected(mi) != 1) {
	debug("[MPD] not connected, try to reconnect...");
	mpd_connect(mi);
	
	if (mpd_check_connected(mi) != 1) {
	    debug("[MPD] connection failed, give up...");
	    return -1;
	}
	debug("[MPD] connection ok...");
    }

    ret = 0;
    
    mpd_status_update(mi);

    l_elapsedTimeSec 	= mpd_status_get_elapsed_song_time(mi);
    l_bitRate 		= mpd_status_get_bitrate(mi);
    l_totalTimeSec 	= mpd_status_get_total_song_time(mi);
    l_repeatEnabled	= mpd_player_get_repeat(mi);
    l_randomEnabled 	= mpd_player_get_random(mi);
    l_state 		= mpd_player_get_state(mi);
    l_volume 		= mpd_status_get_volume(mi);
    l_songsInDb 	= mpd_stats_get_total_songs(mi);
    l_mpdUptime 	= mpd_stats_get_uptime(mi);
    l_mpdPlaytime 	= mpd_stats_get_playtime(mi);
    l_mpdDbPlaytime 	= mpd_stats_get_db_playtime(mi);
    l_mpdPlaylistLength = mpd_playlist_get_playlist_length(mi);
    l_currentSongPos 	= mpd_player_get_current_song_pos(mi);

    /* sanity checks */
    if (l_volume < 0 || l_volume > 100)
	l_volume = 0;

    if (l_bitRate < 0)
	l_bitRate = 0;
	
    song = mpd_playlist_get_current_song(mi);
    if (song) {

	    /* copy song information to local variables */
	    memset(album, 0, TAGLEN);
	    if (song->album != 0) {
		len = strlen(song->album);
		if (len > TAGLEN)
		    len = TAGLEN;
		memcpy(album, song->album, len);

	    } else
		memcpy(album, notagAlb, strlen(notagAlb));

	    memset(artist, 0, TAGLEN);
	    if (song->artist != 0) {
		len = strlen(song->artist);
		if (len > TAGLEN)
		    len = TAGLEN;
		memcpy(artist, song->artist, len);
	    } else
		memcpy(artist, notagArt, strlen(notagArt));

	    memset(title, 0, TAGLEN);
	    if (song->title != 0) {
		len = strlen(song->title);
		if (len > TAGLEN)
		    len = TAGLEN;
		memcpy(title, song->title, len);
	    } else
		memcpy(title, notagTit, strlen(notagTit));

	    memset(filename, 0, MAX_PATH);
	    if (song->file != 0) {
		len = strlen(song->file);
		if (len > MAX_PATH)
		    len = MAX_PATH;
		memcpy(filename, song->file, len);
	    } else
		memcpy(filename, nofilename, strlen(nofilename));
    }
    
//    mpd_disconnect(mi);		/* you need to disconnect here */
    gettimeofday(&timestamp, NULL);

    return ret;
}


static void elapsedTimeSec(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_elapsedTimeSec;
    SetResult(&result, R_NUMBER, &d);
}


static void totalTimeSec(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_totalTimeSec;
    SetResult(&result, R_NUMBER, &d);
}

static void bitRate(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_bitRate;
    SetResult(&result, R_NUMBER, &d);
}


static void getRepeatInt(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_repeatEnabled;
    SetResult(&result, R_NUMBER, &d);
}


static void getRandomInt(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_randomEnabled;
    SetResult(&result, R_NUMBER, &d);
}

static void getArtist(RESULT * result)
{
    int cSong = mpd_update();
    if (cSong != 0) {		/* inform user this information is cached... */
//	debug("[MPD] use cached artist information...");
    }
    SetResult(&result, R_STRING, artist);
}

static void getTitle(RESULT * result)
{
    int cSong = mpd_update();
    if (cSong != 0) {		/* inform user this information is cached... */
//	debug("[MPD] use cached title information...");
    }
    SetResult(&result, R_STRING, title);
}

static void getAlbum(RESULT * result)
{
    int cSong = mpd_update();
    if (cSong != 0) {		/* inform user this information is cached... */
//	debug("[MPD] use cached album information...");
    }
    SetResult(&result, R_STRING, album);
}

static void getFilename(RESULT * result)
{
    int cSong = mpd_update();
    if (cSong != 0) {		/* inform user this information is cached... */
//	debug("[MPD] use cached filename information...");
    }
    SetResult(&result, R_STRING, filename);
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
    case MPD_PLAYER_PLAY:
	ret = 1;
	break;
    case MPD_PLAYER_PAUSE:
	ret = 2;
	break;
    case MPD_PLAYER_STOP:
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
    d = (double)l_volume;
    /* return 0..100 or < 0 when failed */
    SetResult(&result, R_NUMBER, &d);
}

/* return the # of songs in thr mpd db .. */
static void getSongsInDb(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_songsInDb;
    /* return 0..100 or < 0 when failed */
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdUptime(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_mpdUptime;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdPlayTime(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_mpdPlaytime;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdDbPlayTime(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_mpdDbPlaytime;
    SetResult(&result, R_NUMBER, &d);
}

static void getMpdPlaylistLength(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_mpdPlaylistLength;
    SetResult(&result, R_NUMBER, &d);
}

static void getCurrentSongPos(RESULT * result)
{
    double d;
    mpd_update();
    d = (double)l_currentSongPos;
    SetResult(&result, R_NUMBER, &d);
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
    debug("[MPD] v0.7, check lcd4linux configuration file...");

    check = configure_mpd();
    if (check)
	debug("[MPD] done");
    else
	debug("[MPD] error!");

    memset(album, 0, TAGLEN);
    memset(artist, 0, TAGLEN);
    memset(title, 0, TAGLEN);
    memset(filename, 0, MAX_PATH);
    
    gettimeofday(&timestamp, NULL);	

    AddFunction("mpd::artist", 0, getArtist);
    AddFunction("mpd::title", 0, getTitle);
    AddFunction("mpd::album", 0, getAlbum);
    AddFunction("mpd::file", 0, getFilename);
    AddFunction("mpd::totalTimeSec", 0, totalTimeSec);
    AddFunction("mpd::elapsedTimeSec", 0, elapsedTimeSec);
    AddFunction("mpd::bitRate", 0, bitRate);
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

    AddFunction("mpd::formatTimeMMSS", 1, formatTimeMMSS);
    AddFunction("mpd::formatTimeDDHHMM", 1, formatTimeDDHHMM);

    return 0;
}


void plugin_exit_mpd(void)
{
    debug("[MPD] disconnect from mpd");
    mpd_free(mi);
}
