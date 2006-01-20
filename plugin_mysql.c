/* $Id: plugin_mysql.c,v 1.10 2006/01/20 15:58:05 reinelt Exp $
 *
 * plugin for execute SQL queries into a MySQL DBSM.
 *
 * Copyright (C) 2004 Javier Garcia <javi@gsmlandia.com>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: plugin_mysql.c,v $
 * Revision 1.10  2006/01/20 15:58:05  reinelt
 * MySQL::count() added again
 *
 * Revision 1.9  2006/01/20 15:43:25  reinelt
 * MySQL::query() returns a value, not the number of rows
 *
 * Revision 1.8  2006/01/16 15:39:58  reinelt
 * MySQL::queryvalue() extension from Harald Klemm
 *
 * Revision 1.7  2005/06/06 09:24:07  reinelt
 * two bugs in plugin_mysql.c fixed
 *
 * Revision 1.6  2005/05/08 04:32:44  reinelt
 * CodingStyle added and applied
 *
 * Revision 1.5  2005/04/01 05:16:04  reinelt
 * moved plugin init stuff to a seperate function called on first use
 *
 * Revision 1.4  2005/01/18 06:30:23  reinelt
 * added (C) to all copyright statements
 *
 * Revision 1.3  2004/03/21 22:05:53  reinelt
 * MySQL plugin fixes from Javi
 *
 * Revision 1.2  2004/03/20 23:09:01  reinelt
 * MySQL plugin fixes from Javi
 *
 * Revision 1.1  2004/03/10 07:16:15  reinelt
 * MySQL plugin from Javier added
 *
 */

/* DOC:
 * To compile the plugin remember to add -lmysqlclient option into Makefile. I.E: CC = gcc -lmysqlclient 
 * or run:
 * $ gcc -I/usr/include/mysql -L/usr/lib/mysql plugin_mysql.c -lmysqlclient -o plugin_mysql.o
 *
 * exported functions:
 *
 * int plugin_init_mysql (void)
 *
 *  adds various functions:
 *     MySQLquery(query) 
 *        Returns the number of rows in query.
 *     MySQLstatus()
 *        Returns the current server status:
 *        Uptime in seconds and the number of running threads,
 *        questions, reloads, and open tables.
 *
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "plugin.h"
#include "cfg.h"

#ifdef HAVE_MYSQL_MYSQL_H
#include <mysql/mysql.h>
#else
#warning mysql/mysql.h not found: plugin deactivated
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


#ifdef HAVE_MYSQL_MYSQL_H
static MYSQL conex;

static char Section[] = "Plugin:MySQL";


static int configure_mysql(void)
{
    static int configured = 0;

    char server[256];
    unsigned int port;
    char user[128];
    char password[256];
    char database[256];
    char *s;

    if (configured != 0)
	return configured;

    s = cfg_get(Section, "server", "localhost");
    if (*s == '\0') {
	info("[MySQL] empty '%s.server' entry from %s, assuming 'localhost'", Section, cfg_source());
	strcpy(server, "localhost");
    } else
	strcpy(server, s);
    free(s);

    if (cfg_number(Section, "port", 0, 1, 65536, &port) < 1) {
	/* using 0 as default port because mysql_real_connect() will convert it to real default one */
	info("[MySQL] no '%s.port' entry from %s using MySQL's default", Section, cfg_source());
    }

    s = cfg_get(Section, "user", "");
    if (*s == '\0') {
	/* If user is NULL or the empty string "", the lcd4linux Unix user is assumed. */
	info("[MySQL] empty '%s.user' entry from %s, assuming lcd4linux owner", Section, cfg_source());
	strcpy(user, "");
    } else
	strcpy(user, s);
    free(s);

    s = cfg_get(Section, "password", "");
    /* Do not encrypt the password because encryption is handled automatically by the MySQL client API. */
    if (*s == '\0') {
	info("[MySQL] empty '%s.password' entry in %s, assuming none", Section, cfg_source());
	strcpy(password, "");
    } else
	strcpy(password, s);
    free(s);

    s = cfg_get(Section, "database", "");
    if (*s == '\0') {
	error("[MySQL] no '%s:database' entry from %s, specify one", Section, cfg_source());
	free(s);
	configured = -1;
	return configured;
    }
    strcpy(database, s);
    free(s);

    mysql_init(&conex);
    if (!mysql_real_connect(&conex, server, user, password, database, port, NULL, 0)) {
	error("[MySQL] conection error: %s", mysql_error(&conex));
	configured = -1;
	return configured;
    }

    configured = 1;
    return configured;
}

static void my_MySQLcount(RESULT * result, RESULT * query)
{
    char *q;
    double value;
    MYSQL_RES *res;
    
    if (configure_mysql() < 0) {
	value = -1;
	SetResult(&result, R_NUMBER, &value);
	return;
    }

    q = R2S(query);

    /* mysql_ping(MYSQL *mysql) checks whether the connection to the server is working. */
    /* If it has gone down, an automatic reconnection is attempted. */
    mysql_ping(&conex);
    if (mysql_real_query(&conex, q, (unsigned int) strlen(q))) {
	error("[MySQL] query error: %s", mysql_error(&conex));
	value = -1;
    } else {
	/* We don't use res=mysql_use_result();  because mysql_num_rows() will not */
	/* return the correct value until all the rows in the result set have been retrieved */
	/* with mysql_fetch_row(), so we use res=mysql_store_result(); instead */
	res = mysql_store_result(&conex);
	value = (double) mysql_num_rows(res);
	mysql_free_result(res);
    }

    SetResult(&result, R_NUMBER, &value);
}


static void my_MySQLquery(RESULT * result, RESULT * query)
{
    char *q;
    double value;
    MYSQL_RES *res;
    MYSQL_ROW row=NULL;

    if (configure_mysql() < 0) {
	value = -1;
	SetResult(&result, R_NUMBER, &value);
	return;
    }

    q = R2S(query);

    /* mysql_ping(MYSQL *mysql) checks whether the connection to the server is working. */
    /* If it has gone down, an automatic reconnection is attempted. */
    mysql_ping(&conex);
    if (mysql_real_query(&conex, q, (unsigned int) strlen(q))) {
	error("[MySQL] query error: %s", mysql_error(&conex));
	value = -1;
    } else {
	/* We don't use res=mysql_use_result();  because mysql_num_rows() will not */
	/* return the correct value until all the rows in the result set have been retrieved */
	/* with mysql_fetch_row(), so we use res=mysql_store_result(); instead */
	res = mysql_store_result(&conex);
	row = mysql_fetch_row(res);
	mysql_free_result(res);
    }

    SetResult(&result, R_STRING, row[0]);
}


static void my_MySQLstatus(RESULT * result)
{
    const char *value = "";
    const char *status;

    if (configure_mysql() > 0) {

	mysql_ping(&conex);
	status = mysql_stat(&conex);
	if (!status) {
	    error("[MySQL] status error: %s", mysql_error(&conex));
	    value = "error";
	} else {
	    value = status;
	}
    }

    SetResult(&result, R_STRING, value);
}


#endif


int plugin_init_mysql(void)
{
#ifdef HAVE_MYSQL_MYSQL_H
    AddFunction("MySQL::count", 1, my_MySQLcount);
    AddFunction("MySQL::query", 1, my_MySQLquery);
    AddFunction("MySQL::status", 0, my_MySQLstatus);
#endif
    return 0;
}

void plugin_exit_mysql(void)
{
#ifdef HAVE_MYSQL_MYSQL_H
    mysql_close(&conex);
#endif
}
