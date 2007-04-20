/* $Id$
 * $URL$
 *
 * plugin for /proc/stat parsing
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
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
 */

/* 
 * exported functions:
 *
 * int plugin_init_proc_stat (void)
 *  adds functions to access /proc/stat
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "plugin.h"
#include "qprintf.h"
#include "hash.h"


static HASH Stat;
static FILE *stream = NULL;


static void hash_put1(const char *key1, const char *val)
{
    hash_put_delta(&Stat, key1, val);
}


static void hash_put2(const char *key1, const char *key2, const char *val)
{
    char key[32];

    qprintf(key, sizeof(key), "%s.%s", key1, key2);
    hash_put1(key, val);
}


static void hash_put3(const char *key1, const char *key2, const char *key3, const char *val)
{
    char key[32];

    qprintf(key, sizeof(key), "%s.%s.%s", key1, key2, key3);
    hash_put1(key, val);
}


static int parse_proc_stat(void)
{
    int age;

    /* reread every 10 msec only */
    age = hash_age(&Stat, NULL);
    if (age > 0 && age <= 10)
	return 0;

    if (stream == NULL)
	stream = fopen("/proc/stat", "r");
    if (stream == NULL) {
	error("fopen(/proc/stat) failed: %s", strerror(errno));
	return -1;
    }

    rewind(stream);

    while (!feof(stream)) {
	char buffer[1024];
	if (fgets(buffer, sizeof(buffer), stream) == NULL)
	    break;

	if (strncmp(buffer, "cpu", 3) == 0) {
	    char *key[] = { "user", "nice", "system", "idle", "iow", "irq", "sirq" };
	    char delim[] = " \t\n";
	    char *cpu, *beg, *end;
	    int i;

	    cpu = buffer;

	    /* skip "cpu" or "cpu0" block */
	    if ((end = strpbrk(buffer, delim)) != NULL)
		*end = '\0';
	    beg = end ? end + 1 : NULL;

	    for (i = 0; i < 7 && beg != NULL; i++) {
		while (strchr(delim, *beg))
		    beg++;
		if ((end = strpbrk(beg, delim)))
		    *end = '\0';
		hash_put2(cpu, key[i], beg);
		beg = end ? end + 1 : NULL;
	    }
	}

	else if (strncmp(buffer, "page ", 5) == 0) {
	    char *key[] = { "in", "out" };
	    char delim[] = " \t\n";
	    char *beg, *end;
	    int i;

	    for (i = 0, beg = buffer + 5; i < 2 && beg != NULL; i++) {
		while (strchr(delim, *beg))
		    beg++;
		if ((end = strpbrk(beg, delim)))
		    *end = '\0';
		hash_put2("page", key[i], beg);
		beg = end ? end + 1 : NULL;
	    }
	}

	else if (strncmp(buffer, "swap ", 5) == 0) {
	    char *key[] = { "in", "out" };
	    char delim[] = " \t\n";
	    char *beg, *end;
	    int i;

	    for (i = 0, beg = buffer + 5; i < 2 && beg != NULL; i++) {
		while (strchr(delim, *beg))
		    beg++;
		if ((end = strpbrk(beg, delim)))
		    *end = '\0';
		hash_put2("swap", key[i], beg);
		beg = end ? end + 1 : NULL;
	    }
	}

	else if (strncmp(buffer, "intr ", 5) == 0) {
	    char delim[] = " \t\n";
	    char *beg, *end, num[4];
	    int i;

	    for (i = 0, beg = buffer + 5; i < 17 && beg != NULL; i++) {
		while (strchr(delim, *beg))
		    beg++;
		if ((end = strpbrk(beg, delim)))
		    *end = '\0';
		if (i == 0)
		    strcpy(num, "sum");
		else
		    qprintf(num, sizeof(num), "%d", i - 1);
		hash_put2("intr", num, beg);
		beg = end ? end + 1 : NULL;
	    }
	}

	else if (strncmp(buffer, "disk_io:", 8) == 0) {
	    char *key[] = { "io", "rio", "rblk", "wio", "wblk" };
	    char delim[] = " ():,\t\n";
	    char *dev, *beg, *end, *p;
	    int i;

	    dev = buffer + 8;
	    while (dev != NULL) {
		while (strchr(delim, *dev))
		    dev++;
		if ((end = strchr(dev, ')')))
		    *end = '\0';
		while ((p = strchr(dev, ',')) != NULL)
		    *p = ':';
		beg = end ? end + 1 : NULL;
		for (i = 0; i < 5 && beg != NULL; i++) {
		    while (strchr(delim, *beg))
			beg++;
		    if ((end = strpbrk(beg, delim)))
			*end = '\0';
		    hash_put3("disk_io", dev, key[i], beg);
		    beg = end ? end + 1 : NULL;
		}
		dev = beg;
	    }
	}

	else {
	    char delim[] = " \t\n";
	    char *beg, *end;

	    beg = buffer;
	    if ((end = strpbrk(beg, delim)))
		*end = '\0';
	    beg = end ? end + 1 : NULL;
	    if ((end = strpbrk(beg, delim)))
		*end = '\0';
	    while (strchr(delim, *beg))
		beg++;
	    hash_put1(buffer, beg);
	}
    }
    return 0;
}


static void my_proc_stat(RESULT * result, const int argc, RESULT * argv[])
{
    char *string;
    double number;

    if (parse_proc_stat() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    switch (argc) {
    case 1:
	string = hash_get(&Stat, R2S(argv[0]), NULL);
	if (string == NULL)
	    string = "";
	SetResult(&result, R_STRING, string);
	break;
    case 2:
	number = hash_get_delta(&Stat, R2S(argv[0]), NULL, R2N(argv[1]));
	SetResult(&result, R_NUMBER, &number);
	break;
    default:
	error("proc_stat(): wrong number of parameters");
	SetResult(&result, R_STRING, "");
    }
}


static void my_cpu(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char *key;
    int delay;
    double value;
    double cpu_user, cpu_nice, cpu_system, cpu_idle, cpu_total;
    double cpu_iow, cpu_irq, cpu_sirq;

    if (parse_proc_stat() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    key = R2S(arg1);
    delay = R2N(arg2);

    cpu_user = hash_get_delta(&Stat, "cpu.user", NULL, delay);
    cpu_nice = hash_get_delta(&Stat, "cpu.nice", NULL, delay);
    cpu_system = hash_get_delta(&Stat, "cpu.system", NULL, delay);
    cpu_idle = hash_get_delta(&Stat, "cpu.idle", NULL, delay);

    /* new fields for kernel 2.6 */
    /* even if we dont have this param (ie kernel 2.4) */
    /* the return is 0.0 and not change the results */
    cpu_iow = hash_get_delta(&Stat, "cpu.iow", NULL, delay);
    cpu_irq = hash_get_delta(&Stat, "cpu.irq", NULL, delay);
    cpu_sirq = hash_get_delta(&Stat, "cpu.sirq", NULL, delay);

    cpu_total = cpu_user + cpu_nice + cpu_system + cpu_idle + cpu_iow + cpu_irq + cpu_sirq;

    if (strcasecmp(key, "user") == 0)
	value = cpu_user;
    else if (strcasecmp(key, "nice") == 0)
	value = cpu_nice;
    else if (strcasecmp(key, "system") == 0)
	value = cpu_system;
    else if (strcasecmp(key, "idle") == 0)
	value = cpu_idle;
    else if (strcasecmp(key, "iowait") == 0)
	value = cpu_iow;
    else if (strcasecmp(key, "irq") == 0)
	value = cpu_irq;
    else if (strcasecmp(key, "softirq") == 0)
	value = cpu_sirq;
    else if (strcasecmp(key, "busy") == 0)
	value = cpu_total - cpu_idle;

    if (cpu_total > 0.0)
	value = 100 * value / cpu_total;
    else
	value = 0.0;

    SetResult(&result, R_NUMBER, &value);
}


static void my_disk(RESULT * result, RESULT * arg1, RESULT * arg2, RESULT * arg3)
{
    char *dev, *key, buffer[32];
    int delay;
    double value;

    if (parse_proc_stat() < 0) {
	SetResult(&result, R_STRING, "");
	return;
    }

    dev = R2S(arg1);
    key = R2S(arg2);
    delay = R2N(arg3);

    qprintf(buffer, sizeof(buffer), "disk_io\\.%s\\.%s", dev, key);
    value = hash_get_regex(&Stat, buffer, NULL, delay);

    SetResult(&result, R_NUMBER, &value);
}


int plugin_init_proc_stat(void)
{
    hash_create(&Stat);
    AddFunction("proc_stat", -1, my_proc_stat);
    AddFunction("proc_stat::cpu", 2, my_cpu);
    AddFunction("proc_stat::disk", 3, my_disk);
    return 0;
}

void plugin_exit_proc_stat(void)
{
    if (stream != NULL) {
	fclose(stream);
	stream = NULL;
    }
    hash_destroy(&Stat);
}
