/* $Id: $
 * $URL: $
 *
 * plugin getting information about network devices (IPaddress, MACaddress, ...)
 *
 * Copyright (C) 2007 Volker Gering <v.gering@t-online.de>
 * Copyright (C) 2004, 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * int plugin_init_netinfo (void)
 *  adds functions to get information about network devices
 *
 */


#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "plugin.h"
#include "qprintf.h"

#include <sys/types.h>		/* socket() */
#include <sys/socket.h>		/* socket() */
#include <sys/ioctl.h>		/* SIOCGIFNAME */
#include <net/if.h>		/* ifreq{} */
#include <errno.h>		/* errno */
#include <netinet/in.h>		/* inet_ntoa() */
#include <arpa/inet.h>		/* inet_ntoa() */


/* socket descriptor:
 * -2   initial state
 * -1   error: message not printed
 * -3   error: message printed, deactivate plugin
 */
static int socknr = -2;

static int open_net(void)
{

    if (socknr == -3)
	return -1;
    if (socknr == -2) {
	socknr = socket(PF_INET, SOCK_STREAM, 0);
    }
    if (socknr == -1) {
	error("%s: socket(PF_INET, SOCK_STREAM, 0) failed: %s", "plugin_netinfo", strerror(errno));
	error("  deactivate plugin netinfo");
	socknr = -3;
	return -1;
    }

    return 0;
}


static void my_exists(RESULT * result, RESULT * arg1)
{
    static int errcount = 0;
    struct ifreq ifreq;
    double value = 1.0;		// netdev exists

    if (socknr < 0) {
	/* no open socket */
	value = 0.0;
	SetResult(&result, R_NUMBER, &value);
	return;
    }

    strncpy(ifreq.ifr_name, R2S(arg1), sizeof(ifreq.ifr_name));
    if (ioctl(socknr, SIOCGIFFLAGS, &ifreq) < 0) {
	if (errno == ENODEV) {
	    /* device does not exists */
	    value = 0.0;
	} else {
	    errcount++;
	    if (1 == errcount % 1000) {
		error("%s: ioctl(FLAGS %s) failed: %s", "plugin_netinfo", ifreq.ifr_name, strerror(errno));
		error("  (skip next 1000 errors)");
	    }
	    return;
	}
    }

    /* device exists */
    SetResult(&result, R_NUMBER, &value);
}


/* get MAC address (hardware address) of network device */
static void my_hwaddr(RESULT * result, RESULT * arg1)
{
    static int errcount = 0;
    struct ifreq ifreq;
    unsigned char *hw;
    char value[18];

    if (socknr < 0) {
	/* no open socket */
	SetResult(&result, R_STRING, "");
	return;
    }

    strncpy(ifreq.ifr_name, R2S(arg1), sizeof(ifreq.ifr_name));
    if (ioctl(socknr, SIOCGIFHWADDR, &ifreq) < 0) {
	errcount++;
	if (1 == errcount % 1000) {
	    error("%s: ioctl(IFHWADDR %s) failed: %s", "plugin_netinfo", ifreq.ifr_name, strerror(errno));
	    error("  (skip next 1000 errors)");
	}
	SetResult(&result, R_STRING, "");
	return;
    }
    hw = (unsigned char *) ifreq.ifr_hwaddr.sa_data;
    qprintf(value, sizeof(value), "%02x:%02x:%02x:%02x:%02x:%02x",
	    *hw, *(hw + 1), *(hw + 2), *(hw + 3), *(hw + 4), *(hw + 5));

    SetResult(&result, R_STRING, value);
}


/* get ip address of network device */
static void my_ipaddr(RESULT * result, RESULT * arg1)
{
    static int errcount = 0;
    struct ifreq ifreq;
    struct sockaddr_in *sin;
    char value[16];

    if (socknr < 0) {
	/* no open socket */
	SetResult(&result, R_STRING, "");
	return;
    }

    strncpy(ifreq.ifr_name, R2S(arg1), sizeof(ifreq.ifr_name));
    if (ioctl(socknr, SIOCGIFADDR, &ifreq) < 0) {
	errcount++;
	if (1 == errcount % 1000) {
	    error("%s: ioctl(IFADDR %s) failed: %s", "plugin_netinfo", ifreq.ifr_name, strerror(errno));
	    error("  (skip next 1000 errors)");
	}
	SetResult(&result, R_STRING, "");
	return;
    }
    sin = (struct sockaddr_in *) &ifreq.ifr_addr;
    qprintf(value, sizeof(value), "%s", inet_ntoa(sin->sin_addr));

    SetResult(&result, R_STRING, value);
}


/* get ip netmask of network device */
static void my_netmask(RESULT * result, RESULT * arg1)
{
    static int errcount = 0;
    struct ifreq ifreq;
    struct sockaddr_in *sin;
    char value[16];

    if (socknr < 0) {
	/* no open socket */
	SetResult(&result, R_STRING, "");
	return;
    }

    strncpy(ifreq.ifr_name, R2S(arg1), sizeof(ifreq.ifr_name));
    if (ioctl(socknr, SIOCGIFNETMASK, &ifreq) < 0) {
	errcount++;
	if (1 == errcount % 1000) {
	    error("%s: ioctl(IFNETMASK %s) failed: %s", "plugin_netinfo", ifreq.ifr_name, strerror(errno));
	    error("  (skip next 1000 errors)");
	}
	SetResult(&result, R_STRING, "");
	return;
    }
    sin = (struct sockaddr_in *) &ifreq.ifr_netmask;
    qprintf(value, sizeof(value), "%s", inet_ntoa(sin->sin_addr));

    SetResult(&result, R_STRING, value);
}


/* get ip broadcast address of network device */
static void my_bcaddr(RESULT * result, RESULT * arg1)
{
    static int errcount = 0;
    struct ifreq ifreq;
    struct sockaddr_in *sin;
    char value[16];

    if (socknr < 0) {
	/* no open socket */
	SetResult(&result, R_STRING, "");
	return;
    }

    strncpy(ifreq.ifr_name, R2S(arg1), sizeof(ifreq.ifr_name));
    if (ioctl(socknr, SIOCGIFBRDADDR, &ifreq) < 0) {
	errcount++;
	if (1 == errcount % 1000) {
	    error("%s: ioctl(IFBRDADDR %s) failed: %s", "plugin_netinfo", ifreq.ifr_name, strerror(errno));
	    error("  (skip next 1000 errors)");
	}
	SetResult(&result, R_STRING, "");
	return;
    }
    sin = (struct sockaddr_in *) &ifreq.ifr_broadaddr;
    qprintf(value, sizeof(value), "%s", inet_ntoa(sin->sin_addr));

    SetResult(&result, R_STRING, value);
}


int plugin_init_netinfo(void)
{
    open_net();

    AddFunction("netinfo::exists", 1, my_exists);
    AddFunction("netinfo::hwaddr", 1, my_hwaddr);
    AddFunction("netinfo::ipaddr", 1, my_ipaddr);
    AddFunction("netinfo::netmask", 1, my_netmask);
    AddFunction("netinfo::bcaddr", 1, my_bcaddr);

    return 0;
}

void plugin_exit_netinfo(void)
{
    close(socknr);
}
