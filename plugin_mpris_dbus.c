/* $Id: plugin_mpris_dbus.c 870 2009-04-09 14:55:23Z abbaskosan $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/plugin_mpris_dbus.c $
 *
 * plugin mpris dbus
 *
 * Copyright (C) 2009 Abbas Kosan <abbaskosan@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * int plugin_init_mpris_dbus (void)
 *  adds various functions
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <stdio.h>

// TODO: dbus-1 folder should be added to include path 
#include <dbus/dbus.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"
#include "hash.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static HASH DBUS;

// D-Bus variables
static DBusMessage *msg;
static DBusMessageIter args;
static DBusConnection *conn;
static DBusError err;
static DBusPendingCall *pending;

static void read_MetaData()
{
    // put pairs to hash table
    int current_type;
    // TODO: check size of char arrays
    char str_key[255];
    char str_value[2048];
    DBusMessageIter subiter1;

    dbus_message_iter_recurse(&args, &subiter1);
    while ((current_type = dbus_message_iter_get_arg_type(&subiter1)) != DBUS_TYPE_INVALID) {
	DBusMessageIter subiter2;
	DBusMessageIter subiter3;

	strcpy(str_key, "");
	strcpy(str_value, "");
	dbus_message_iter_recurse(&subiter1, &subiter2);
	current_type = dbus_message_iter_get_arg_type(&subiter2);

	if (current_type == DBUS_TYPE_INVALID)
	    break;
	if (current_type == DBUS_TYPE_STRING) {
	    void *str_tmp1;
	    void *str_tmp2;
	    dbus_message_iter_get_basic(&subiter2, &str_tmp1);
	    strcpy(str_key, str_tmp1);
	    dbus_message_iter_next(&subiter2);
	    dbus_message_iter_recurse(&subiter2, &subiter3);
	    current_type = dbus_message_iter_get_arg_type(&subiter3);
	    switch (current_type) {
	    case DBUS_TYPE_STRING:
		{
		    dbus_message_iter_get_basic(&subiter3, &str_tmp2);
		    strcpy(str_value, str_tmp2);
		    break;
		}
	    case DBUS_TYPE_INT32:
		{
		    dbus_int32_t val;
		    dbus_message_iter_get_basic(&subiter3, &val);
		    sprintf(str_value, "%d", val);
		    break;
		}
	    case DBUS_TYPE_INT64:
		{
		    dbus_int64_t val;
		    dbus_message_iter_get_basic(&subiter3, &val);
		    sprintf(str_value, "%jd", (intmax_t) val);
		    break;
		}
	    default:
		// unexpected type
		//printf (" (2-dbus-monitor too dumb to decipher arg type '%c')\n", current_type);
		break;
	    }
	    // add key-value pair to hash
	    hash_put(&DBUS, str_key, str_value);
	    //printf("Key: %s , Value: %s\t",str_key,str_value);
	}
	//else
	// unexpected type
	//printf (" (2-dbus-monitor too dumb to decipher arg type '%c')\n", current_type);                                                              
	dbus_message_iter_next(&subiter1);
    }
}

static int listen_TrackChange(void)
{
    // non blocking read of the next available message
    dbus_connection_read_write(conn, 0);
    msg = dbus_connection_pop_message(conn);

    // nothing to do if we haven't read a message
    if (NULL == msg)
	return 0;

    // check if the message is a signal from the correct interface and with the correct name
    if (dbus_message_is_signal(msg, "org.freedesktop.MediaPlayer", "TrackChange")) {
	// read the parameters
	// TrackChange signal returns an array of string-variant pairs
	if (!dbus_message_iter_init(msg, &args)) {
	    // No parameter !
	    //fprintf(stderr, "Message Has No Parameters\n");
	    return 0;
	} else if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args)) {
	    // Argument is not array
	    //fprintf(stderr, "Argument is not array!\n");
	    return 0;
	} else
	    read_MetaData();
	//printf("Got Signal\n");
    }
    // free message
    dbus_message_unref(msg);
    return 1;
}

static unsigned long int call_PositionGet(char *target)
{
    unsigned long int value = 0;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(target,	//target for the method call e.g "org.kde.amarok"
				       "/Player",	// object to call on
				       "org.freedesktop.MediaPlayer",	// interface to call on
				       "PositionGet");	// method name
    if (NULL == msg) {
	//fprintf(stderr, "Message Null\n");
	return 0;
    }
    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {	// -1 is default timeout
	//fprintf(stderr, "Out Of Memory!\n");
	return 0;
    }
    if (NULL == pending) {
	//fprintf(stderr, "Pending Call Null\n");
	return 0;
    }
    dbus_connection_flush(conn);

    //printf("Request Sent\n");

    // free message
    dbus_message_unref(msg);

    // block until we recieve a reply
    dbus_pending_call_block(pending);

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
	//fprintf(stderr, "Reply Null\n");
	return 0;
    }
    // free the pending message handle
    dbus_pending_call_unref(pending);

    // read the parameters
    if (!dbus_message_iter_init(msg, &args)) {
	//fprintf(stderr, "Message has no arguments!\n");
	return 0;
    }
    if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) {
	//fprintf(stderr, "Argument is not INT32!\n");
	return 0;
    } else
	dbus_message_iter_get_basic(&args, &value);
    //printf("\nGot Reply: %d", value);

    // free message
    dbus_message_unref(msg);

    return value;
}

static void signal_TrackChange(RESULT * result, RESULT * arg1)
{
    char *str_tmp;
    /*
       if (listen_TrackChange() < 0) {
       SetResult(&result, R_STRING, "");
       return;
       }
     */
    listen_TrackChange();
    str_tmp = hash_get(&DBUS, R2S(arg1), NULL);
    if (str_tmp == NULL)
	str_tmp = "";

    SetResult(&result, R_STRING, str_tmp);
}

static void method_PositionGet(RESULT * result, RESULT * arg1)
{
    unsigned long int mtime = 0;
    unsigned long int value = 0;
    double ratio = 0;
    char *str_tmp;

    value = call_PositionGet(R2S(arg1));
    //printf("\ncalled :call_PositionGet %d",value);


    str_tmp = hash_get(&DBUS, "mtime", NULL);
    if (str_tmp != NULL)
	mtime = atoi(str_tmp);
    if (mtime > 0)
	ratio = (double) (((float) value / mtime) * 100);

    //printf("\nvalue:%d mtime:%d ratio:%f",value,mtime,ratio);
    // return actual position as percentage of total length 
    SetResult(&result, R_NUMBER, &ratio);
}

/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_mpris_dbus(void)
{
    hash_create(&DBUS);

    int ret;

    //printf("Listening for signals\n");

    // initialise the errors
    dbus_error_init(&err);

    // connect to the bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    /*
       if (dbus_error_is_set(&err)) {
       fprintf(stderr, "Connection Error (%s)\n", err.message);
       dbus_error_free(&err);
       }
     */
    if (NULL == conn)
	return 0;

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(conn, "org.lcd4linux.mpris_dbus", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    /*
       if (dbus_error_is_set(&err)) {
       fprintf(stderr, "Name Error (%s)\n", err.message);
       dbus_error_free(&err);
       }
     */
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
	return 0;

    // add a rule for which messages we want to see
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.MediaPlayer'", &err);	// see signals from the given interface
    dbus_connection_flush(conn);
    /*
       if (dbus_error_is_set(&err)) {
       fprintf(stderr, "Match Error (%s)\n", err.message);
       return 0;
       }
     */
    //printf("Match rule sent\n");


    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("mpris_dbus::signal_TrackChange", 1, signal_TrackChange);
    AddFunction("mpris_dbus::method_PositionGet", 1, method_PositionGet);

    return 0;
}

void plugin_exit_mpris_dbus(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
    hash_destroy(&DBUS);

    if (NULL != msg)
	dbus_message_unref(msg);
    dbus_error_free(&err);
}
