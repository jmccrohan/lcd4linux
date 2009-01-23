/* $Id$
 * $URL$
 *
 * Wireless Extension plugin
 *
 * Copyright (C) 2004 Xavier Vello <xavier66@free.fr>
 * Copyright (C) 2004 Martin Hejl <martin@hejl.de> 
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net> 
 *
 * Losts of code borrowed from Wireless Tools, which is
 *         Copyright (C) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 * (avaible at http://web.hpl.hp.com/personal/Jean_Tourrilhes/Linux/)
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
  * Exported functions:

  wifi_level
  wifi_noise
  wifi_quality
  wifi_protocol
  wifi_frequency
  wifi_bitrate
  wifi_essid
  wifi_op_mode
  wifi_sensitivity
  wifi_sec_mode

  All Functions take one parameter 
  (the name of the device, like "wlan0", "ath0" and so on)  

  */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <linux/if.h>
#include <linux/wireless.h>

#include "debug.h"
#include "plugin.h"
#include "cfg.h"
#include "hash.h"
#include "qprintf.h"


#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define HASH_TTL 100

   /*#define KILO  1e3 */
   /*#define MEGA  1e6 */
   /*#define GIGA  1e9 */


#define KEY_LEVEL "level"
#define KEY_QUALITY "quality"
#define KEY_NOISE "noise"
#define KEY_PROTO "proto"
#define KEY_FREQUENCY "frequency"
#define KEY_BIT_RATE "bit_rate"
#define KEY_ESSID "essid"
#define KEY_OP_MODE "op_mode"
#define KEY_SENS "sens"
#define KEY_SEC_MODE "sec_mode"

#define FREQ2FLOAT(m,e) (((double) m) * pow(10,e))
#define MWATT2DBM(in) ((int) (ceil(10.0 * log10((double) in))))


static HASH wireless;
static int sock = -2;

static char *operation_mode[] = {
    "Auto",
    "Ad-Hoc",
    "Managed",
    "Master",
    "Repeater",
    "Secondary",
    "Monitor",
    "n/a"
};


static void ioctl_error(const int line)
{
    error("IOCTL call to wireless extensions in line %d returned error", line);
}

int do_ioctl(const int sock,	/* Socket to the kernel */
	     const char *ifname,	/* Device name */
	     const int request,	/* WE ID */
	     struct iwreq *pwrq)
{				/* Fixed part of the request */
    int ret;

    /* Set device name */
    strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);

    /* Do the request */
    ret = ioctl(sock, request, pwrq);
    if (ret < 0) {
	debug("ioctl(0x%04x) failed: %d '%s'", request, errno, strerror(errno));
    }

    return ret;

}

int get_range_info(const int sock, const char *ifname, struct iw_range *range)
{
    struct iwreq req;
    char buffer[sizeof(struct iw_range) * 2];	/* Large enough */

    if (sock <= 0) {
	return (-1);
    }

    /* Cleanup */
    memset(buffer, 0, sizeof(buffer));

    req.u.data.pointer = (caddr_t) buffer;
    req.u.data.length = sizeof(buffer);
    req.u.data.flags = 0;
    if (do_ioctl(sock, ifname, SIOCGIWRANGE, &req) < 0)
	return (-1);

    /* Copy stuff at the right place, ignore extra */
    memcpy((char *) range, buffer, sizeof(struct iw_range));

    return (0);
}



static int get_ifname(struct iwreq *preq, const char *dev)
{
    /* do not cache this call !!! */
    struct iwreq req;
    char key_buffer[32];

    if (sock <= 0) {
	return (-1);
    }

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_PROTO);

    if (!preq)
	preq = &req;

    if (do_ioctl(sock, dev, SIOCGIWNAME, preq)) {
	debug("%s isn't a wirelesss interface !", dev);
	return -1;
    }

    hash_put(&wireless, key_buffer, preq->u.name);
    return (0);

}

static int get_frequency(const char *dev, const char *key)
{
    /* Get frequency / channel */
    struct iwreq req;
    char qprintf_buffer[1024];
    char key_buffer[32];
    double freq;
    int age;

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    req.u.data.length = 0;
    req.u.data.flags = 1;	/* Clear updated flag */

    if (do_ioctl(sock, dev, SIOCGIWFREQ, &req) < 0) {
	ioctl_error(__LINE__);
	return -1;
    }

    freq = FREQ2FLOAT(req.u.freq.m, req.u.freq.e);

    /*if( freq>= GIGA)
       snprintf(qprintf_buffer,sizeof(qprintf_buffer), "%g GHz", freq / GIGA);
       else
       if(freq>= MEGA)
       snprintf(qprintf_buffer,sizeof(qprintf_buffer), "%g MHz", freq / MEGA);
       else
       snprintf(qprintf_buffer,sizeof(qprintf_buffer), "%g kHz", freq / KILO);
     */
    snprintf(qprintf_buffer, sizeof(qprintf_buffer), "%g", freq);

    hash_put(&wireless, key_buffer, qprintf_buffer);
    return (0);

}

static int get_essid(const char *dev, const char *key)
{
    /* Get ESSID */
    struct iwreq req;
    char key_buffer[32];
    char essid_buffer[IW_ESSID_MAX_SIZE + 1];
    int age;


    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    memset(essid_buffer, 0, sizeof(essid_buffer));
    req.u.essid.pointer = (caddr_t) essid_buffer;
    req.u.essid.length = IW_ESSID_MAX_SIZE + 1;
    req.u.essid.flags = 0;

    if (do_ioctl(sock, dev, SIOCGIWESSID, &req) < 0) {
	ioctl_error(__LINE__);
	return -1;
    }

    hash_put(&wireless, key_buffer, essid_buffer);
    return (0);

}

static int get_op_mode(const char *dev, const char *key)
{
    /* Get operation mode */
    struct iwreq req;
    char key_buffer[32];
    int age;

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    if (do_ioctl(sock, dev, SIOCGIWMODE, &req) < 0) {
	ioctl_error(__LINE__);
	return -1;
    }

    /* Fixme: req.u.mode is unsigned and therefore never < 0 */
    /* if((req.u.mode > 6) || (req.u.mode < 0)) { */

    if (req.u.mode > 6) {
	req.u.mode = 7;		/* mode not available */
    }

    hash_put(&wireless, key_buffer, operation_mode[req.u.mode]);
    return (0);

}

static int get_bitrate(const char *dev, const char *key)
{
    /* Get bit rate */
    struct iwreq req;
    char key_buffer[32];
    char bitrate_buffer[64];
    int age;
    double bitrate = 0;

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    if (do_ioctl(sock, dev, SIOCGIWRATE, &req) < 0) {
	ioctl_error(__LINE__);
	return -1;
    }

    bitrate_buffer[0] = '\0';
    bitrate = (double) (req.u.bitrate.value);
/*  if( bitrate>= GIGA)
    snprintf(bitrate_buffer,sizeof(bitrate_buffer), "%gGb/s", bitrate / GIGA);
  else
    if(bitrate >= MEGA)
      snprintf(bitrate_buffer,sizeof(bitrate_buffer), "%gMb/s", bitrate / MEGA);
    else
      snprintf(bitrate_buffer,sizeof(bitrate_buffer), "%gkb/s", bitrate / KILO);
*/
    snprintf(bitrate_buffer, sizeof(bitrate_buffer), "%g", bitrate);

    hash_put(&wireless, key_buffer, bitrate_buffer);

    return (0);
}

static int get_sens(const char *dev, const char *key)
{
    /* Get sensitivity */
    struct iwreq req;
    struct iw_range range;
    char key_buffer[32];
    char buffer[64];
    int age;
    int has_sens = 0;
    int has_range = 0;

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    if (do_ioctl(sock, dev, SIOCGIWSENS, &req) >= 0) {
	has_sens = 1;
    }

    if (get_range_info(sock, dev, &range) < 0) {
	memset(&range, 0, sizeof(range));
    } else {
	has_range = 1;
    }

    if (has_range) {
	if (req.u.sens.value < 0) {
	    qprintf(buffer, sizeof(buffer), "%d dBm", req.u.sens.value);
	} else {
	    qprintf(buffer, sizeof(buffer), "%d/%d", req.u.sens.value, range.sensitivity);
	}
    } else {
	qprintf(buffer, sizeof(buffer), "%d", req.u.sens.value);
    }

    hash_put(&wireless, key_buffer, buffer);
    return (0);
}


static int get_sec_mode(const char *dev, const char *key)
{
    /* Get encryption information  */
    struct iwreq req;
    char key_buffer[32];
    char encrypt_key[IW_ENCODING_TOKEN_MAX + 1];
    int age;
    int has_key = 0;
    int key_flags = 0;
    int key_size = 0;

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    req.u.data.pointer = (caddr_t) & encrypt_key;
    req.u.data.length = IW_ENCODING_TOKEN_MAX;
    req.u.data.flags = 0;

    if (do_ioctl(sock, dev, SIOCGIWENCODE, &req) >= 0) {
	has_key = 1;
	key_flags = req.u.data.flags;
	key_size = req.u.data.length;
    } else {
	return (-1);
    }

    /* Display encryption information  */
    /*if(has_key && (key_flags & IW_ENCODE_INDEX) > 1) */
    /*  printf(" [%d]", info->key_flags & IW_ENCODE_INDEX); */

    if (has_key && (key_flags & IW_ENCODE_RESTRICTED))
	hash_put(&wireless, key_buffer, "restricted");
    else if (has_key && (key_flags & IW_ENCODE_OPEN))
	hash_put(&wireless, key_buffer, "open");

    return (0);
}

static int get_stats(const char *dev, const char *key)
{
    struct iw_statistics stats;
    struct iwreq req;
    char qprintf_buffer[1024];
    char key_buffer[32];
    int age;
    struct iw_range range;

    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);
    age = hash_age(&wireless, key);

    /* reread every HASH_TTL msec only */
    if (age > 0 && age <= HASH_TTL) {
	return (0);
    }

    if (get_ifname(&req, dev) != 0) {
	return (-1);
    }

    req.u.data.pointer = (caddr_t) & stats;
    req.u.data.length = sizeof(stats);
    req.u.data.flags = 1;	/* Clear updated flag */

    if (do_ioctl(sock, dev, SIOCGIWSTATS, &req) < 0) {
	ioctl_error(__LINE__);
	return -1;
    }

    if (get_range_info(sock, dev, &range) < 0)
	memset(&range, 0, sizeof(range));

    if (stats.qual.level > range.max_qual.level) {
	qprintf(qprintf_buffer, sizeof(qprintf_buffer), "%d", stats.qual.level - 0x100);
	qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_LEVEL);
	hash_put(&wireless, key_buffer, qprintf_buffer);

	qprintf(qprintf_buffer, sizeof(qprintf_buffer), "%d", stats.qual.noise - 0x100);
	qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_NOISE);
	hash_put(&wireless, key_buffer, qprintf_buffer);

	qprintf(qprintf_buffer, sizeof(qprintf_buffer), "%d/%d", stats.qual.qual, range.max_qual.qual);
	qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_QUALITY);
	hash_put(&wireless, key_buffer, qprintf_buffer);
    } else {

	qprintf(qprintf_buffer, sizeof(qprintf_buffer), "%d/%d", stats.qual.level, range.max_qual.level);
	qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_LEVEL);;
	hash_put(&wireless, key_buffer, qprintf_buffer);

	qprintf(qprintf_buffer, sizeof(qprintf_buffer), "%d/%d", stats.qual.noise, range.max_qual.noise);
	qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_NOISE);
	hash_put(&wireless, key_buffer, qprintf_buffer);

	qprintf(qprintf_buffer, sizeof(qprintf_buffer), "%d/%d", stats.qual.qual, range.max_qual.qual);
	qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, KEY_QUALITY);
	hash_put(&wireless, key_buffer, qprintf_buffer);
    }

    return 0;
}

static int check_socket()
{

    /* already handled in a previous run */
    if (sock == -3)
	return (-1);

    /* socket not initialized */
    if (sock == -2)
	sock = socket(AF_INET, SOCK_DGRAM, 0);

    /* error initilalizing socket */
    if (sock <= 0) {
	error("Error opening socket for reading wireless stats");
	sock = -3;
	return (-1);
    }

    return (0);

}

static void save_result(RESULT * result, const char *dev, const char *key, const int res)
{
    char key_buffer[64];
    char *val = NULL;
    qprintf(key_buffer, sizeof(key_buffer), "%s.%s", dev, key);

    if (res < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    val = hash_get(&wireless, key_buffer, NULL);

    if (val) {
	SetResult(&result, R_STRING, val);
    } else {
	SetResult(&result, R_STRING, "");
    }


}

/* 
 *functions exported to the evaluator 
*/

static void wireless_quality(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_QUALITY, get_stats(dev, KEY_QUALITY));
}

static void wireless_level(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_LEVEL, get_stats(dev, KEY_LEVEL));
}


static void wireless_noise(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_NOISE, get_stats(dev, KEY_NOISE));
}


static void wireless_protocol(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_PROTO, get_ifname(NULL, dev));
}

static void wireless_frequency(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_FREQUENCY, get_frequency(dev, KEY_FREQUENCY));
}

static void wireless_bitrate(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_BIT_RATE, get_bitrate(dev, KEY_BIT_RATE));
}

static void wireless_essid(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_ESSID, get_essid(dev, KEY_ESSID));
}

static void wireless_op_mode(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_OP_MODE, get_op_mode(dev, KEY_OP_MODE));
}

static void wireless_sensitivity(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_SENS, get_sens(dev, KEY_SENS));
}

static void wireless_sec_mode(RESULT * result, RESULT * arg1)
{
    char *dev = R2S(arg1);
    if (check_socket() != 0)
	return;

    save_result(result, dev, KEY_SEC_MODE, get_sec_mode(dev, KEY_SEC_MODE));
}

/*
init and cleanup
*/

int plugin_init_wireless(void)
{
    hash_create(&wireless);

    AddFunction("wifi::level", 1, wireless_level);
    AddFunction("wifi::noise", 1, wireless_noise);
    AddFunction("wifi::quality", 1, wireless_quality);
    AddFunction("wifi::protocol", 1, wireless_protocol);
    AddFunction("wifi::frequency", 1, wireless_frequency);
    AddFunction("wifi::bitrate", 1, wireless_bitrate);
    AddFunction("wifi::essid", 1, wireless_essid);
    AddFunction("wifi::op_mode", 1, wireless_op_mode);
    AddFunction("wifi::sensitivity", 1, wireless_sensitivity);
    AddFunction("wifi::sec_mode", 1, wireless_sec_mode);

    return 0;
}

void plugin_exit_wireless(void)
{
    if (sock > 0)
	close(sock);
    hash_destroy(&wireless);
}
