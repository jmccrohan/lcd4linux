/* $Id: plugin_huawei.c 870 2008-04-10 14:55:23Z michux $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/plugin_huawei.c $
 *
 * plugin plugin_huawei. Copyright (C) 2010 Jar <jar@pcuf.fi>
 *
 * Huawei E220 3G modem statistics via /dev/ttyUSB* user interface. It may (or may not) work well
 * with other Huawei 3G USB modems too (which uses PPP interface between modem and computer), like
 * E160, E169, E226, E272, E230 etc. Since they all seem to use same kind of set AT commands and
 * responses. Tested with Huawei E220, E160E  and Vodafone Huawei K3565.
 *
 * Thanks to Mikko <vmj@linuxbox.fi> for contributing the scan_uint() function and some other code.
 *
 * Based on sample plugin which is
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
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
 * huawei::quality('%'|'dbm'|'rssi')
 * huawei::mode('text'|'number')
 * huawei::manuf()
 * huawei::model()
 * huawei::fwver()
 * huawei::operator()
 * huawei::flowreport('uptime'|'uptime_seconds'|'tx_rate'|'rx_rate'|'total_tx'|'total_rx')
 *
 * With '%' parameter, you can get the percentage (0...100%) value of RSSI (received signal strength).
 * With 'dbm' parameter, you can get the dbm value of RSSI.
 * With 'rssi' parameter, you can get the relative RSSI value 0...31.
 *
 * With 'text' parameter, you can get the mode value as text, like 'HSDPA'.
 * With 'number' parameter, you can get the mode value as number, like 5.
 *
 * With 'uptime' parameter, you can get the uptime (connection time) value as text, like '1 days 10:11:12'.
 * With 'uptime_seconds' parameter, you can get the uptime (connection time) seconds as number, like 12345.
 *
 * This plugin tries to connect per default to the Huawei user interface device at /dev/ttyUSB1.
 * If your modem is connected to another device you can use the environment variable HUAWEI_PORT.
 * For example export HUAWEI_PORT=/dev/ttyUSB2
 *
 */


/* define the include files you need */
#include "config.h"

#include <stdio.h>		/* standard buffered input/output */
#include <stdlib.h>		/* standard library definitions */
#include <stdarg.h>		/* for va_start et al */
#include <string.h>		/* string operations */
#include <unistd.h>		/* read(), close() */
#include <fcntl.h>		/* open() */
#include <termios.h>		/* tcflush() */
#include <sys/types.h>		/* data types */
#include <sys/stat.h>		/* stat structure */
#include <sys/time.h>		/* timeval structure */
#include <errno.h>		/* system error numbers */


/* these should always be included */
#include "debug.h"
#include "plugin.h"


#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


#define PORT "/dev/ttyUSB1"	/* default port */
#define BAUDRATE B38400		/* default baudrate */
#define SEND_BUFFER_SIZE 128	/* send buffer size */
#define RECV_BUFFER_SIZE 256	/* receive buffer size */
#define MIN_INTERVAL 100	/* minimum query interval 100 ms */


#define INIT_STRING	"ATE1;^CURC=0;^DSFLOWCLR"	/* disable unsolicited report codes and reset DS traffic
							 * with local echo enabled 
							 */
#define QUALITY		"ATE1;+CSQ"	/* signal quality query with local echo enabled */
#define SYSINFO		"ATE1;^SYSINFO"	/* network access query with local echo enabled */
#define MANUF		"ATE1;+GMI"	/* manufacturer query with local echo enabled */
#define MODEL		"ATE1;+GMM"	/* model query with local echo enabled */
#define FWVER		"ATE1;+CGMR"	/* firmware version query with local echo enabled */
#define FLOWREPORT	"ATE1;^DSFLOWQRY"	/* DS traffic query with local echo enabled */
#define OPERATOR	"ATE1;+COPS=3,0;+COPS?"	/* gsm/umts operator query (3=set format only, 0=long alphanum. string)
						 * with local echo enabled
						 */

static char name[] = "plugin_huawei.c";

static char *sub_system_mode[] = {
    "NO CONN",			/* no service */
    "GSM",			/* 2G/GSM */
    "GRPS",			/* 2.5G/GPRS */
    "EDGE",			/* 2.75G/EDGE */
    "WCDMA",			/* 3G/UMTS */
    "HSDPA",			/* 3.5G/UMTS */
    "HSUPA",			/* 3.5G/UMTS */
    "HSPA",			/* HSDPA+HSUPA */
    "UNKNOWN"
};

static int fd = -2;		/* serial fd */
static char *port = NULL;	/* serial device */

/* signal strength query */
static unsigned int rssi = 0;	/* relative rssi 0...31 */
static unsigned int ber = 0;	/* ber (bit error rate, not supported) */

/* manufacturer query */
static char manuf[32] = "";	/* manufacturer */

/* model query */
static char model[32] = "";	/* model */

/* fw version query */
static char fwver[32] = "";	/* firmware version */

/* gsm/umts operator query */
static char operator[64] = "";	/* current gsm/umts network operator */

/* DS traffic report query */
static unsigned long int last_ds_time = 0;	/* last DS connection time [s] */
static unsigned long long int last_tx_flow = 0;	/* last DS transmiting trafic [Bytes] */
static unsigned long long int last_rx_flow = 0;	/* last DS receiving trafic [Bytes] */
static unsigned long int total_ds_time = 0;	/* total DS connection time [s] */
static unsigned long long int total_tx_flow = 0;	/* total transmitting DS traffic [Bytes] */
static unsigned long long int total_rx_flow = 0;	/* total receiving DS traffic [Bytes] */

static double calc_tx_rate = 0;	/* calculated tx rate */
static double calc_rx_rate = 0;	/* calculated rx rate */

/* system information query */
static unsigned int status = 0;	/* system service state */
static unsigned int domain = 0;	/* system service domain */
static unsigned int roaming_status = 0;	/* roaming status */
static unsigned int mode = 0;	/* system mode */
static unsigned int sim_state = 0;	/* SIM card state */
static unsigned int reserved = 0;	/* reserved, E618 used it to indicate the simlock state */
static unsigned int sub_mode = 0;	/* system sub mode */

static int debug = 0;		/* enable debug messages */


static int age_diff(struct timeval prev_age)
{
    int diff;
    struct timeval now;

    gettimeofday(&now, NULL);
    diff = (now.tv_sec - prev_age.tv_sec) * 1000 + (now.tv_usec - prev_age.tv_usec) / 1000;

    return diff;
}

static int scan_uint(const char *str, unsigned int nargs, ...)
{
    unsigned int wall = 0;
    unsigned int i = 0;		/* number of parsed numbers */
    unsigned int *a = NULL;	/* pointer to currently parsed number (type
				   must be that of the actual arguments) */

    /* initialize the argument list */
    va_list argv;
    va_start(argv, nargs);

    if (nargs > 0) {
	/* initialize to first argument */
	a = va_arg(argv, unsigned int *);
	/* initialize it to zero */
	*a = 0;
    }

    /* loop through the argument list and the string */
    while (str && *str && wall < 1024) {
	switch (*str) {
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '0':
	    /* intent: multiply current value of 'a' with 10 and add
	       numeric value of character str) */
	    if (i < nargs) {
		*a = ((*a) * 10) + ((*str) - '0');
	    }
	    break;

	case ',':
	    if (++i < nargs) {
		/* proceed to next argument */
		a = va_arg(argv, unsigned int *);
		/* initialize it to zero */
		*a = 0;
	    } else {
		/* this was the last number to parse */
		/* (comma indicates that there are more) */
	    }
	    break;

	default:
	    /* ignore any unknown characters */
	    break;
	}

	/* proceed to next character */
	str++;
	wall++;
    }

    /* fill any extraneous arguments with zero */
    if (str && !(*str) && i < nargs - 1) {
	unsigned int j = i;
	for (j++; j < nargs; j++) {
	    a = va_arg(argv, unsigned int *);
	    *a = 0;
	}
    }

    /* finalize the argument list */
    va_end(argv);

    /* return the number of parsed numbers */
    return (int) (i > 0) ? i + 1 : i;
}

static int huawei_port_exists(const char *device)
{
    int ret;
    struct stat stat_ptr;

    /* check the device is present */
    ret = stat(device, &stat_ptr);
    if (ret < 0)
	return 0;

    /* check the device is character device */
    return (S_ISCHR(stat_ptr.st_mode));
}

static void huawei_read_port(void)
{
    char *test;

    /* port can be defined via env variable */
    if ((test = getenv("HUAWEI_PORT"))) {
	if (port != test) {
	    port = test;
	    info("%s: Using HUAWEI_PORT=%s from env variable for user interface device", name, port);
	}
    } else
	port = PORT;

    return;
}

static int huawei_configure_port(void)
{
    int ret;
    struct termios options;

    /* open port */
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
	error("%s: ERROR: Problem opening %s. Try using the HUAWEI_PORT env variable (export HUAWEI_PORT=/dev/ttyUSB*)",
	      name, port);
	return -1;
    }

    /* enable blocking behavior with timeout */
    ret = fcntl(fd, F_SETFL, 0);
    if (ret < 0) {
	error("%s: ERROR: Problem setting file descriptor status flags %i", name, fd);
	close(fd);
	fd = -2;
	return -1;
    }

    /* get the current options */
    ret = tcgetattr(fd, &options);
    if (ret < 0) {
	error("%s: ERROR: Problem getting terminal attributes %s", name, port);
	close(fd);
	fd = -2;
	return -1;
    }

    /* set raw input */
    options.c_cflag |= (CS8 | CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | IGNCR | INLCR | ICRNL |
			 IUCLC | IXANY | IXON | IXOFF | INPCK | ISTRIP);
    options.c_oflag = 0;	/* raw output */
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;	/* 0.1 second timeout */

    /* set the input baud rate to BAUDRATE */
    ret = cfsetispeed(&options, BAUDRATE);
    if (ret < 0) {
	error("%s: ERROR: Problem setting input baud rate %s", name, port);
	close(fd);
	fd = -2;
	return -1;
    }

    /* set the output baud rate to BAUDRATE */
    ret = cfsetospeed(&options, BAUDRATE);
    if (ret < 0) {
	error("%s: ERROR: Problem setting output baud rate %s", name, port);
	close(fd);
	fd = -2;
	return -1;
    }

    /* set the options */
    ret = tcsetattr(fd, TCSANOW, &options);
    if (ret < 0) {
	error("%s: ERROR: Problem setting terminal attributes %s", name, port);
	close(fd);
	fd = -2;
	return -1;
    }

    /* flush input and output buffers */
    ret = tcflush(fd, TCIFLUSH);
    if (ret < 0) {
	error("%s: ERROR: Problem flushing input and output buffers %s", name, port);
	close(fd);
	fd = -2;
	return -1;
    }

    return fd;
}

static int huawei_send(const char *cmd)
{
    int len, bytes;
    char buf[SEND_BUFFER_SIZE];

    sprintf(buf, "%s\r", cmd);
    len = strlen(buf);

    /* write */
    bytes = write(fd, buf, len);

    /* force null termination */
    if (bytes > 0)
	buf[bytes] = '\0';
    else
	buf[0] = '\0';

    if (bytes < 0 && errno != EAGAIN) {
	error("%s: ERROR: Writing to device %s failed: %s", name, port, strerror(errno));
	return -1;
    } else if (bytes != len) {
	error("%s: ERROR: Partial write when writing to device %s", name, port);
	return -1;
    }

    /* remove trailing CR */
    while (bytes > 0 && buf[bytes - 1] == '\r') {
	buf[--bytes] = '\0';
    }

    if (debug)
	debug("DEBUG: <-Send bytes=%i, send buf=%s", bytes, buf);

    return bytes;
}

static char *huawei_receive(void)
{
    int run, count, bytes;
    char c[1], *p;
    static char buf[RECV_BUFFER_SIZE];

    /* skip the plain NL's and CR's (<CR><LF><CR><LF> makes max. 4 pcs. of them) and re-read */
    for (run = 0; run < 4; run++) {

	/* pointer to the current position in the buffer */
	p = buf;

	/* set count to buffer size */
	count = RECV_BUFFER_SIZE;

	/* Read until a NL or a CR is encountered. If 'buf' fills up before newline is seen,
	 * exit the loop when there's still room for zero termination (count == 1).
	 */
	while (count > 1) {
	    bytes = read(fd, c, 1);

	    if (*c == '\n' || *c == '\r' || (bytes < 0 && errno != EAGAIN))
		break;

	    *p = *c;
	    p++;
	    count--;
	}

	/* force null termination */
	*p = '\0';

	/* now we have data (when buf len > 0), exit from for-loop and return the data to caller */
	if (strlen(buf) > 0 || (bytes < 0 && errno != EAGAIN))
	    break;
    }

    if (bytes < 0 && errno != EAGAIN) {
	error("%s: ERROR: Reading from device %s failed: %s", name, port, strerror(errno));
	return ("");
    }

    /* print bytes read and buffer data when debuging enabled */
    if (debug)
	debug("DEBUG: ->Received bytes=%i, receive buf=%s", (int) strlen(buf), buf);

    return (buf);
}

static int huawei_recv_error(const char *msg)
{
    if (strstr(msg, "ERROR") != NULL || strncmp(msg, "NO CARRIER", 10) == 0 ||
	strncmp(msg, "COMMAND NOT SUPPORT", 19) == 0 || strncmp(msg, "TOO MANY PARAMETERS", 19) == 0)
	return 1;

    return 0;
}

static char *huawei_send_receive(const char *cmd)
{
    int run, bytes_send, recv_seq;
    char *tmp;
    static char reply[RECV_BUFFER_SIZE];

    /* send command to modem */
    bytes_send = huawei_send(cmd);

    /* start with an empty reply */
    strcpy(reply, "");

    if (bytes_send > 0) {

	recv_seq = 0;

	/* receive modem responses (currently handle only 3 lines of responses:
	 * request->reply->ok|error)
	 */
	for (run = 0; run < 3; run++) {

	    tmp = huawei_receive();

	    /* init cmd */
	    if (strncmp(cmd, INIT_STRING, strlen(INIT_STRING)) == 0) {

		/* waiting local echo back after the init string has been sent */
		if (recv_seq == 0 && strncmp(tmp, INIT_STRING, strlen(INIT_STRING)) == 0) {
		    recv_seq = 1;
		}

		/* waiting "OK" after local echo has been received */
		else if (recv_seq == 1 && strncmp(tmp, "OK", 2) == 0) {
		    strncpy(reply, tmp, sizeof(reply) - 1);
		    break;
		}

		/* waiting possible "ERROR" after local echo has been received */
		else if (recv_seq == 1 && huawei_recv_error(tmp) == 1) {
		    strncpy(reply, tmp, sizeof(reply) - 1);
		    break;
		}

		/* data query cmd */
	    } else {

		/* waiting local echo back after the cmd has been sent */
		if (recv_seq == 0 && strncmp(tmp, cmd, strlen(cmd)) == 0) {
		    recv_seq = 1;
		}

		/* waiting data (but not "OK" or "ERROR") after local echo has been received */
		else if (recv_seq == 1 && strlen(tmp) > 0 && huawei_recv_error(tmp) == 0) {
		    recv_seq = 2;
		    strncpy(reply, tmp, sizeof(reply) - 1);
		}

		/* waiting "OK" after data has been received */
		else if (recv_seq == 2 && strncmp(tmp, "OK", 2) == 0) {
		    break;
		}

		/* waiting possible "ERROR" after local echo has been received */
		else if (recv_seq == 1 && huawei_recv_error(tmp) == 1) {
		    strncpy(reply, tmp, sizeof(reply) - 1);
		    break;
		}
	    }
	}
    }

    return (reply);
}

static int huawei_configured(void)
{
    int port_exists, ret, bytes;
    static int connected = -1, configured = 0;
    char *buf;

    /* re-read port because device name may change during plugin execution */
    huawei_read_port();

    /* check the device exists before reading on it */
    port_exists = huawei_port_exists(port);

    /* modem removed event */
    if (port_exists < 1 && (connected == -1 || connected == 1)) {
	info("%s: Modem doesn't exists or has been removed, device %s is to be closed", name, port);
	if (fd > 0) {
	    ret = close(fd);
	    if (ret < 0) {
		error("%s: ERROR: Error when closing device %s after modem removal, ret=%i", name, port, ret);
	    }
	    fd = -2;
	}
	connected = 0;
    }

    /* modem inserted event */
    if (port_exists > 0 && (connected == -1 || connected == 0)) {
	info("%s: Modem has been inserted, device %s will be opened", name, port);
	if (fd < 0) {
	    ret = huawei_configure_port();
	    if (ret > 0) {
		connected = 1;
		if (debug)
		    debug("DEBUG: Device %s configured successfully, fd=%i", port, ret);
	    } else {
		error("%s: ERROR: Device %s configuring failure->retrying..., ret=%i", name, port, ret);
	    }
	}
    }

    /* init variables to zero (except tx/rx total flows) when device disconnected */
    if (connected == 0) {
	rssi = 0;
	sub_mode = 0;
	last_ds_time = 0;
	total_ds_time = 0;
	last_tx_flow = 0;
	last_rx_flow = 0;
	calc_tx_rate = 0;
	calc_rx_rate = 0;
	strcpy(manuf, "");
	strcpy(model, "");
	strcpy(fwver, "");
	strcpy(operator, "");
	configured = 0;
    }

    /* modem initialization */
    if (connected == 1 && configured != 1) {
	buf = huawei_send_receive(INIT_STRING);
	bytes = strlen(buf);

	if (strncmp(buf, "OK", 2) == 0) {
	    configured = 1;
	    info("%s: Modem user inerface succesfully initialized to: \'%s\'", name, INIT_STRING);
	} else {
	    configured = 0;
	    error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, INIT_STRING);
	}
    }

    return configured;
}

static void huawei_read_quality(const char *cmd)
{
    int bytes;
    char *buf;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    if (strncmp(buf, "+CSQ: ", 6) == 0) {

	/* Returns relative signal strength (RSSI) and ber (not supported by modems).
	 *
	 * +CSQ: 14,99
	 *     rssi,ber
	 *
	 *  0 <= -113 dBm
	 *  1 -111 dBm
	 *  2 to 30 -109 dBm to -53 dBm
	 *  31 >= -51 dBm
	 *  99 unknown or unmeasurable
	 */

	if (scan_uint(buf + 6, 2, &rssi, &ber) != 2)
	    error("%s: ERROR: Cannot parse all +CSQ: data fields, some data may be wrong or missing", name);

	if (debug)
	    debug("DEBUG: Relative rssi value: %u", rssi);
    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void huawei_read_sysinfo(const char *cmd)
{
    int bytes;
    char *buf;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    if (strncmp(buf, "^SYSINFO:", 9) == 0) {

	/* Returns system information.
	 *
	 * ^SYSINFO:2,3,0,5,1,,4
	 *     status,domain,roaming_status,mode,SIM state,reserved,sub_mode
	 */

	if (scan_uint(buf + 9, 7, &status, &domain, &roaming_status, &mode, &sim_state, &reserved, &sub_mode) != 7)
	    error("%s: ERROR: Cannot parse all ^SYSINFO: data fields, some data may be wrong or missing", name);

	if (debug)
	    debug("DEBUG: Sub mode value: %u", sub_mode);
    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void huawei_read_manuf(const char *cmd)
{
    int bytes;
    char *buf;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    /* accept all but "ERROR" */
    if (bytes > 0 && huawei_recv_error(buf) == 0) {

	/* Returns manufacturer string.
	 * 
	 * huawei
	 */
	strncpy(manuf, buf, sizeof(manuf) - 1);

	if (debug)
	    debug("DEBUG: Manufacturer string: %s", manuf);
    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void huawei_read_model(const char *cmd)
{
    int bytes;
    char *buf;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    /* accept all but "ERROR" */
    if (bytes > 0 && huawei_recv_error(buf) == 0) {

	/* Returns model string.
	 * 
	 * E220
	 */
	strncpy(model, buf, sizeof(model) - 1);

	if (debug)
	    debug("DEBUG: Model string: %s", model);
    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void huawei_read_fwver(const char *cmd)
{
    int bytes;
    char *buf;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    /* accept all but "ERROR" */
    if (bytes > 0 && huawei_recv_error(buf) == 0) {

	/* Returns firmware version string.
	 * 
	 * 11.110.03.00.00
	 */
	strncpy(fwver, buf, sizeof(fwver) - 1);

	if (debug)
	    debug("DEBUG: Firmware version string: %s", fwver);
    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void huawei_read_operator(const char *cmd)
{
    int bytes, i, pos, copy;
    char *buf;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    if (strncmp(buf, "+COPS:", 6) == 0) {

	/* Returns operator string.
	 * 
	 * +COPS: 0,0,"vodafone ES",2
	 * reg_mode,format,operator string (based on format),network access type
	 */

	/* parse "operator string" */
	pos = 0;
	copy = 0;

	for (i = 6; i <= bytes; i++) {
	    if (buf[i] == '\"' && copy == 0) {
		i++;
		copy = 1;
	    } else if ((buf[i] == '\"') && copy == 1)
		copy = 0;

	    if (copy == 1 && strlen(operator) < sizeof(operator) - 1) {
		operator[pos] = buf[i];
		pos++;
	    }
	}

	if (debug)
	    debug("DEBUG: Operator version string: \'%s\'", operator);
    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void huawei_read_flowreport(const char *cmd)
{
    char *buf;
    int bytes;
    static unsigned long long int prev_tx_flow = 0, prev_rx_flow = 0;
    static unsigned long int prev_ds_time = 0;

    buf = huawei_send_receive(cmd);
    bytes = strlen(buf);

    if (strncmp(buf, "^DSFLOWQRY:", 11) == 0) {

	/* Returns flow report.
	 *
	 * ^DSFLOWQRY:00000E8E,0000000000333ACC,000000000A93E9AD,007B3514,000000007C0E5F69,00000007AD1AE9C6
	 *        last_ds_time,last_rx_flow    ,last_tx_flow    ,total_ds_time,total_tx_flow,total_rx_flow
	 */

	if (sscanf(buf + 11, "%lX,%LX,%LX,%lX,%LX,%LX", &last_ds_time, &last_tx_flow, &last_rx_flow, &total_ds_time,
		   &total_tx_flow, &total_rx_flow) != 6)
	    error("%s: ERROR: Cannot parse all ^DSFLOWQRY: data fields, some data may be wrong or missing", name);

	/* ^DSFLOWQRY: lacks tx_rate and rx_rate values (^DSFLOWRPT has them), we try to calculate them here. 
	 * Values comes at 2s interval.
	 */

	/* tx_rate calculation */
	if (last_ds_time >= prev_ds_time + 2 && last_tx_flow > prev_tx_flow)
	    calc_tx_rate = ((double) (last_tx_flow - prev_tx_flow)) / ((double) (last_ds_time - prev_ds_time));
	else if (last_ds_time >= prev_ds_time + 2 && last_tx_flow == prev_tx_flow)
	    calc_tx_rate = 0;

	/* rx_rate calculation */
	if (last_ds_time >= prev_ds_time + 2 && last_rx_flow > prev_rx_flow)
	    calc_rx_rate = ((double) (last_rx_flow - prev_rx_flow)) / ((double) (last_ds_time - prev_ds_time));
	else if (last_ds_time >= prev_ds_time + 2 && last_rx_flow == prev_rx_flow)
	    calc_rx_rate = 0;

	/* previous values */
	prev_tx_flow = last_tx_flow;
	prev_rx_flow = last_rx_flow;
	prev_ds_time = last_ds_time;

	if (debug) {
	    debug("DEBUG: Last DS connection time [s]: %lu", last_ds_time);
	    debug("DEBUG: Last DS transmiting traffic [bytes]: %llu", last_tx_flow);
	    debug("DEBUG: Last DS receiving traffic [bytes]: %llu", last_rx_flow);
	    debug("DEBUG: Total DS connection time [s]: %lu", total_ds_time);
	    debug("DEBUG: Total DS transmiting trafic [bytes]: %llu", total_tx_flow);
	    debug("DEBUG: Total DS receiving trafic [bytes]: %llu", total_rx_flow);
	    debug("DEBUG: Calculated tx rate [bytes/s]: %lf", calc_tx_rate);
	    debug("DEBUG: Calculated rx rate [bytes/s]: %lf", calc_rx_rate);
	}

    } else
	error("%s: ERROR: Invalid or empty response: \'%s\' received for: \'%s\'", name, buf, cmd);

    return;
}

static void my_quality(RESULT * result, RESULT * arg1)
{
    int age;
    static struct timeval prev_age;
    static double value;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_quality(QUALITY);
    }

    /* Note: R2S stands for 'Result to String' */
    if (strncmp(R2S(arg1), "%", 1) == 0) {
	/* scale rssi 0...31 to 0..100% value */
	if (rssi > 0 && rssi < 32)
	    value = (double) rssi *100 / 31;
	else if (rssi == 0)
	    value = 0;

    } else if (strncmp(R2S(arg1), "dbm", 3) == 0) {
	/* scale rssi 0...31 to -113 dBm...-51 dBm value */
	if (rssi > 0 && rssi < 32)
	    value = ((double) rssi * 2) - 113;
	else if (rssi == 0)
	    value = -113;

    } else if (strncmp(R2S(arg1), "rssi", 4) == 0) {
	/* pass through relative rssi 0...31 value */
	if (rssi > 0 && rssi < 32)
	    value = (double) rssi;
	else if (rssi == 0)
	    value = 0;

    } else {
	error("%s: ERROR: Argument for huawei::quality() is missing, give: '%%'|'dbm'|'rssi'", name);
	value = 0;
    }

    /* store result */
    SetResult(&result, R_NUMBER, &value);

    return;
}

static void my_mode(RESULT * result, RESULT * arg1)
{
    int age;
    static struct timeval prev_age;
    double value_num;
    char *value_str, *mode_str;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_sysinfo(SYSINFO);
    }

    if (strncmp(R2S(arg1), "text", 4) == 0) {
	/* sub modes 8 and 9 are unknown */
	if (sub_mode > 7)
	    sub_mode = 8;

	/* start with an empty string */
	value_str = strdup("");

	/* mode string */
	mode_str = sub_system_mode[sub_mode];

	/* allocate memory for value */
	value_str = realloc(value_str, strlen(mode_str) + 1);

	/* write mode string to value */
	strcat(value_str, mode_str);

	/* store result */
	SetResult(&result, R_STRING, value_str);

	/* free local string */
	free(value_str);

    } else if (strncmp(R2S(arg1), "number", 6) == 0) {
	value_num = (double) mode;

	/* store result */
	SetResult(&result, R_NUMBER, &value_num);

    } else {
	error("%s: ERROR: Argument for huawei::mode() is missing, give: 'text'|'number'", name);

	value_num = 0;

	/* store result */
	SetResult(&result, R_NUMBER, &value_num);
    }

    return;
}

static void my_manuf(RESULT * result)
{
    int age;
    static struct timeval prev_age;
    char *value_str;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_manuf(MANUF);
    }

    /* start with an empty string */
    value_str = strdup("");

    /* allocate memory for value */
    value_str = realloc(value_str, strlen(manuf) + 1);

    /* write mode string to value */
    strcat(value_str, manuf);

    /* store result */
    SetResult(&result, R_STRING, value_str);

    /* free local string */
    free(value_str);

    return;
}

static void my_model(RESULT * result)
{
    int age;
    static struct timeval prev_age;
    char *value_str;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_model(MODEL);
    }

    /* start with an empty string */
    value_str = strdup("");

    /* allocate memory for value */
    value_str = realloc(value_str, strlen(model) + 1);

    /* write mode string to value */
    strcat(value_str, model);

    /* store result */
    SetResult(&result, R_STRING, value_str);

    /* free local string */
    free(value_str);

    return;
}

static void my_fwver(RESULT * result)
{
    int age;
    static struct timeval prev_age;
    char *value_str;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_fwver(FWVER);
    }

    /* start with an empty string */
    value_str = strdup("");

    /* allocate memory for value */
    value_str = realloc(value_str, strlen(fwver) + 1);

    /* write mode string to value */
    strcat(value_str, fwver);

    /* store result */
    SetResult(&result, R_STRING, value_str);

    /* free local string */
    free(value_str);

    return;
}

static void my_operator(RESULT * result)
{
    int age;
    static struct timeval prev_age;
    char *value_str;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_operator(OPERATOR);
    }

    /* start with an empty string */
    value_str = strdup("");

    /* allocate memory for value */
    value_str = realloc(value_str, strlen(operator) + 1);

    /* write mode string to value */
    strcat(value_str, operator);

    /* store result */
    SetResult(&result, R_STRING, value_str);

    /* free local string */
    free(value_str);

    return;
}

static void my_flowreport(RESULT * result, RESULT * arg1)
{
    int age;
    unsigned int days, hours, mins, secs;
    double value_num;
    char value_str[32];
    static struct timeval prev_age;

    age = age_diff(prev_age);

    if (age < 0 || age >= MIN_INTERVAL) {
	gettimeofday(&prev_age, NULL);

	if (huawei_configured() == 1)
	    huawei_read_flowreport(FLOWREPORT);
    }

    if (strncmp(R2S(arg1), "uptime", 6) == 0) {

	days = last_ds_time / 86400;
	hours = (last_ds_time / 3600) - (days * 24);
	mins = (last_ds_time / 60) - (days * 1440) - (hours * 60);
	secs = last_ds_time % 60;

	if (days > 0)
	    sprintf(value_str, "%u days %02u:%02u:%02u", days, hours, mins, secs);
	else
	    sprintf(value_str, "%02u:%02u:%02u", hours, mins, secs);

	/* store result: days, hours, mins, secs */
	SetResult(&result, R_STRING, value_str);

	return;
    }

    if (strncmp(R2S(arg1), "uptime_seconds", 14) == 0) {
	/* uptime in seconds */
	value_num = (double) last_ds_time;

	/* store result: seconds */
	SetResult(&result, R_NUMBER, &value_num);

	return;
    }

    if (strncmp(R2S(arg1), "tx_rate", 7) == 0) {
	/* tx data in Bytes/s */
	value_num = calc_tx_rate;

	/* store result */
	SetResult(&result, R_NUMBER, &value_num);

	return;
    }

    if (strncmp(R2S(arg1), "rx_rate", 7) == 0) {
	/* rx data in Bytes/s */
	value_num = calc_rx_rate;

	/* store result */
	SetResult(&result, R_NUMBER, &value_num);

	return;
    }

    if (strncmp(R2S(arg1), "total_tx", 8) == 0) {
	/* total rx data in Bytes */
	value_num = (double) total_tx_flow;

	/* store result */
	SetResult(&result, R_NUMBER, &value_num);

	return;
    }

    if (strncmp(R2S(arg1), "total_rx", 8) == 0) {
	/* total rx data in Bytes */
	value_num = (double) total_rx_flow;

	/* store result */
	SetResult(&result, R_NUMBER, &value_num);

	return;
    }

    error
	("%s: ERROR: Argument for huawei::flowreport() is missing, give: 'uptime'|'uptime_seconds'|'tx_rate'|'rx_rate'|'total_tx'|'total_rx')",
	 name);
    value_num = 0;

    /* store result */
    SetResult(&result, R_NUMBER, &value_num);

    return;
}

/* plugin initialization. MUST NOT be declared 'static'! */
int plugin_init_huawei(void)
{
    /* register our functions */
    AddFunction("huawei::quality", 1, my_quality);
    AddFunction("huawei::mode", 1, my_mode);
    AddFunction("huawei::model", 0, my_model);
    AddFunction("huawei::manuf", 0, my_manuf);
    AddFunction("huawei::fwver", 0, my_fwver);
    AddFunction("huawei::operator", 0, my_operator);
    AddFunction("huawei::flowreport", 1, my_flowreport);

    return 0;
}

void plugin_exit_huawei(void)
{
    int ret;

    /* close file descriptor */
    if (fd > 0) {
	ret = close(fd);
	if (ret < 0)
	    error("%s: ERROR: Device %s closing failure on plugin_exit, %i", name, port, ret);
    }
    fd = -2;
}
