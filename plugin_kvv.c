/* $Id: plugin_kvv.c,v 1.5 2006/08/16 14:18:14 reinelt Exp $
 *
 * plugin kvv (karlsruher verkehrsverbund)
 *
 * Copyright (C) 2006 Till Harbaum <till@harbaum.org>
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
 *
 * $Log: plugin_kvv.c,v $
 * Revision 1.5  2006/08/16 14:18:14  reinelt
 * T6963 enhancements: soft timing, DualScan, Cell size
 *
 * Revision 1.4  2006/08/15 17:28:27  harbaum
 * Cleaned up thread and error handling
 *
 * Revision 1.3  2006/08/14 19:24:22  harbaum
 * Umlaut support, added KVV HTTP-User-Agent
 *
 * Revision 1.2  2006/08/13 18:45:25  harbaum
 * Little cleanup ...
 *
 * Revision 1.1  2006/08/13 18:14:03  harbaum
 * Added KVV plugin
 *
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_kvv (void)
 *  adds various functions
 * void plugin_exit_kvv (void)
 *
 */

/* define the include files you need */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

/* network specific includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"
#include "cfg.h"
#include "thread.h"

/* these can't be configured as it doesn't make sense to change them */
#define HTTP_SERVER "www.init-ka.de"
#define HTTP_REQUEST "/webfgi/StopInfoInplace.aspx?ID=%s"
#define USER_AGENT   "lcd4linux - KVV plugin (http://ssl.bulix.org/projects/lcd4linux/wiki/plugin_kvv)"

#define DEFAULT_STATION_ID    "89"	// Hauptbahnhof

/* example ids: 
 * 89     = Hauptbahnhof
 * 12_701 = Berufsakademie
 */

/* total max values to calculate shm size */
#define MAX_LINES           4
#define MAX_LINE_LENGTH     8
#define MAX_STATION_LENGTH 32

typedef struct {
    char line[MAX_LINE_LENGTH + 1];
    char station[MAX_STATION_LENGTH + 1];
    int time;
} kvv_entry_t;

typedef struct {
    int entries, error;
    kvv_entry_t entry[MAX_LINES];
} kvv_shm_t;

static char *station_id = NULL;
static char *proxy_name = NULL;
static int port = 80;
static pid_t pid = -1;
static int refresh = 60;

static int initialized = 0;
static int mutex = 0;
static int shmid = -1;
static kvv_shm_t *shm = NULL;

#define SECTION   "Plugin:KVV"

#define TIMEOUT_SHORT 1		/* wait this long for additional data */
#define TIMEOUT_LONG  10	/* wait this long for initial data */

/* search an element in the result string */
static int get_element(char *input, char *name, char **data)
{
    int skip = 0;
    int len = 0;
    int state = 0;		// nothing found yet

    // search entire string
    while (*input) {
	if (skip == 0) {
	    switch (state) {
	    case 0:
		if (*input == '<')
		    state = 1;
		else
		    state = 0;
		break;

	    case 1:
		// ignore white spaces
		if (*input != ' ') {
		    if (strncasecmp(input, name, strlen(name)) == 0) {
			state = 2;
			skip = strlen(name) - 1;
		    } else
			state = 0;
		}
		break;

	    case 2:
		if (*input == ' ') {
		    *data = ++input;
		    while (*input++ != '>')
			len++;

		    return len;
		} else
		    state = 0;
		break;
	    }
	} else if (skip)
	    skip--;

	input++;
    }
    return -1;
}

/* serach an attribute within an element */
static int get_attrib(char *input, char *name, char **data)
{
    int skip = 0;
    int len = 0;
    int state = 0;		// nothing found

    // search in this element
    while (*input != '>') {
	// ignore white spaces
	if (((*input != ' ') && (*input != '\t')) && (skip == 0)) {
	    switch (state) {
	    case 0:
		if (strncasecmp(input, name, strlen(name)) == 0) {
		    state = 1;
		    skip = strlen(name) - 1;
		}
		break;

	    case 1:
		if (*input == '=')
		    state = 2;
		else
		    state = 0;
		break;

	    case 2:
		if (*input == '\"') {
		    *data = ++input;
		    while (*input++ != '\"')
			len++;

		    return len;
		} else
		    state = 0;

		break;
	    }
	} else if (skip)
	    skip--;

	input++;
    }
    return -1;
}

static int http_open(char *name)
{
    struct sockaddr_in server;
    struct hostent *host_info;
    unsigned long addr;
    int sock;

    /* create socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror("failed to create socket");
	return -1;
    }

    /* Erzeuge die Socketadresse des Servers 
     * Sie besteht aus Typ, IP-Adresse und Portnummer */
    memset(&server, 0, sizeof(server));
    if ((addr = inet_addr(name)) != INADDR_NONE) {
	memcpy((char *) &server.sin_addr, &addr, sizeof(addr));
    } else {
	/* Wandle den Servernamen in eine IP-Adresse um */
	host_info = gethostbyname(name);
	if (NULL == host_info) {
	    error("[KVV] Unknown server: %s", name);
	    return -1;
	}
	memcpy((char *) &server.sin_addr, host_info->h_addr, host_info->h_length);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    /* Baue die Verbindung zum Server auf */
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
	perror("can't connect to server");
	return -1;
    }

    return sock;
}

static void get_text(char *input, char *end, char *dest, int dlen)
{
    int state = 0;		// nothing yet, outside any element
    int cnt = 0;

    while (*input) {
	switch (state) {
	case 0:
	    if (*input == '<')
		state = 1;
	    else {
		if (cnt < (dlen - 1))
		    dest[cnt++] = *input;
	    }
	    break;

	case 1:
	    if (*input == '/')
		state = 2;
	    else if (*input == '>')
		state = 0;
	    break;

	case 2:
	    if (strncasecmp(input, end, strlen(end)) == 0) {
		dest[cnt++] = 0;
		return;
	    }
	    break;
	}

	input++;
    }

}

static void process_station_string(char *str)
{
    char *p, *q;
    int last, i;

    /* some strings to replace */
    char *repl[] = {
	"Hauptbahnhof", "Hbf.",
	"Bahnhof", "Bhf.",
	"Karlsruhe", "KA",
	"Schienenersatzverkehr", "Ersatzv.",
	"Marktplatz", "Marktpl.",
    };

    /* erase multiple spaces and replace umlauts */
    p = q = str;
    last = 1;			// no leading spaces
    while (*p) {
	if ((!last) || (*p != ' ')) {

	    /* translate from latin1 to hd44780 */
	    if (*p == (char) 228)	// lower a umlaut
		*q++ = 0xe1;
	    else if (*p == (char) 223)	// sz ligature
		*q++ = 0xe2;
	    else if (*p == (char) 246)	// lower o umlaut
		*q++ = 0xef;
	    else if (*p == (char) 252)	// lower u umlaut
		*q++ = 0xf5;
	    else
		*q++ = *p;
	}

	last = (*p == ' ');
	p++;
    }
    *q++ = 0;

    /* decode utf8 */
    p = q = str;
    last = 0;
    while (*p) {
	if (last) {
	    *q++ = (last << 6) | (*p & 0x3f);
	    last = 0;
	} else if ((*p & 0xe0) == 0xc0) {
	    last = *p & 3;
	} else
	    *q++ = *p;

	p++;
    }
    *q++ = 0;

    /* replace certain (long) words with e.g. abbreviations */
    for (i = 0; i < (int) (sizeof(repl) / (2 * sizeof(char *))); i++) {
	if ((p = strstr(str, repl[2 * i])) != NULL) {

	    /* move new string */
	    memcpy(p, repl[2 * i + 1], strlen(repl[2 * i + 1]));
	    /* move rest of string down */
	    memmove(p + strlen(repl[2 * i + 1]),
		    p + strlen(repl[2 * i]), strlen(str) - (p - str) - strlen(repl[2 * i]) + 1);
	}
    }
}

static void kvv_client( __attribute__ ((unused))
		       void *dummy)
{
    char ibuffer[4096];
    char obuffer[1024];
    int count, i, sock;

    char server_name[] = HTTP_SERVER;
    char *connect_to;

    /* connect to proxy if given, to server otherwise */
    if ((proxy_name != NULL) && (strlen(proxy_name) != 0))
	connect_to = proxy_name;
    else
	connect_to = server_name;

    info("[KVV] Connecting to %s", connect_to);

    while (1) {

	sock = http_open(connect_to);
	if (sock < 0) {
	    error("[KVV] Error accessing server/proxy: %s", strerror(errno));
	    return;
	}
	// create and set get request
	if (snprintf(obuffer, sizeof(obuffer),
		     "GET http://%s" HTTP_REQUEST " HTTP/1.1\n"
		     "Host: %s\n" "User-Agent: " USER_AGENT "\n\n", server_name, station_id,
		     server_name) >= sizeof(obuffer)) {

	    info("[KVV] Warning, request has been truncated!");
	}

	info("[KVV] Sending first (GET) request ...");
	send(sock, obuffer, strlen(obuffer), 0);

	count = 0;
	do {
	    fd_set rfds;
	    struct timeval tv;

	    FD_ZERO(&rfds);
	    FD_SET(sock, &rfds);

	    tv.tv_sec = count ? TIMEOUT_SHORT : TIMEOUT_LONG;
	    tv.tv_usec = 0;

	    i = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
	    if (i < 0) {
		perror("select");
		exit(1);
	    }

	    if (i != 0) {
		i = recv(sock, ibuffer + count, sizeof(ibuffer) - count - 1, 0);
		count += i;
	    }
	}
	while (i > 0);

	ibuffer[count] = 0;	// terminate string
	close(sock);

	if (count > 0) {
	    char *input, *cookie, *name, *value;
	    int input_len, cookie_len, name_len, value_len;

	    // buffer to html encode value
	    char value_enc[512];
	    int value_enc_len;

	    // find cookie
	    cookie_len = 0;
	    cookie = strstr(ibuffer, "Set-Cookie:");
	    if (cookie) {
		cookie += strlen("Set-Cookie:");

		while (*cookie == ' ')
		    cookie++;

		while (cookie[cookie_len] != ';')
		    cookie_len++;
	    }
	    // find input element
	    input_len = get_element(ibuffer, "input", &input);


	    if (input_len > 0) {
		char *input_end = input;
		while (*input_end != '>')
		    input_end++;
		while (*input_end != '\"')
		    input_end--;
		*(input_end + 1) = 0;

		name_len = get_attrib(input, "name", &name);
		value_len = get_attrib(input, "value", &value);

		for (value_enc_len = 0, i = 0; i < value_len; i++) {
		    if (isalnum(value[i]))
			value_enc[value_enc_len++] = value[i];
		    else {
			sprintf(value_enc + value_enc_len, "%%%02X", 0xff & value[i]);
			value_enc_len += 3;
		    }
		}

		if (cookie_len >= 0)
		    cookie[cookie_len] = 0;
		if (name_len >= 0)
		    name[name_len] = 0;
		if (value_len >= 0)
		    value[value_len] = 0;
		if (value_enc_len >= 0)
		    value_enc[value_enc_len] = 0;

		sock = http_open(connect_to);

		// send POST
		if (snprintf(obuffer, sizeof(obuffer),
			     "POST http://%s" HTTP_REQUEST " HTTP/1.1\n"
			     "Host: %s\n"
			     "User-Agent: " USER_AGENT "\n"
			     "Cookie: %s\n"
			     "Content-Type: application/x-www-form-urlencoded\n"
			     "Content-Length: %d\n"
			     "\n%s=%s",
			     server_name, station_id, server_name, cookie, name_len + value_enc_len + 1, name,
			     value_enc) >= sizeof(obuffer)) {

		    info("[KVV] Warning, request has been truncated!");
		}

		info("[KVV] Sending second (POST) request ...");
		send(sock, obuffer, strlen(obuffer), 0);

		count = 0;
		do {
		    fd_set rfds;
		    struct timeval tv;

		    FD_ZERO(&rfds);
		    FD_SET(sock, &rfds);

		    tv.tv_sec = count ? TIMEOUT_SHORT : TIMEOUT_LONG;
		    tv.tv_usec = 0;

		    i = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
		    if (i > 0) {
			i = recv(sock, ibuffer + count, sizeof(ibuffer) - count - 1, 0);
			count += i;
		    }
		}
		while (i > 0);	/* leave on select or read error */

		ibuffer[count] = 0;

//      printf("Result (%d):\n%s\n", count, ibuffer);

		/* close connection */
		close(sock);

		if (count > 0) {
		    int last_was_stop = 0;
		    char *td = ibuffer;
		    char str[32];
		    int td_len, i, overflow = 0;

		    /* lock shared memory */
		    mutex_lock(mutex);

		    /* free allocated memory */
		    shm->entries = 0;

		    if (strstr(ibuffer, "Die Daten konnten nicht abgefragt werden.") != NULL) {
			info("[KVV] Server returned error!");
			shm->error = 1;
		    } else
			shm->error = 0;

		    /* scan through all <td> entries and search the line nums */
		    do {
			if ((td_len = get_element(td, "td", &td)) > 0) {
			    char *attr, *p;
			    int attr_len;

			    /* time does not have a class but comes immediately after stop :-( */
			    if (last_was_stop) {
				td += td_len + 1;
				get_text(td, "td", str, sizeof(str));

				/* time needs special treatment */
				if (strncasecmp(str, "sofort", strlen("sofort")) == 0)
				    i = 0;
				else {
				    /* skip everything that is not a number */
				    p = str;
				    while (!isdigit(*p))
					p++;

				    /* and convert remaining to number */
				    i = atoi(p);
				}

				/* save time */
				if (!overflow)
				    shm->entry[shm->entries - 1].time = i;

				last_was_stop = 0;
			    }

			    /* linenum and stopname fields have proper classes */
			    if ((attr_len = get_attrib(td, "class", &attr)) > 0) {

				if (strncasecmp(attr, "lineNum", strlen("lineNum")) == 0) {
				    td += td_len + 1;
				    get_text(td, "td", str, sizeof(str));

				    if (shm->entries < MAX_LINES) {
					// allocate a new slot
					shm->entries++;
					shm->entry[shm->entries - 1].time = -1;
					memset(shm->entry[shm->entries - 1].line, 0, MAX_LINE_LENGTH + 1);
					memset(shm->entry[shm->entries - 1].station, 0, MAX_STATION_LENGTH + 1);

					// add new lines entry
					strncpy(shm->entry[shm->entries - 1].line, str, MAX_LINE_LENGTH);
				    } else
					overflow = 1;	/* don't add further entries */
				}

				if (strncasecmp(attr, "stopname", strlen("stopname")) == 0) {
				    td += td_len + 1;
				    get_text(td, "td", str, sizeof(str));


				    /* stopname may need further tuning */
				    process_station_string(str);

				    if (!overflow)
					strncpy(shm->entry[shm->entries - 1].station, str, MAX_STATION_LENGTH);

				    last_was_stop = 1;
				}
			    }
			}
		    } while (td_len >= 0);

		    mutex_unlock(mutex);
		}
	    }
	}

	sleep(refresh);
    }
}

static int kvv_fork(void)
{
    if (initialized)
	return 0;

    info("[KVV] creating client thread");

    /* set this here to prevent continous retries if init fails */
    initialized = 1;

    /* create communication buffer */
    shmid = shm_create((void **) &shm, sizeof(kvv_shm_t));

    /* catch error */
    if (shmid < 0) {
	error("[KVV] Shared memory allocation failed!");
	return -1;
    }

    /* attach client thread */
    mutex = mutex_create();
    pid = thread_create("plugin_kvv", kvv_client, NULL);

    if (pid < 0) {
	error("[KVV] Unable to fork client: %s", strerror(errno));
	return -1;
    }

    info("[KVV] forked client with pid %d", pid);
    return 0;
}

static void kvv_line(RESULT * result, RESULT * arg1)
{
    int index = (int) R2N(arg1);

    if (kvv_fork() != 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    mutex_lock(mutex);

    if (index < shm->entries) {
	SetResult(&result, R_STRING, shm->entry[index].line);
    } else
	SetResult(&result, R_STRING, "");

    mutex_unlock(mutex);
}

static void kvv_station(RESULT * result, RESULT * arg1)
{
    int index = (int) R2N(arg1);

    if (kvv_fork() != 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    mutex_lock(mutex);

    if (shm->error && index == 0)
	SetResult(&result, R_STRING, "Server Err");
    else {
	if (index < shm->entries)
	    SetResult(&result, R_STRING, shm->entry[index].station);
	else
	    SetResult(&result, R_STRING, "");
    }

    mutex_unlock(mutex);
}

static void kvv_time(RESULT * result, RESULT * arg1)
{
    int index = (int) R2N(arg1);
    double value = -1.0;

    if (kvv_fork() != 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    mutex_lock(mutex);

    if (index < shm->entries)
	value = shm->entry[index].time;

    SetResult(&result, R_NUMBER, &value);

    mutex_unlock(mutex);
}

static void kvv_time_str(RESULT * result, RESULT * arg1)
{
    int index = (int) R2N(arg1);

    if (kvv_fork() != 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    mutex_lock(mutex);

    if (index < shm->entries) {
	char str[8];
	sprintf(str, "%d", shm->entry[index].time);
	SetResult(&result, R_STRING, str);
    } else
	SetResult(&result, R_STRING, "");

    mutex_unlock(mutex);
}

/* plugin initialization */
int plugin_init_kvv(void)
{
    int val;
    char *p;

    /* register all our cool functions */
    AddFunction("kvv::line", 1, kvv_line);
    AddFunction("kvv::station", 1, kvv_station);
    AddFunction("kvv::time", 1, kvv_time);
    AddFunction("kvv::time_str", 1, kvv_time_str);

    /* parse parameter */
    if ((p = cfg_get(SECTION, "StationID", DEFAULT_STATION_ID)) != NULL) {
	station_id = malloc(strlen(p) + 1);
	strcpy(station_id, p);
    }
    info("[KVV] Using station %s", station_id);

    if ((p = cfg_get(SECTION, "Proxy", NULL)) != NULL) {
	proxy_name = malloc(strlen(p) + 1);
	strcpy(proxy_name, p);
	info("[KVV] Using proxy \"%s\"", proxy_name);
    }

    if (cfg_number(SECTION, "Port", 0, 0, 65535, &val) > 0) {
	port = val;
	info("[KVV] Using port %d", port);
    } else {
	info("[KVV] Using default port %d", port);
    }

    if (cfg_number(SECTION, "Refresh", 0, 0, 65535, &val) > 0) {
	refresh = val;
	info("[KVV] Using %d seconds refresh interval", refresh);
    } else {
	info("[KVV] Using default refresh interval of %d seconds", refresh);
    }

    return 0;
}

void plugin_exit_kvv(void)
{
    /* kill client thread if it's running */
    if (initialized) {
	/* kill client */
	if (pid != -1)
	    thread_destroy(pid);

	/* free shared mem and its mutex */
	if (shm) {
	    shm_destroy(shmid, shm);
	    mutex_destroy(mutex);
	}
    }

    if (station_id)
	free(station_id);
    if (proxy_name)
	free(proxy_name);
}
