/* $Id: plugin_mysql.c,v 1.1 2004/03/10 07:16:15 reinelt Exp $
 *
 * plugin for execute SQL queries into a MySQL DBSM.
 *
 * Copyright 2004 Javier Garcia <javi@gsmlandia.com>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 *     mySQLquery(server,login,pass,database,query) 
 *        Returns the number of rows in a query.
 *     mySQLstatus(server,login,pass)
 *        Returns the current server status:
 *        Uptime, Threads, Questions,Slow queries, Flush tables,...
 *
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "plugin.h"

#ifdef HAVE_MYSQL_MYSQL_H
#include <mysql/mysql.h>
#else
#warning mysql/mysql.h not found: plugin deactivated
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


#ifdef HAVE_MYSQL_MYSQL_H
static void my_mySQLquery (RESULT *result, RESULT *server, RESULT *login, RESULT *pass, RESULT *database, RESULT *query)
{
	double value;
	MYSQL conex;
	MYSQL_RES *res;
	char *s=R2S(server);
	char *l=R2S(login);
	char *p=R2S(pass);
	char *db=R2S(database);
	char *q=R2S(query);
	
	mysql_init(&conex);
	if (!mysql_real_connect(&conex,s,l,p,db,0,NULL,0))
	{
		error( "mySQL conection error: %s",mysql_error(&conex));
		value=-1;
	}
	else
	{
		if (mysql_real_query(&conex,q,(unsigned int) strlen(q)))
		{
			error( "mySQL query error: %s",mysql_error(&conex));
			value=-2;
		}
		else
		{
			/* We don't use res=mysql_use_result();  because mysql_num_rows() will not
			 return the correct value until all the rows in the result set have been retrieved
			  with mysql_fetch_row(), so we use res=mysql_store_result(); instead */
			res=mysql_store_result(&conex);	 
			value = (double) mysql_num_rows(res);
			mysql_free_result(res);
		}
	}
	mysql_close(&conex);
	SetResult(&result, R_NUMBER, &value); 
}

static void my_mySQLstatus (RESULT *result, RESULT *server, RESULT *login, RESULT *pass)
{
	char *value;
	MYSQL conex;
	char *s=R2S(server);
	char *l=R2S(login);
	char *p=R2S(pass);
	char *status;
	
	mysql_init(&conex);
	if (!mysql_real_connect(&conex,s,l,p,NULL,0,NULL,0))
	{
		error( "mySQL conection error: %s",mysql_error(&conex));
		value="error";
	}
	else
	{
		status=strdup(mysql_stat(&conex));
		if (!status)
		{
			error( "mySQLstatus error: %s",mysql_error(&conex));
			value="error";
		}
		else
		{
			value = status;
		}
	}
	mysql_close(&conex);
	SetResult(&result, R_STRING, value); 
}

#endif

int plugin_init_mysql (void)
{
#ifdef HAVE_MYSQL_MYSQL_H
  AddFunction ("mySQLquery",    5, my_mySQLquery);
  AddFunction ("mySQLstatus",   3, my_mySQLstatus);
#endif
  return 0;
}

void plugin_exit_mysql(void) 
{
}
