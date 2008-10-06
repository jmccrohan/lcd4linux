/* $Id$
 * $URL$
 *
 * plugin for asterisk
 *
 * Copyright (C) 2003 Michael Reinelt <michael@reinelt.co.at>
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
 * int plugin_init_sample (void)
 *  adds various functions
 *
 */


/* define the include files you need */
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

struct Line {
    char Channel[25];		/* Zap Channel */
    char EndPoint[25];
    unsigned char active;
};

static char *rtrim(char *string, char junk)
{
    char *original = string + strlen(string);
    while (*--original == junk);
    *(original + 1) = '\0';

    return string;
}

static void zapstatus(RESULT * result, RESULT * arg1)
{
    FILE *infile;
    int skipline = 0;		// Skip the first in the file, it throws off the detection
    char line[100], *SipLoc, Channel[25], Location[25], State[9], Application[25], EndPoint[8], Ret[50], inp;
    int i = 0, ChannelInt = 0, ZapLine = 0;
    struct Line Lines[32];	// Setup 32 lines, ZAP 1-32 (memory is cheap)

    ZapLine = R2N(arg1);

    // Set all the lines status's default to inactive
    for (i = 0; i < 32; i++) {
	strcpy(Lines[i].Channel, "ZAP/");
	Lines[i].Channel[4] = (char) (i + 49);
	Lines[i].Channel[5] = '\0';
	Lines[i].active = 0;
    }

    system("touch /tmp/asterisk.state");	// Touch the file in it's naughty place
    system("chmod 744 /tmp/asterisk.state");
    system("asterisk -rx \"show channels\" > /tmp/asterisk.state");	// Crappy CLI way to do it

    infile = fopen("/tmp/asterisk.state", "r");
    while (fgets(line, 100, infile) != NULL) {
	if (strstr(line, "Zap") != NULL) {
	    for (i = 0; i < strlen(line); i++) {
		if (i < 20) {
		    Channel[i] = line[i];
		} else if (i < 42) {
		    Location[i - 21] = line[i];
		} else if (i < 50) {
		    State[i - 42] = line[i];
		} else {
		    Application[i - 50] = line[i];
		}
	    }
	    strncpy(Channel, Channel, 7);
	    Channel[7] = '\0';
	    strcpy(Location, rtrim(Location, ' '));
	    State[4] = '\0';
	    memcpy(EndPoint, Application + 13, 7);
	    EndPoint[7] = '\0';


	    if (strstr(Application, "Bridged Call") != NULL) {
		// Subtract 48 from the character value to get the int
		// value. Subtract one more because arrays start at 0.
		ChannelInt = (int) (Channel[4]) - 49;
		strcpy(Lines[ChannelInt].Channel, Channel);
		strncpy(Lines[ChannelInt].EndPoint, EndPoint, 8);
		Lines[ChannelInt].active = 1;
	    } else {
		SipLoc = strstr(Application, "SIP");
		if (SipLoc != NULL) {
		    strncpy(EndPoint, SipLoc, 7);
		} else {
		    EndPoint[0] = '\0';
		}

		ChannelInt = (int) (Channel[4]) - 49;
		strcpy(Lines[ChannelInt].Channel, Channel);
		strcpy(Lines[ChannelInt].EndPoint, EndPoint);
		Lines[ChannelInt].active = 1;
	    }
	} else {
	    if (strlen(line) > 54 && skipline > 1) {
		for (i = 55; i < 88; i++) {
		    if (i < 40) {
			Channel[i - 55] = line[i];
		    } else {
			EndPoint[i - 40] = line[i];
		    }
		}
		strncpy(Channel, rtrim(Channel, ' '), 5);
		strncpy(EndPoint, rtrim(EndPoint, ' '), 7);

		ChannelInt = (int) (Channel[4]) - 49;
		strcpy(Lines[ChannelInt].Channel, Channel);
		strcpy(Lines[ChannelInt].EndPoint, EndPoint);
		Lines[ChannelInt].active = 1;
	    }
	}
	skipline += 1;
    }
    fclose(infile);

    ZapLine -= 1;
    if (Lines[ZapLine].active == 1) {
	memset(Ret, ' ', 50);
	Ret[0] = '\0';
	strcat(Ret, Lines[ZapLine].Channel);
	strcat(Ret, " -> ");
	strncat(Ret, Lines[ZapLine].EndPoint, 8);
	SetResult(&result, R_STRING, &Ret);
    } else {
	memset(Ret, ' ', 50);
	Ret[0] = '\0';
	strcat(Ret, Lines[ZapLine].Channel);
	strcat(Ret, ": inactive");
	SetResult(&result, R_STRING, &Ret);
    }

    return;
}

int plugin_init_asterisk(void)
{
    AddFunction("asterisk::zapstatus", 1, zapstatus);
    return 0;
}

void plugin_exit_asterisk(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}
