/* $Id$
 * $URL$
 *
 * imond/telmond data processing
 *
 * Copyright (C) 2003 Nico Wallmeier <nico.wallmeier@post.rwth-aachen.de>
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

#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "qprintf.h"
#include "cfg.h"
#include "hash.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>		/* decl of inet_addr()      */
#include <sys/socket.h>


static HASH TELMON;
static HASH IMON;

static char thost[256];
static int tport;
static char phoneb[256];

static char ihost[256];
static char ipass[256];
static int iport;

static int fd = 0;
static int err = 0;

/*----------------------------------------------------------------------------
 *  service_connect (host_name, port)       - connect to tcp-service
 *----------------------------------------------------------------------------
 */
static int service_connect(const char *host_name, const int port)
{
    struct sockaddr_in addr;
    struct hostent *host_p;
    int fd;
    int opt = 1;

    (void) memset((char *) &addr, 0, sizeof(addr));

    if ((addr.sin_addr.s_addr = inet_addr((char *) host_name)) == INADDR_NONE) {
	host_p = gethostbyname(host_name);
	if (!host_p) {
	    error("%s: host not found\n", host_name);
	    return (-1);
	}
	(void) memcpy((char *) (&addr.sin_addr), host_p->h_addr, host_p->h_length);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short) port);

    /* open socket */
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket");
	return (-1);
    }

    (void) setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, sizeof(opt));

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
	(void) close(fd);
	perror(host_name);
	return (-1);
    }

    return (fd);
}				/* service_connect (char * host_name, int port) */


/*----------------------------------------------------------------------------
 *  send_command (int fd, char * str)       - send command to imond
 *----------------------------------------------------------------------------
 */
static void send_command(const int fd, const char *str)
{
    char buf[256];
    int len = strlen(str);

    sprintf(buf, "%s\r\n", str);
    write(fd, buf, len + 2);

    return;
}				/* send_command (int fd, char * str) */


/*----------------------------------------------------------------------------
 *  get_answer (int fd)                     - get answer from imond
 *----------------------------------------------------------------------------
 */
static char *get_answer(const int fd)
{
    static char buf[8192];
    int len;

    len = read(fd, buf, 8192);

    if (len <= 0) {
	return ((char *) NULL);
    }

    while (len > 1 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
	buf[len - 1] = '\0';
	len--;
    }

    if (!strncmp(buf, "OK ", 3)) {	/* OK xxxx */
	return (buf + 3);
    } else if (len > 2 && !strcmp(buf + len - 2, "OK")) {
	*(buf + len - 2) = '\0';
	return (buf);
    } else if (len == 2 && !strcmp(buf + len - 2, "OK")) {
	return (buf);
    }

    return ((char *) NULL);	/* ERR xxxx */
}				/* get_answer (int fd) */

/*----------------------------------------------------------------------------
 *  get_value (char * cmd)         - send command, get value
 *----------------------------------------------------------------------------
 */
static char *get_value(const char *cmd)
{
    char *answer;

    send_command(fd, cmd);

    answer = get_answer(fd);

    if (answer) {
	return (answer);
    }

    return ("");
}				/* get_value (char * cmd, int arg) */


static void phonebook(char *number)
{
    FILE *fp;
    char line[256];

    fp = fopen(phoneb, "r");

    if (!fp)
	return;

    while (fgets(line, sizeof(line), fp)) {
	if (*line == '#')
	    continue;
	if (!strncmp(line, number, strlen(number))) {
	    char *komma = strchr(line, ',');
	    char *beginn = strchr(line, '=');
	    if (!beginn)
		return;
	    while (strrchr(line, '\r'))
		strrchr(line, '\r')[0] = '\0';
	    while (strrchr(line, '\n'))
		strrchr(line, '\n')[0] = '\0';
	    if (komma)
		komma[0] = '\0';
	    strcpy(number, beginn + 1);
	    break;
	}
    }

    fclose(fp);
}


static int parse_telmon()
{
    static int telmond_fd = -2;
    static char oldanswer[128];
    int age;

    /* reread every 1 sec only */
    age = hash_age(&TELMON, NULL);
    if (age > 0 && age <= 1000)
	return 0;

    if (telmond_fd != -1) {
	char telbuf[128];

	telmond_fd = service_connect(thost, tport);
	if (telmond_fd >= 0) {
	    int l = read(telmond_fd, telbuf, 127);
	    if ((l > 0) && (strcmp(telbuf, oldanswer))) {
		char date[11];
		char time[11];
		char number[256];
		char msn[256];
		sscanf(telbuf, "%s %s %s %s", date, time, number, msn);
		hash_put(&TELMON, "time", time);
		date[4] = '\0';
		date[7] = '\0';
		qprintf(time, sizeof(time), "%s.%s.%s", date + 8, date + 5, date);
		hash_put(&TELMON, "number", number);
		hash_put(&TELMON, "msn", msn);
		hash_put(&TELMON, "date", time);
		phonebook(number);
		phonebook(msn);
		hash_put(&TELMON, "name", number);
		hash_put(&TELMON, "msnname", msn);
	    }
	    close(telmond_fd);
	    strcpy(oldanswer, telbuf);
	}
    }
    return 0;
}


static int configure_telmon(void)
{
    static int configured = 0;

    char *s;

    if (configured != 0)
	return configured;

    hash_create(&TELMON);

    s = cfg_get("Plugin:Telmon", "Host", "127.0.0.1");
    if (*s == '\0') {
	error("[Telmon] empty 'Host' entry in %s", cfg_source());
	configured = -1;
	return configured;
    }
    strcpy(thost, s);
    free(s);

    if (cfg_number("Plugin:Telmon", "Port", 5001, 1, 65536, &tport) < 0) {
	error("[Telmon] no valid port definition");
	configured = -1;
	return configured;
    }

    s = cfg_get("Plugin:Telmon", "Phonebook", "/etc/phonebook");
    strcpy(phoneb, s);
    free(s);

    configured = 1;
    return configured;
}


static void my_telmon(RESULT * result, RESULT * arg1)
{
    char *val = NULL;

    if (configure_telmon() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    if (parse_telmon() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    val = hash_get(&TELMON, R2S(arg1), NULL);
    if (val == NULL)
	val = "";
    SetResult(&result, R_STRING, val);
}


void init()
{
    if (fd != 0)
	return;

    fd = service_connect(ihost, iport);

    if (fd < 0) {
	err++;
    } else if ((ipass != NULL) && (*ipass != '\0')) {	/* Passwort senden */
	char buf[40];
	qprintf(buf, sizeof(buf), "pass %s", ipass);
	send_command(fd, buf);
	get_answer(fd);
    }
}


static int parse_imon(const char *cmd)
{
    /* reread every half sec only */
    int age = hash_age(&IMON, cmd);
    if (age > 0 && age <= 500)
	return 0;

    init();			/* establish connection */

    if (err)
	return -1;

    hash_put(&IMON, cmd, get_value(cmd));

    return 0;
}


static int configure_imon(void)
{
    static int configured = 0;

    char *s;

    if (configured != 0)
	return configured;

    hash_create(&IMON);

    s = cfg_get("Plugin:Imon", "Host", "127.0.0.1");
    if (*s == '\0') {
	error("[Imon] empty 'Host' entry in %s", cfg_source());
	configured = -1;
	return configured;
    }
    strcpy(ihost, s);
    free(s);

    if (cfg_number("Plugin:Imon", "Port", 5000, 1, 65536, &iport) < 0) {
	error("[Imon] no valid port definition");
	configured = -1;
	return configured;
    }

    s = cfg_get("Plugin:Imon", "Pass", "");
    strcpy(ipass, s);
    free(s);

    configured = 1;
    return configured;
}


static void my_imon_version(RESULT * result)
{
    char *val;
    int age;

    if (configure_imon() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    /* read only once */
    age = hash_age(&IMON, "version");
    if (age < 0) {
	char *s;
	init();
	if (err) {
	    SetResult(&result, R_STRING, "");
	    return;
	}
	s = get_value("version");
	for (;;) {		/* interne Versionsnummer killen */
	    if (s[0] == ' ') {
		s = s + 1;
		break;
	    }
	    s = s + 1;
	}
	hash_put(&IMON, "version", s);
    }

    val = hash_get(&IMON, "version", NULL);
    if (val == NULL)
	val = "";
    SetResult(&result, R_STRING, val);
}


static int parse_imon_rates(const char *channel)
{
    char buf[128], in[25], out[25];
    char *s;
    int age;

    qprintf(buf, sizeof(buf), "rate %s in", channel);

    /* reread every half sec only */
    age = hash_age(&IMON, buf);
    if (age > 0 && age <= 500)
	return 0;

    init();			/* establish connection */

    if (err)
	return -1;

    qprintf(buf, sizeof(buf), "rate %s", channel);
    s = get_value(buf);

    if (sscanf(s, "%s %s", in, out) != 2)
	return -1;

    qprintf(buf, sizeof(buf), "rate %s in", channel);
    hash_put(&IMON, buf, in);
    qprintf(buf, sizeof(buf), "rate %s out", channel);
    hash_put(&IMON, buf, out);

    return 0;
}


static void my_imon_rates(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char *val;
    char buf[128];

    if (configure_imon() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    if (parse_imon_rates(R2S(arg1)) < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    qprintf(buf, sizeof(buf), "rate %s %s", R2S(arg1), R2S(arg2));

    val = hash_get(&IMON, buf, NULL);
    if (val == NULL)
	val = "";
    SetResult(&result, R_STRING, val);
}

static int parse_imon_quantity(const char *channel)
{
    char buf[256], fill1[25], in[25], fill2[25], out[25];
    char *s;
    int age;

    qprintf(buf, sizeof(buf), "quantity %s in", channel);

    /* reread every half sec only */
    age = hash_age(&IMON, buf);
    if (age > 0 && age <= 500)
	return 0;

    init();			/* establish connection */

    if (err)
	return -1;

    qprintf(buf, sizeof(buf), "quantity %s", channel);
    s = get_value(buf);

    if (sscanf(s, "%s %s %s %s", fill1, in, fill2, out) != 4)
	return -1;

    qprintf(buf, sizeof(buf), "quantity %s in", channel);
    hash_put(&IMON, buf, in);
    qprintf(buf, sizeof(buf), "quantity %s out", channel);
    hash_put(&IMON, buf, out);

    return 0;
}

static int parse_imon_status(const char *channel)
{
    char buf[256], status[25];
    char *s;
    int age;

    qprintf(buf, sizeof(buf), "status %s", channel);

    /* reread every half sec only */
    age = hash_age(&IMON, buf);
    if (age > 0 && age <= 500)
	return 0;

    init();			/* establish connection */

    if (err)
	return -1;

    qprintf(buf, sizeof(buf), "status %s", channel);
    s = get_value(buf);

    if (sscanf(s, "%s", status) != 1)
	return -1;

    qprintf(buf, sizeof(buf), "status %s", channel);
    if (strcasecmp(status, "Online") == 0)
	hash_put(&IMON, buf, "1");
    else
	hash_put(&IMON, buf, "0");

    return 0;
}

static void my_imon_quantity(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char *val;
    char buf[256];

    if (configure_imon() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    if (parse_imon_quantity(R2S(arg1)) < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    qprintf(buf, sizeof(buf), "quantity %s %s", R2S(arg1), R2S(arg2));

    val = hash_get(&IMON, buf, NULL);
    if (val == NULL)
	val = "";
    SetResult(&result, R_STRING, val);
}

static void my_imon_status(RESULT * result, RESULT * arg1)
{
    char *val;
    char buf[256];

    if (configure_imon() < 0) {
	SetResult(&result, R_NUMBER, "-1");
	return;
    }

    if (parse_imon_status(R2S(arg1)) < 0) {
	SetResult(&result, R_STRING, "-1");
	return;
    }

    qprintf(buf, sizeof(buf), "status %s", R2S(arg1));

    val = hash_get(&IMON, buf, NULL);
    if (val == NULL)
	val = "-1";
    SetResult(&result, R_STRING, val);
}

static void my_imon(RESULT * result, RESULT * arg1)
{
    char *val;
    char *cmd;

    if (configure_imon() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    cmd = R2S(arg1);
    if (parse_imon(cmd) < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    val = hash_get(&IMON, cmd, NULL);
    if (val == NULL)
	val = "";
    SetResult(&result, R_STRING, val);
}


int plugin_init_imon(void)
{
    AddFunction("imon", 1, my_imon);
    AddFunction("imon::version", 0, my_imon_version);
    AddFunction("imon::rates", 2, my_imon_rates);
    AddFunction("imon::quantity", 2, my_imon_quantity);
    AddFunction("imon::status", 1, my_imon_status);
    AddFunction("imon::telmon", 1, my_telmon);

    return 0;
}


void plugin_exit_imon(void)
{
    if (fd > 0) {
	send_command(fd, "quit");
	close(fd);
    }
    hash_destroy(&TELMON);
    hash_destroy(&IMON);
}
