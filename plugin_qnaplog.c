/* $Id: plugin_sample.c 733 2007-01-15 05:47:13Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/branches/0.10.1/plugin_qnaplog.c $
 *
 * plugin qnaplog
 *
 * Copyright (C) 2009 Ralf Tralow <qnaplog@ringwelt.de>
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

/* DOC:
 * To compile the plugin remember to add -lsqlite3 option into Makefile. I.E: CC = gcc -lsqlite3
 * or run:
 * $ gcc -I/opt/include -L/opt/lib/sqlite3 plugin_qnaplog.c -lsqlite3 -o plugin_qnaplog.o
 *
 *
 *  exported functions:
 *
 *  int plugin_init_qnaplog (void)
 *  void plugin_exit_qnaplog(void)
 *  static void my_status(RESULT * result, RESULT *arg1)
 *  static void my_conn(RESULT *result, RESULT *arg1)
 *  static void my_event(RESULT *result, RESULT *arg1)
 *
 *
 *  adds various functions
 *
 */

/* define the include files you need */
#include "config.h"
#include "cfg.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_SQLITE3_H
#include <sqlite3.h>
#else
#warning sqlite3.h not found: plugin deactivated
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#ifdef HAVE_SQLITE3_H

#define SQLSTATEMENT_CONN    "select * from NASLOG_CONN ORDER BY conn_id DESC LIMIT 1"
#define SQLSTATEMENT_EVENT   "select * from NASLOG_EVENT ORDER BY event_id DESC LIMIT 1"

time_t lastaccesstimeConn = 0;
time_t lastaccesstimeEvent = 0;
sqlite3 *connexConn;
sqlite3 *connexEvent;
char dbNameConn[256];
char dbNameEvent[256];

char conn_id[10];
char conn_type[12];
char conn_date[12];
char conn_time[12];
char conn_user[30];
char conn_ip[16];
char conn_comp[30];
char conn_res[255];
char conn_serv[10];
char conn_action[12];

char event_id[10];
char event_comp[30];
char event_date[12];
char event_ip[16];
char event_time[12];
char event_type[12];
char event_user[30];
char event_desc[255];

#define MAX_IDS_TYPE	3
char *IDS_TYPE[MAX_IDS_TYPE] =
{	"Information", "Warning", "Error"};
#define MAX_IDS_SERV	8
char *IDS_SERV[MAX_IDS_SERV] =
{	"S0","Samba","S2","HTTP","S4","S5","S6","SSH"};
#define MAX_IDS_ACTION	16
char *IDS_ACTION[MAX_IDS_ACTION] =
{	"C0","Delete","Read","Write","C4","C5","C6","C7","C8","Login fail","Login ok","Logout","C12","C13","C14","Add"};

static char Section[] = "Plugin:QnapLog";


/** test the connection to conn.log
 *
 */
static int configureConn(void)
{
	static int configured = 0;
	char *s;
	int rc;

	if (configured != 0)
	return configured;

	s = cfg_get(Section, "databaseConn", "");
	if (*s == '\0')
	{
		info("[QnapLog] empty '%s.database' entry in %s, assuming none", Section, cfg_source());
		strcpy(dbNameConn, "/etc/logs/conn.log");
	}
	else
	{
		snprintf(dbNameConn, sizeof(dbNameConn), "%s", s);
	}
	free(s);

	rc = sqlite3_open(dbNameConn, &connexConn);
	if (rc)
	{
		error("[QnapLog] connection error: %s", sqlite3_errmsg(connexConn));
		configured = -1;
		sqlite3_close(connexConn);
		return configured;
	}

	configured = 1;
	return configured;
}


/** test the connection to event.log
 *
 */
static int configureEvent(void)
{
	static int configured = 0;
	char *s;
	int rc;

	if (configured != 0)
	return configured;

	s = cfg_get(Section, "databaseEvent", "");
	if (*s == '\0')
	{
		info("[QnapLog] empty '%s.database' entry in %s, assuming none", Section, cfg_source());
		strcpy(dbNameEvent, "/etc/logs/event.log");
	}
	else
	{
		snprintf(dbNameEvent, sizeof(dbNameEvent), "%s", s);
	}
	free(s);

	rc = sqlite3_open(dbNameEvent, &connexEvent);
	if (rc)
	{
		error("[QnapLog] connection error: %s", sqlite3_errmsg(connexEvent));
		configured = -1;
		sqlite3_close(connexEvent);
		return configured;
	}

	configured = 1;
	return configured;
}


/** callback function for conn request
 *
 */
static int callbackConn(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	int c;

	for (i = 0; i < argc; i++)
	{
		if (strcmp(azColName[i], "conn_id") == 0)
		{
			snprintf(conn_id, sizeof(conn_id), "%s", argv[i] ? argv[i] : "NULL");
		}
		else if (strcmp(azColName[i], "conn_type") == 0)
		{
			c = atoi( argv[i] );
			if( c < MAX_IDS_TYPE )
				snprintf(conn_type, sizeof(conn_type), "%s", IDS_TYPE[c]);
		}
		else if (strcmp(azColName[i], "conn_date") == 0)
		{
			snprintf(conn_date, sizeof(conn_date), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "conn_time") == 0)
		{
			snprintf(conn_time, sizeof(conn_time), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "conn_user") == 0)
		{
			snprintf(conn_user, sizeof(conn_user), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "conn_ip") == 0)
		{
			snprintf(conn_ip, sizeof(conn_ip), "%s", argv[i] ? argv[i] : "NULL");
		}
		else if (strcmp(azColName[i], "conn_comp") == 0)
		{
			snprintf(conn_comp, sizeof(conn_comp), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "conn_res") == 0)
		{
			snprintf(conn_res, sizeof(conn_res), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "conn_serv") == 0)
		{
			c = atoi( argv[i] );
			if( c < MAX_IDS_SERV )
				snprintf(conn_serv, sizeof(conn_serv), "%s", IDS_SERV[c]);
		}
		else if (strcmp(azColName[i], "conn_action") == 0)
		{
			c = atoi( argv[i] );
			if( c < MAX_IDS_ACTION )
				snprintf(conn_action, sizeof(conn_action), "%s", IDS_ACTION[c]);
		}
	}

	return 0;
}


/** callback function for event request
 *
 */
static int callbackEvent(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	int c;

	for (i = 0; i < argc; i++)
	{
		if (strcmp(azColName[i], "event_id") == 0)
		{
			snprintf(event_id, sizeof(event_id), "%s", argv[i] ? argv[i] : "NULL");
		}
		else if (strcmp(azColName[i], "event_type") == 0)
		{
			c = *argv[i] & 0x0F;
			if( c < MAX_IDS_TYPE )
			snprintf(event_type, sizeof(event_type), "%s", IDS_TYPE[c]);
		}
		else if (strcmp(azColName[i], "event_date") == 0)
		{
			snprintf(event_date, sizeof(event_date), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "event_time") == 0)
		{
			snprintf(event_time, sizeof(event_time), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "event_user") == 0)
		{
			snprintf(event_user, sizeof(event_user), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "event_ip") == 0)
		{
			snprintf(event_ip, sizeof(event_ip), "%s", argv[i] ? argv[i] : "NULL");
		}
		else if (strcmp(azColName[i], "event_comp") == 0)
		{
			snprintf(event_comp, sizeof(event_comp), "%s", argv[i] ? argv[i]
					: "NULL");
		}
		else if (strcmp(azColName[i], "event_desc") == 0)
		{
			snprintf(event_desc, sizeof(event_desc), "%s", argv[i] ? argv[i]
					: "NULL");
		}
	}

	return 0;
}


/** request last of qnap connection
 *
 */
static void my_conn(RESULT *result, RESULT *arg1)
{
	char *key;
	char *value;
	char *zErrMsg = 0;
	int rc;
	struct stat attrib; // create a file attribute structure
	time_t accesstime;


	value = -1;
	key = R2S(arg1);

	if (configureConn() >= 0)
	{
		stat(dbNameConn, &attrib); // get the attributes
		accesstime = attrib.st_mtime; // Get the last modified time and put it into the time structure

		if( accesstime > lastaccesstimeConn )
		{
			lastaccesstimeConn = accesstime;
			rc = sqlite3_exec(connexConn, SQLSTATEMENT_CONN, callbackConn, 0, &zErrMsg);
			if (rc != SQLITE_OK)
			{
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			}
		}

		if (strcasecmp(key, "id") == 0)
		{
			value = conn_id;
		}
		else if (strcasecmp(key, "type") == 0)
		{
			value = conn_type;
		}
		else if (strcasecmp(key, "date") == 0)
		{
			value = conn_date;
		}
		else if (strcasecmp(key, "time") == 0)
		{
			value = conn_time;
		}
		else if (strcasecmp(key, "user") == 0)
		{
			value = conn_user;
		}
		else if (strcasecmp(key, "ip") == 0)
		{
			value = conn_ip;
		}
		else if (strncasecmp(key, "res", 3) == 0)
		{
			value = conn_res;
		}
		else if (strncasecmp(key, "serv", 4) == 0)
		{
			value = conn_serv;
		}
		else if (strncasecmp(key, "act", 3) == 0)
		{
			value = conn_action;
		}
		else if (strncasecmp(key, "comp", 4) == 0)
		{
			value = conn_comp;
		}
	}

	/* store result */
	SetResult(&result, R_STRING, value);
}

/** request last qnap event
 *
 */
static void my_event(RESULT *result, RESULT *arg1)
{
	char *key;
	char *value;
	char *zErrMsg = 0;
	int rc;
	struct stat attrib; // create a file attribute structure
	time_t accesstime;


	value = -1;
	key = R2S(arg1);

	if (configureEvent() >= 0)
	{
		stat(dbNameEvent, &attrib); // get the attributes
		accesstime = attrib.st_mtime; // Get the last modified time and put it into the time structure

		if( accesstime > lastaccesstimeEvent )
		{
			lastaccesstimeEvent = accesstime;
			rc = sqlite3_exec(connexEvent, SQLSTATEMENT_EVENT, callbackEvent, 0, &zErrMsg);
			if (rc != SQLITE_OK)
			{
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			}
		}

		if (strcasecmp(key, "id") == 0)
		{
			value = event_id;
		}
		else if (strncasecmp(key, "comp", 4) == 0)
		{
			value = event_comp;
		}
		else if (strcasecmp(key, "date") == 0)
		{
			value = event_date;
		}
		else if (strcasecmp(key, "ip") == 0)
		{
			value = event_ip;
		}
		else if (strcasecmp(key, "time") == 0)
		{
			value = event_time;
		}
		else if (strcasecmp(key, "type") == 0)
		{
			value = event_type;
		}
		else if (strcasecmp(key, "user") == 0)
		{
			value = event_user;
		}
		else if (strncasecmp(key, "desc", 4) == 0)
		{
			value = event_desc;
		}
	}

	/* store result */
	SetResult(&result, R_STRING, value);
}

/** status for conn or event connection
 *
 */
static void my_status(RESULT * result, RESULT *arg1)
{
	const char *value = "";
	const char *status = "Ok";
	char *key;


	key = R2S(arg1);
	if( strcmp(key, "conn") == 0 )
	{
		if( configureConn() > 0 )
		{
			value = status;
		}
	}
	else
	if( strcmp(key, "event") == 0 )
	{
		if( configureEvent() > 0 )
		{
			value = status;
		}
	}

	SetResult(&result, R_STRING, value);
}

#endif

/* plugin initialization */
int plugin_init_qnaplog(void)
{
#ifdef HAVE_SQLITE3_H
	/* register all our cool functions */
	/* the second parameter is the number of arguments */
	/* -1 stands for variable argument list */
	AddFunction("qnaplog::status", 1, my_status);
	AddFunction("qnaplog::conn", 1, my_conn);
	AddFunction("qnaplog::event", 1, my_event);
#endif
	return 0;
}

void plugin_exit_qnaplog(void)
{
#ifdef HAVE_SQLITE3_H
	/* free any allocated memory */
	/* close filedescriptors */
	sqlite3_close(connexConn);
	sqlite3_close(connexEvent);
#endif
}
