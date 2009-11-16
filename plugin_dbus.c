/* $Id: plugin_dbus.c -1   $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/plugin_dbus.c $
 *
 * plugin template
 *
 * Copyright (C) 2009 Edward Martin <edman007@edman007.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * int plugin_init_dbus (void)
 *  adds various functions
 *
 */


/* define the include files you need */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"
#include "cfg.h"
#include "timer.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "event.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <assert.h>
/*
 * Internal Text Storage (to catch the events)
 */

//a buffer of the most recent results (store the strings you see on the screen)
typedef struct {
    int argc;
    char **arguments;
} dbus_signal_t;

static struct {
    int signals;
    dbus_signal_t *a;
} dbus_results = {
0, NULL};

//free all resources for the text of a signal
static int clear_signal_txt(const int sig);

//return a pointer to the struct of text for the signal
static dbus_signal_t *get_signal_txt(const int sig);

//set the struct of text for the signal
static int set_signal_txt(const int sig, const dbus_signal_t * data);


/*
 * DBus Functions/types
 */


//the max that a message may be in characters (if you have a screen that fits more go crazy)
static const int LCD_MAX_MSG_LEN = 255;
#define DBUS_MAX_SIGNALS 64
static const char *DBUS_PLUGIN_SECTION = "Plugin:DBus";



/*
 * This connects to dbus and gets relavant things for the LCD
 */

typedef struct lcd_sig_t lcd_sig_t;
//do not look inside, its private!
struct lcd_sig_t {
    void *user_data;
    void (*callback) (void *user_data, int argc, char **argv);
    void (*user_free) (void *);
    char *sender;
    char *path;
    char *interface;
    char *member;
    char *rule;
};


typedef struct {
    int id;
    char *event_name;
} handle_signal_t;

static DBusConnection *sessconn;
static DBusConnection *sysconn;
static DBusError err;

static lcd_sig_t *lcd_registered_signals[DBUS_MAX_SIGNALS];	//used mostly for freeing resources
static int registered_sig_count = 0;

//takes the signal, matches it against a rule, and sends the correct agrument, as a string, to the screen
static DBusHandlerResult lcd_sig_received(DBusConnection * connection, DBusMessage * message, void *sigv);

//frees all resources for a signal (use lcd_unregister_signal instead)
static void free_signal(lcd_sig_t * sig);
//unregisters a signal and frees its resources
static void lcd_unregister_signal(lcd_sig_t * sig);

//these copy data in/out of dbus
static void fill_args(DBusMessage * message, int *argcount, char ***argv);
static void free_args(int argc, char **argv);

//returns 0 on error (no connection opened)
//        1 when only the system connection was opened
//        2 when only the session connection was opened
//        3 both connections were opened
static int lcd_dbus_init(void);

static void handle_inbound_signal(void *signal, int argc, char **argv);

//given a signal, will add a hook so when the signal appears your callback is called
static lcd_sig_t *lcd_register_signal(const char *sender, const char *path,
				      const char *interface, const char *member,
				      void (*callback) (void *user_data, int argc, char **argv), void *user_data,
				      void (*user_free) (void *));

static void setup_dbus_events(DBusConnection * conn);

//to handle watches through the main loop
static dbus_bool_t add_watch(DBusWatch * w, void *data);
static void remove_watch(DBusWatch * w, void *data);
static void toggle_watch(DBusWatch * w, void *data);
static void watch_handle(event_flags_t f, void *data);

static void dispatch_dbus(void);	//tell dbus to read something


//to handle timers through the main loop
static void timeout_dbus_handle(void *data);
static dbus_bool_t add_dbus_timeout(DBusTimeout * t, void *data);
static void remove_dbus_timeout(DBusTimeout * t, void *data);
static void toggle_dbus_timeout(DBusTimeout * t, void *data);

static lcd_sig_t *create_signal(const char *sender, const char *path,
				const char *interface, const char *member,
				void (*callback) (void *user_data, int argc, char **argv), void *user_data,
				void (*user_free) (void *));

static int clear_signal_txt(const int sig)
{
    dbus_signal_t *s = get_signal_txt(sig);
    if (s == NULL) {
	return 1;
    }
    s->argc = 0;
    if (s->arguments != NULL) {
	free(s->arguments);
	s->arguments = NULL;
    }
    return 0;
}

static dbus_signal_t *get_signal_txt(const int sig)
{
    if (sig < 0 || sig >= dbus_results.signals) {
	return NULL;
    }
    return &dbus_results.a[sig];
}

//sets a signal struct for the given data
static int set_signal_txt(const int sig, const dbus_signal_t * data)
{
    if (sig < 0 || sig >= dbus_results.signals) {
	int new_sigs = sig + 1;
	//allocate it
	dbus_results.a = realloc(dbus_results.a, sizeof(dbus_signal_t *) * new_sigs);
	int i;
	//clear anything just allocated that we are not writing to right now
	for (i = dbus_results.signals + 1; i < new_sigs; i++) {
	    dbus_results.a[i].argc = 0;
	    dbus_results.a[i].arguments = NULL;
	}
	dbus_results.signals = new_sigs;
    }
    //free the old version
    if (dbus_results.a[sig].argc != 0) {
	clear_signal_txt(sig);
    }
    dbus_results.a[sig].argc = data->argc;
    //need to dup the strings
    dbus_results.a[sig].arguments = malloc(sizeof(data->arguments) * data->argc);
    int i;
    for (i = 0; i < data->argc; i++) {
	dbus_results.a[sig].arguments[i] = strdup(data->arguments[i]);
    }
    return 0;
}



/*
 * lcd4linux interface fucntions
 */


static void get_argument(RESULT * result, RESULT * sig, RESULT * arg)
{
    int signal = (int) R2N(sig);
    int argument = (int) R2N(arg);

    dbus_signal_t *signal_value = get_signal_txt(signal);
    char *value = "";
    if (signal_value != NULL && signal_value->argc > argument && signal_value->arguments[argument] != NULL) {
	value = signal_value->arguments[argument];

    }

    /* store result */
    SetResult(&result, R_STRING, value);

}

static void clear_arguments(RESULT * result, RESULT * sig)
{
    int signal = (int) R2N(sig);
    dbus_signal_t *signal_value = get_signal_txt(signal);
    int i;
    if (signal_value->arguments != NULL) {
	for (i = 0; i < signal_value->argc; i++) {
	    if (signal_value->arguments[i] != NULL) {
		free(signal_value->arguments[i]);
	    }
	}
	free(signal_value->arguments);
	signal_value->arguments = NULL;
    }
    signal_value->argc = 0;

    /* store result */
    SetResult(&result, R_STRING, "");
}

static void load_dbus_cfg(void)
{
    char *sender, *path, *interface, *member, *eventname;
    int i;
    const char *sender_fmt = "signal%dsender";
    const char *path_fmt = "signal%dpath";
    const char *interface_fmt = "signal%dinterface";
    const char *member_fmt = "signal%dmember";
    const char *eventname_fmt = "signal%deventname";
    const int max_cfg_len = 32;
    char cfg_name[max_cfg_len];
    for (i = 0; i < DBUS_MAX_SIGNALS; i++) {
	snprintf(cfg_name, max_cfg_len - 1, sender_fmt, i);
	sender = cfg_get(DBUS_PLUGIN_SECTION, cfg_name, "");

	snprintf(cfg_name, max_cfg_len - 1, path_fmt, i);
	path = cfg_get(DBUS_PLUGIN_SECTION, cfg_name, "");

	snprintf(cfg_name, max_cfg_len - 1, interface_fmt, i);
	interface = cfg_get(DBUS_PLUGIN_SECTION, cfg_name, "");

	snprintf(cfg_name, max_cfg_len - 1, member_fmt, i);
	member = cfg_get(DBUS_PLUGIN_SECTION, cfg_name, "");

	snprintf(cfg_name, max_cfg_len - 1, eventname_fmt, i);
	eventname = cfg_get(DBUS_PLUGIN_SECTION, cfg_name, "");

	if (*path == '\0' && *interface == '\0' && *member == '\0') {
	    continue;
	} else if (*path == '\0' || *interface == '\0' || *member == '\0') {
	    error("[DBus] Incomplete configuration specified for signal%d", i);
	    continue;
	}

	handle_signal_t *sig_info = malloc(sizeof(handle_signal_t));
	sig_info->id = i;
	if (*eventname == '\0') {
	    sig_info->event_name = NULL;
	} else {
	    sig_info->event_name = strdup(eventname);
	}

	if (!lcd_register_signal(sender, path, interface, member, handle_inbound_signal, sig_info, free)) {
	    error("[DBus] Error Registering signal %d", i);
	}

    }
}

/* plugin initialization */
int plugin_init_dbus(void)
{
    if (!lcd_dbus_init()) {
	error("[DBus] Could not connect to DBus");
	return 1;
    }
    //dbus::argument(<DisplaySignal>, <Argument#>)//displays arg# for signal#
    AddFunction("dbus::argument", 2, get_argument);
    //dbus::clear(<signal>)//sets the arguments for signal#
    AddFunction("dbus::clear", 1, clear_arguments);

    //read out config
    load_dbus_cfg();
    return 0;
}

void plugin_exit_dbus(void)
{
    int i;
    //remove all known signals
    for (i = registered_sig_count - 1; i >= 0; i--) {
	lcd_unregister_signal(lcd_registered_signals[i]);
    }

    //remove all knows signal results
    for (i = dbus_results.signals - 1; i >= 0; i--) {
	clear_signal_txt(i);
    }
}


/*
 * From here one out it is all dbus specific funtions,
 * should probably be split into a seperate file, but i
 * keept it here to make the plugin a single file
 *
 */

//watch functions
static dbus_bool_t add_watch(DBusWatch * w, void *data)
{
    (void) data;		//ignore it
#if (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR == 1 && DBUS_VERSION_MICRO >= 1) || (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR > 1) || (DBUS_VERSION_MAJOR > 1) 
    int fd = dbus_watch_get_unix_fd(w); 
#else 
    int fd = dbus_watch_get_fd(w); 
#endif 
    // int fd = dbus_watch_get_unix_fd(w);	//we assume we are using unix
    int flags = dbus_watch_get_flags(w);
    event_add(watch_handle, w, fd, flags & DBUS_WATCH_READABLE, flags & DBUS_WATCH_WRITABLE, dbus_watch_get_enabled(w));
    return TRUE;
}

static void remove_watch(DBusWatch * w, void *data)
{
    (void) data;		//ignore it
#if (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR == 1 && DBUS_VERSION_MICRO >= 1) || (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR > 1) || (DBUS_VERSION_MAJOR > 1) 
    event_del(dbus_watch_get_unix_fd(w));
#else 
    event_del(dbus_watch_get_fd(w));
#endif 
    // event_del(dbus_watch_get_unix_fd(w));
}

static void toggle_watch(DBusWatch * w, void *data)
{
#if (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR == 1 && DBUS_VERSION_MICRO >= 1) || (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR > 1) || (DBUS_VERSION_MAJOR > 1) 
    int fd = dbus_watch_get_unix_fd(w); 
#else 
    int fd = dbus_watch_get_fd(w); 
#endif 
    // int fd = dbus_watch_get_unix_fd(w);	//we assume we are using unix
    int flags = dbus_watch_get_flags(w);
    event_modify(fd, flags & DBUS_WATCH_READABLE, flags & DBUS_WATCH_WRITABLE, dbus_watch_get_enabled(w));
}

static void watch_handle(event_flags_t f, void *data)
{
    DBusWatch *w = (DBusWatch *) data;

    //convert the flags
    unsigned int flags = 0;

    flags |= (f & EVENT_READ) ? DBUS_WATCH_READABLE : 0;
    flags |= (f & EVENT_WRITE) ? DBUS_WATCH_WRITABLE : 0;
    flags |= (f & EVENT_HUP) ? DBUS_WATCH_HANGUP : 0;
    flags |= (f & EVENT_ERR) ? DBUS_WATCH_ERROR : 0;

    //tell dbus
    if (!dbus_watch_handle(w, flags)) {
	info("[DBus] dbus_watch_handle(): Not enough memory!");
    }
    dispatch_dbus();
}

static void dispatch_dbus(void)
{
    if (sessconn != NULL && dbus_connection_get_dispatch_status(sessconn) == DBUS_DISPATCH_DATA_REMAINS) {
	while (DBUS_DISPATCH_DATA_REMAINS == dbus_connection_dispatch(sessconn));
    }
    if (sysconn != NULL && dbus_connection_get_dispatch_status(sysconn) == DBUS_DISPATCH_DATA_REMAINS) {
	while (DBUS_DISPATCH_DATA_REMAINS == dbus_connection_dispatch(sysconn));
    }
}

static void timeout_dbus_handle(void *data)
{
    DBusTimeout *t = (DBusTimeout *) data;
    if (!dbus_timeout_handle(t)) {
	info("[DBus] Not enough memory to handle timeout!");
    }
    dispatch_dbus();
}

static dbus_bool_t add_dbus_timeout(DBusTimeout * t, void *data)
{
    (void) data;		//ignore warning
    if (!timer_add_late(timeout_dbus_handle, t, dbus_timeout_get_interval(t), 0)) {
	return FALSE;
    }
    return TRUE;
}


static void remove_dbus_timeout(DBusTimeout * t, void *data)
{
    (void) data;		//ignore warning
    timer_remove(timeout_dbus_handle, t);
}

static void toggle_dbus_timeout(DBusTimeout * t, void *data)
{
    remove_dbus_timeout(t, data);
    add_dbus_timeout(t, data);
}

//add the proper events/callbacks so we get notified about out messages
static void setup_dbus_events(DBusConnection * conn)
{
    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, NULL, NULL)) {
	error("[DBus] dbus_connection_set_watch_functions(): Not enough memory!");
    }
    if (!dbus_connection_set_timeout_functions
	(conn, add_dbus_timeout, remove_dbus_timeout, toggle_dbus_timeout, NULL, NULL)) {
	error("[DBus] dbus_connection_set_timeout_functions(): Not enough memory!");
    }
}

static int lcd_dbus_init(void)
{
    int success = 3;
    dbus_error_init(&err);	//dbus_error_free(&err);
    sessconn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (sessconn == NULL) {
	info("[DBus] Error connecting to the dbus session bus: %s\n", err.message);
	dbus_error_free(&err);
	success &= 1;
    } else {
	setup_dbus_events(sessconn);
    }

    sysconn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (sysconn == NULL) {
	info("[DBus] Error connecting to the dbus system bus: %s\n", err.message);
	success &= 2;
    } else {
	setup_dbus_events(sysconn);
    }

    return success;
}

static lcd_sig_t *create_signal(const char *sender, const char *path,
				const char *interface, const char *member,
				void (*callback) (void *user_data, int argc, char **argv), void *user_data,
				void (*user_free) (void *))
{
    lcd_sig_t *sig = malloc(sizeof(lcd_sig_t));
    sig->callback = callback;
    sig->user_data = user_data;
    sig->user_free = user_free;
    sig->sender = malloc(sizeof(char) * (1 + strlen(sender)));
    strcpy(sig->sender, sender);
    sig->path = malloc(sizeof(char) * (1 + strlen(path)));
    strcpy(sig->path, path);
    sig->interface = malloc(sizeof(char) * (1 + strlen(interface)));
    strcpy(sig->interface, interface);
    sig->member = malloc(sizeof(char) * (1 + strlen(member)));
    strcpy(sig->member, member);

    size_t len = strlen(sender) + strlen(path) + strlen(interface) + strlen(member);
    char *format;
    if (strlen(sig->sender) == 0) {
	format = "type='signal',path='%s',interface='%s',member='%s'";
    } else {
	format = "type='signal',sender='%s',path='%s',interface='%s',member='%s'";
    }
    len += strlen(format);
    len *= sizeof(char);
    sig->rule = malloc(len);
    if (strlen(sig->sender) == 0) {
	sprintf(sig->rule, format, path, interface, member);
    } else {
	sprintf(sig->rule, format, sender, path, interface, member);
    }
    assert(strlen(sig->rule) < len);
    return sig;
}

static void handle_inbound_signal(void *signal, int argc, char **argv)
{
    handle_signal_t *signal_info = (handle_signal_t *) signal;
    dbus_signal_t sig;
    sig.argc = argc;
    sig.arguments = argv;
    set_signal_txt(signal_info->id, &sig);
    debug("[DBUS] Got Signal");
    if (signal_info->event_name != NULL) {
	debug("[DBUS] Calling Screen update");
	named_event_trigger(signal_info->event_name);
    }
    free_args(argc, argv);
}

static lcd_sig_t *lcd_register_signal(const char *sender, const char *path,
				      const char *interface, const char *member,
				      void (*callback) (void *user_data, int argc, char **argv), void *user_data,
				      void (*user_free) (void *))
{
    if (__builtin_expect(registered_sig_count >= DBUS_MAX_SIGNALS, 0)) {	//gcc >= 2.96
	//i don't think anything will allow this to ever be hit..in fact It's impossible in the form i wrote the plugin
	error
	    ("[DBus] Attempted to add more than %d dus signals, if you actually need more than that edit DBUS_MAX_SIGNALS in plugin_dbus.c",
	     DBUS_MAX_SIGNALS);
	return NULL;
    }
    //store everything we need in the singal struct
    lcd_sig_t *sig = create_signal(sender, path, interface, member, callback, user_data, user_free);
    int success = 3;
    if (sessconn != NULL) {
	dbus_bus_add_match(sessconn, sig->rule, &err);
	if (dbus_error_is_set(&err)) {
	    info("[DBus] Error adding dbus match to the session bus: %s \n", err.message);
	    dbus_error_free(&err);
	    success ^= 1;
	}


	if (!dbus_connection_add_filter(sessconn, lcd_sig_received, sig, NULL) && (success & 1)) {
	    info("[DBus] Dbus signal registration failed to the session bus!\n");
	    dbus_bus_remove_match(sessconn, sig->rule, &err);
	    success ^= 1;
	}
    } else {
	success ^= 1;
    }
    if (sysconn != NULL) {
	dbus_bus_add_match(sysconn, sig->rule, &err);
	if (dbus_error_is_set(&err)) {
	    info("[DBus] Error adding dbus match to the system bus: %s \n", err.message);
	    success ^= 2;
	}

	if (!dbus_connection_add_filter(sysconn, lcd_sig_received, sig, NULL) && (success & 2)) {
	    info("[DBus] Dbus signal registration failed to the system bus!\n");
	    dbus_bus_remove_match(sysconn, sig->rule, &err);
	    success ^= 2;
	}
    } else {
	success ^= 2;
    }
    if (!success) {
	free_signal(sig);
	return NULL;
    }

    lcd_registered_signals[registered_sig_count] = sig;
    registered_sig_count++;
    return sig;
}

static void free_signal(lcd_sig_t * sig)
{
    if (sig->user_free != NULL) {
	sig->user_free(sig->user_data);
    }
    free(sig->sender);
    free(sig->path);
    free(sig->interface);
    free(sig->member);
    free(sig->rule);
    free(sig);
}

static void lcd_unregister_signal(lcd_sig_t * sig)
{
    if (sig == NULL) {
	return;
    }
    info("[DBus] Unregistering signal %p!", sig);
    //remove filter and match
    if (sessconn != NULL) {
	dbus_connection_remove_filter(sessconn, lcd_sig_received, sig);
	dbus_bus_remove_match(sessconn, sig->rule, NULL);
    }
    if (sysconn != NULL) {
	dbus_connection_remove_filter(sysconn, lcd_sig_received, sig);
	dbus_bus_remove_match(sysconn, sig->rule, NULL);
    }
    //free user data
    free_signal(sig);

    //drop it off the list
    int i;
    for (i = 0; i < registered_sig_count; i++) {
	if (lcd_registered_signals[i] == sig) {
	    registered_sig_count--;
	    lcd_registered_signals[i] = lcd_registered_signals[registered_sig_count];
	    break;
	}
    }
}


static void fill_args(DBusMessage * message, int *argcount, char ***argv)
{
    dbus_message_ref(message);
    int argc = 0;
    char **args = NULL;


    DBusMessageIter iter;
    int buf_size = 0;
    dbus_message_iter_init(message, &iter);
    int current_type;

    //iterate over all arguments, casting all primitaves to strings
    while ((current_type = dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
	assert(argc <= buf_size);
	if (argc >= buf_size) {
	    buf_size = argc * 2;
	    if (buf_size == 0) {
		buf_size = 1;
	    }
	    args = realloc(args, sizeof(char **) * buf_size);
	}
	if (dbus_type_is_basic(current_type)) {
	    args[argc] = malloc(sizeof(char) * (1 + LCD_MAX_MSG_LEN));
	    args[argc][LCD_MAX_MSG_LEN] = '\0';
	    //determine the type
	    switch (current_type) {
		/* Primitive types */
	    case DBUS_TYPE_BYTE:
		{
		    char value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%hhx", value);
		}

		break;
	    case DBUS_TYPE_BOOLEAN:
		{
		    dbus_bool_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    if (value) {
			strcpy(args[argc], "true");
		    } else {
			strcpy(args[argc], "false");
		    }

		}
		break;
	    case DBUS_TYPE_INT16:
		{
		    dbus_int16_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%hd", value);
		}
		break;
	    case DBUS_TYPE_UINT16:
		{
		    dbus_uint16_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%hu", value);
		}
		break;
	    case DBUS_TYPE_INT32:
		{
		    dbus_int32_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%d", value);
		}
		break;
	    case DBUS_TYPE_UINT32:
		{
		    dbus_uint32_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%u", value);
		}
		break;
	    case DBUS_TYPE_INT64:
		{
		    dbus_int64_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%ld", value);
		}
		break;
	    case DBUS_TYPE_UINT64:
		{
		    dbus_uint64_t value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%lu", value);
		}
		break;
	    case DBUS_TYPE_DOUBLE:
		{
		    double value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%f", value);
		}
		break;
	    case DBUS_TYPE_STRING:	//all strings
	    case DBUS_TYPE_OBJECT_PATH:
	    case DBUS_TYPE_SIGNATURE:
		{
		    char *value;
		    dbus_message_iter_get_basic(&iter, &value);
		    snprintf(args[argc], LCD_MAX_MSG_LEN, "%s", value);
		}
		break;
	    case DBUS_TYPE_INVALID:
	    default:
		//not supported
		free(args[argc]);
		args[argc] = NULL;
		assert(0);	//should never ever happen...

		break;
	    }
	} else {
	    args[argc] = NULL;
	}

	argc++;
	dbus_message_iter_next(&iter);
    }


    *argcount = argc;
    *argv = args;
    dbus_message_unref(message);
}


static DBusHandlerResult lcd_sig_received(DBusConnection * connection, DBusMessage * msg, void *sigv)
{
    dbus_message_ref(msg);

    lcd_sig_t *sig = sigv;
    //compare the signal to the one we were assigned
    if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_SIGNAL) {
	dbus_message_unref(msg);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    //we don't check the sender because we (probably) asked by name, not by sender
    if (!dbus_message_has_member(msg, sig->member) ||
	!dbus_message_has_path(msg, sig->path) || !dbus_message_has_interface(msg, sig->interface)) {
	dbus_message_unref(msg);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    char **args;
    int argc;
    fill_args(msg, &argc, &args);

    //call the users function
    if (sig->callback != NULL) {
	sig->callback(sig->user_data, argc, args);
    }
    dbus_message_unref(msg);
    return DBUS_HANDLER_RESULT_HANDLED;
}



static void free_args(int argc, char **argv)
{
    int i;
    //free args
    for (i = 0; i < argc; i++) {
	if (argv[i] != NULL) {
	    free(argv[i]);
	}
    }

    if (argv != NULL && argc != 0) {
	free(argv);
    }

}
