/* $Id: plugin_xmms.c,v 1.9 2004/03/03 03:47:04 reinelt Exp $
 *
 * XMMS-Plugin for LCD4Linux
 * Copyright 2003 Markus Keil <markus_keil@t-online.de>
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
 * $Log: plugin_xmms.c,v $
 * Revision 1.9  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.8  2004/02/05 23:58:18  mkeil
 * Fixed/Optimized Hashage-timings
 *
 * Revision 1.7  2004/01/29 04:40:02  reinelt
 * every .c file includes "config.h" now
 *
 * Revision 1.6  2004/01/21 11:31:23  reinelt
 * two bugs with hash_age() ixed
 *
 * Revision 1.5  2004/01/21 10:48:17  reinelt
 * hash_age function added
 *
 * Revision 1.4  2004/01/16 11:12:26  reinelt
 * some bugs in plugin_xmms fixed, parsing moved to own function
 * plugin_proc_stat nearly finished
 *
 * Revision 1.3  2004/01/16 10:09:49  mkeil
 *   -include caching for values
 *
 * Revision 1.2  2004/01/06 22:33:14  reinelt
 * Copyright statements cleaned up
 *
 * Revision 1.1  2003/12/19 06:27:33  reinelt
 * added XMMS plugin from Markus Keil
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_xmms (void)
 *  adds parser for /tmp/xmms-info
 *
 */


/*
 * The Argument 'arg1' must be one of these Things (without brackets):
 *
 * 'Title' - The title of the current song
 * 'Status' - The status of XMMS (playing, pause, ...)
 * 'Tunes in playlist' - How many entries are in the playlist
 * 'Currently playing' - which playlist-entry is playing
 * 'uSecPosition' - The position of the title in seconds (usefull for bargraphs ;-) )
 * 'Position' - The position of the title in mm:ss
 * 'uSecTime' - The length of the current title in seconds
 * 'Time' - The length of the current title in mm:ss
 * 'Current bitrate' - The current bitrate in bit
 * 'Samping Frequency' - The current samplingfreqency in Hz
 * 'Channels' - The current number of audiochannels
 * 'File' - The full path of the current file
 * 
 * These arguments are case-sensitive
 */


#include "config.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "hash.h"
#include "debug.h"
#include "plugin.h"


static HASH xmms = { 0, };


static int parse_xmms_info (void)
{
  int age;
  FILE *xmms_stream;
  char zeile[200];
  
  // reread every 100msec only
  age=hash_age(&xmms, NULL, NULL);
  if (age>=0 && age<=200) return 0;
  // Open Filestream for '/tmp/xmms-info'
  xmms_stream = fopen("/tmp/xmms-info","r");

  // Check for File
  if( !xmms_stream ) {
    error("Error: Cannot open XMMS-Info Stream! Is XMMS started?");
    return -1;
  }
  
  // Read Lines from the Stream
  while(fgets(zeile,sizeof(zeile),xmms_stream)) {
    char *c, *key, *val;
    c=strchr(zeile, ':');
    if (c==NULL) continue;
    key=zeile; val=c+1;
    // strip leading blanks from key
    while (isspace(*key)) *key++='\0';
    // strip trailing blanks from key
    do *c='\0'; while (isspace(*--c));
    // strip leading blanks from value
    while (isspace(*val)) *val++='\0';
    // strip trailing blanks from value
    for (c=val; *c!='\0';c++);
    while (isspace(*--c)) *c='\0';
    hash_set (&xmms, key, val);
  }
  
  fclose(xmms_stream);
  return 0;
  
}

static void my_xmms (RESULT *result, RESULT *arg1)
{	
  char *key, *val;

  if (parse_xmms_info()<0) {
    SetResult(&result, R_STRING, ""); 
    return;
  }
   
  key=R2S(arg1);
  val=hash_get(&xmms, key);
  if (val==NULL) val="";
  
  SetResult(&result, R_STRING, val);
}


int plugin_init_xmms (void)
{

  // register xmms info
  AddFunction ("xmms", 1, my_xmms);

  return 0;
}

void plugin_exit_xmms(void) 
{
	hash_destroy(&xmms);
}
