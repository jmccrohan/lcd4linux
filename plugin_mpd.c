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

#include <locale.h>
#include <langinfo.h>
#include <iconv.h>

/* source: http://www.musicpd.org/libmpdclient.shtml */
#include "mpd/client.h"

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
static int l_singleEnabled;
static int l_consumeEnabled;
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

static struct mpd_song *currentSong;

/* connection information */
static char host[255];
static char pw[255];
static int iport;
static int plugin_enabled;
static int waittime;
struct timeval timestamp;

static struct mpd_connection *conn;
static char Section[] = "Plugin:MPD";
static int errorcnt = 0;

static iconv_t char_conv_iconv;
static char *char_conv_to;
static char *char_conv_from;

#define BUFFER_SIZE	1024

static void charset_close(void)
{
    if (char_conv_to) {
	iconv_close(char_conv_iconv);
	free(char_conv_to);
	free(char_conv_from);
	char_conv_to = NULL;
	char_conv_from = NULL;
    }
}

static int charset_set(const char *to, const char *from)
{
    if (char_conv_to && strcmp(to, char_conv_to) == 0 && char_conv_from && strcmp(from, char_conv_from) == 0)
	return 0;

    charset_close();

    if ((char_conv_iconv = iconv_open(to, from)) == (iconv_t) (-1))
	return -1;

    char_conv_to = strdup(to);
    char_conv_from = strdup(from);
    return 0;
}

static inline size_t deconst_iconv(iconv_t cd,
				   const char **inbuf, size_t * inbytesleft, char **outbuf, size_t * outbytesleft)
{
    union {
	const char **a;
	char **b;
    } deconst;

    deconst.a = inbuf;

    return iconv(cd, deconst.b, inbytesleft, outbuf, outbytesleft);
}

static char *charset_conv_strdup(const char *string)
{
    char buffer[BUFFER_SIZE];
    size_t inleft = strlen(string);
    char *ret;
    size_t outleft;
    size_t retlen = 0;
    size_t err;
    char *bufferPtr;

    if (!char_conv_to)
	return NULL;

    ret = strdup("");

    while (inleft) {
	bufferPtr = buffer;
	outleft = BUFFER_SIZE;
	err = deconst_iconv(char_conv_iconv, &string, &inleft, &bufferPtr, &outleft);
	if (outleft == BUFFER_SIZE || (err == (size_t) - 1 /* && errno != E2BIG */ )) {
	    free(ret);
	    return NULL;
	}

	ret = realloc(ret, retlen + BUFFER_SIZE - outleft + 1);
	memcpy(ret + retlen, buffer, BUFFER_SIZE - outleft);
	retlen += BUFFER_SIZE - outleft;
	ret[retlen] = '\0';
    }

    return ret;
}

const char *charset_from_utf8(const char *from)
{
    static char *to = NULL;

    if (to)
	free(to);

    charset_set("ISO−8859−1", "UTF-8");
    to = charset_conv_strdup(from);

    if (to == NULL)
	return from;

    return to;
}

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
    if (!s || *s == '\0') {
	info("[MPD] empty '%s.server' entry from %s, assuming 'localhost'", Section, cfg_source());
	strcpy(host, "localhost");
    } else
	strcpy(host, s);
    if (s)
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
    if (!s || *s == '\0') {
	info("[MPD] empty '%s.password' entry in %s, assuming none", Section, cfg_source());
	memset(pw, 0, sizeof(pw));
    } else
	strcpy(pw, s);
    if (s)
	free(s);

    debug("[MPD] connection detail: [%s:%d]", host, iport);
    configured = 1;
    return configured;
}


static void mpd_printerror(const char *cmd)
{
    const char *s;
    if (conn) {
	//assert(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS);

	s = mpd_connection_get_error_message(conn);
	if (mpd_connection_get_error(conn) == MPD_ERROR_SERVER)
	    /* messages received from the server are UTF-8; the
	       rest is either US-ASCII or locale */
	    s = charset_from_utf8(s);

	error("[MPD] %s to [%s]:[%i] failed : [%s]", cmd, host, iport, s);
	mpd_connection_free(conn);
	conn = NULL;
    }
}

void mpd_query_status(struct mpd_connection *conn)
{
    struct mpd_status *status;
    struct mpd_song *song;
    const struct mpd_audio_format *audio;

    if (!conn)
	return;

    if (!mpd_command_list_begin(conn, true) ||
	!mpd_send_status(conn) || !mpd_send_current_song(conn) || !mpd_command_list_end(conn)) {
	mpd_printerror("queue_commands");
	return;
    }

    status = mpd_recv_status(conn);
    if (status == NULL) {
	mpd_printerror("recv_status");
	return;
    }
    if (currentSong != NULL) {
	mpd_song_free(currentSong);
	currentSong = NULL;
    }

    if (!mpd_response_next(conn)) {
	mpd_printerror("response_next");
	return;
    }

    song = mpd_recv_song(conn);
    if (song != NULL) {
	currentSong = mpd_song_dup(song);
	mpd_song_free(song);

	l_elapsedTimeSec = mpd_status_get_elapsed_time(status);
	l_totalTimeSec = mpd_status_get_total_time(status);
	l_bitRate = mpd_status_get_kbit_rate(status);
    } else {
	l_elapsedTimeSec = 0;
	l_totalTimeSec = 0;
	l_bitRate = 0;
    }
    l_state = mpd_status_get_state(status);

    l_repeatEnabled = mpd_status_get_repeat(status);
    l_randomEnabled = mpd_status_get_random(status);
    l_singleEnabled = mpd_status_get_single(status);
    l_consumeEnabled = mpd_status_get_consume(status);

    l_volume = mpd_status_get_volume(status);

    l_currentSongPos = mpd_status_get_song_pos(status) + 1;
    l_playlistLength = mpd_status_get_queue_length(status);


    audio = mpd_status_get_audio_format(status);
    if (audio) {
	l_sampleRate = audio->sample_rate;
	l_channels = audio->channels;
    } else {
	l_sampleRate = 0;
	l_channels = 0;
    }

    if (mpd_status_get_error(status) != NULL)
	error("[MPD] query status : %s", charset_from_utf8(mpd_status_get_error(status)));

    mpd_status_free(status);

    if (!mpd_response_finish(conn)) {
	mpd_printerror("response_finish");
	return;
    }
}

void mpd_query_stats(struct mpd_connection *conn)
{
    struct mpd_stats *stats;

    if (!conn)
	return;

    if (!mpd_command_list_begin(conn, true) || !mpd_send_stats(conn) || !mpd_command_list_end(conn)) {
	mpd_printerror("queue_commands");
	return;
    }

    stats = mpd_recv_stats(conn);
    if (stats == NULL) {
	mpd_printerror("recv_stats");
	return;
    }

    l_numberOfSongs = mpd_stats_get_number_of_songs(stats);
    l_uptime = mpd_stats_get_uptime(stats);
    l_playTime = mpd_stats_get_play_time(stats);
    l_dbPlayTime = mpd_stats_get_db_play_time(stats);

    mpd_stats_free(stats);

    if (!mpd_response_finish(conn)) {
	mpd_printerror("response_finish");
	return;
    }
}

static int mpd_update()
{
    struct timeval now;

    /* reread every 1000 msec only */
    gettimeofday(&now, NULL);
    int timedelta = (now.tv_sec - timestamp.tv_sec) * 1000 + (now.tv_usec - timestamp.tv_usec) / 1000;

    if (timedelta > 0 && timedelta < waittime) {
	/* debug("[MPD] waittime not reached...\n"); */
	return 1;
    }

    /* check if configured */
    if (configure_mpd() < 0) {
	return -1;
    }

    /* check if connected */
    if (conn == NULL || mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
	if (conn) {
	    if (errorcnt < ERROR_DISPLAY)
		mpd_printerror("reconnect");
	} else
	    debug("[MPD] initialize connect to [%s]:[%i]", host, iport);
    }
    if (!conn) {
	conn = mpd_connection_new(host, iport, TIMEOUT_IN_S * 1000);
	if (conn == NULL || mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
	    if (conn) {
		if (errorcnt < ERROR_DISPLAY)
		    mpd_printerror("connect");
	    }
	    if (errorcnt == ERROR_DISPLAY)
		error("[MPD] stop logging, until connection is fixed!");
	    errorcnt++;
	    gettimeofday(&timestamp, NULL);
	    return -1;
	}

	if (*pw && !mpd_run_password(conn, pw)) {
	    errorcnt++;
	    mpd_printerror("run_password");
	    return -1;
	}
	errorcnt = 0;
	debug("[MPD] connection fixed...");
    }

    mpd_query_status(conn);
    mpd_query_stats(conn);

    gettimeofday(&timestamp, NULL);
    return 1;
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

static void getSingleInt(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_singleEnabled;
    SetResult(&result, R_NUMBER, &d);
}

static void getConsumeInt(RESULT * result)
{
    double d;
    mpd_update();
    d = (double) l_consumeEnabled;
    SetResult(&result, R_NUMBER, &d);
}

/* if no tag is availabe, use filename */
static void getArtist(RESULT * result)
{
    const char *value = NULL;
    mpd_update();
    if (currentSong != NULL) {
	value = mpd_song_get_tag(currentSong, MPD_TAG_ARTIST, 0);
	if (!value) {
	    value = mpd_song_get_tag(currentSong, MPD_TAG_ALBUM_ARTIST, 0);
	}
	if (!value) {
	    value = mpd_song_get_uri(currentSong);
	}
    }
    if (value)
	SetResult(&result, R_STRING, charset_from_utf8(value));
    else
	SetResult(&result, R_STRING, "");
}

static void getTitle(RESULT * result)
{
    const char *value = NULL;
    mpd_update();
    if (currentSong != NULL) {
	value = mpd_song_get_tag(currentSong, MPD_TAG_TITLE, 0);
    }
    if (value)
	SetResult(&result, R_STRING, charset_from_utf8(value));
    else
	SetResult(&result, R_STRING, "");
}

static void getAlbum(RESULT * result)
{
    const char *value = NULL;
    mpd_update();
    if (currentSong != NULL) {
	value = mpd_song_get_tag(currentSong, MPD_TAG_ALBUM, 0);
    }
    if (value)
	SetResult(&result, R_STRING, charset_from_utf8(value));
    else
	SetResult(&result, R_STRING, "");
}

static void getFilename(RESULT * result)
{
    const char *value = NULL;
    mpd_update();
    if (currentSong != NULL) {
	value = mpd_song_get_uri(currentSong);
    }
    if (value)
	SetResult(&result, R_STRING, charset_from_utf8(value));
    else
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
    case MPD_STATE_PLAY:
	ret = 1;
	break;
    case MPD_STATE_PAUSE:
	ret = 2;
	break;
    case MPD_STATE_STOP:
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
	if ((!mpd_run_next(conn))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_next");
	}
    }
}

static void prevSong()
{
    mpd_update();
    if (currentSong != NULL) {
	if ((!mpd_run_previous(conn))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_previous");
	}
    }
}

static void stopSong()
{
    mpd_update();
    if (currentSong != NULL) {
	if ((!mpd_run_stop(conn))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_stop");
	}
    }
}

static void pauseSong()
{
    mpd_update();
    if (currentSong != NULL) {
	if ((!mpd_send_pause(conn, l_state == MPD_STATE_PAUSE ? 0 : 1))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("send_pause");
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

	if ((!mpd_run_set_volume(conn, l_volume))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("set_volume");
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

	if ((!mpd_run_set_volume(conn, l_volume))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("set_volume");
	}
    }
}

static void toggleRepeat()
{
    mpd_update();
    if (currentSong != NULL) {
	l_repeatEnabled = !l_repeatEnabled;
	if ((!mpd_run_repeat(conn, l_repeatEnabled))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_repeat");
	}
    }
}

static void toggleRandom()
{
    mpd_update();
    if (currentSong != NULL) {
	l_randomEnabled = !l_randomEnabled;
	if ((!mpd_run_random(conn, l_randomEnabled))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_random");
	}
    }
}

static void toggleSingle()
{
    mpd_update();
    if (currentSong != NULL) {
	l_singleEnabled = !l_singleEnabled;
	if ((!mpd_run_single(conn, l_singleEnabled))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_single");
	}
    }
}

static void toggleConsume()
{
    mpd_update();
    if (currentSong != NULL) {
	l_consumeEnabled = !l_consumeEnabled;
	if ((!mpd_run_consume(conn, l_consumeEnabled))
	    || (!mpd_response_finish(conn))) {
	    mpd_printerror("run_consume");
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
    AddFunction("mpd::getSingleInt", 0, getSingleInt);
    AddFunction("mpd::getConsumeInt", 0, getConsumeInt);
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
    AddFunction("mpd::cmdToggleSingle", 0, toggleSingle);
    AddFunction("mpd::cmdToggleConsume", 0, toggleConsume);

    AddFunction("mpd::formatTimeMMSS", 1, formatTimeMMSS);
    AddFunction("mpd::formatTimeDDHHMM", 1, formatTimeDDHHMM);

    return 0;
}


void plugin_exit_mpd(void)
{
    if (plugin_enabled == 1) {
	debug("[MPD] disconnect from mpd");
	if (currentSong != NULL)
	    mpd_song_free(currentSong);
    }
    if (conn != NULL)
	mpd_connection_free(conn);
    charset_close();
}
