/* $Id: plugin_xmms.c,v 1.3 2004/01/16 10:09:49 mkeil Exp $
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


#include <string.h>
#include <stdio.h>
#include <time.h>

#include "hash.h"
#include "debug.h"
#include "plugin.h"

static void my_xmms (RESULT *result, RESULT *arg1)
{	
  static HASH xmms = { 0, 0, NULL };
  char *hash_key, *val;
  static time_t now=0;
  FILE *xmms_stream;
  char zeile[200];
  char *key=R2S(arg1);
  int len=strlen(key);
  
  // reread every second only
  if (time(NULL)!=now) { 

  //Open Filestream for '/tmp/xmms-info'
  xmms_stream = fopen("/tmp/xmms-info","r");

  //Check for File
  if( !xmms_stream ) {
    error("Error: Cannot open XMMS-Info Stream! Is XMMS started?");
    SetResult(&result, R_STRING, "");
    return;
  }
  
  //Read Lines from the Stream
  while(fgets(zeile,200,xmms_stream)) {
   hash_key=key; val=zeile; 
   if (strncmp(key, zeile, len)==0 && zeile[len]==':') {
      // remove trailing newline
      zeile[strlen(zeile)-1]='\0';
      // add entry to hash table
      val=zeile+len+2;
      hash_set (&xmms, hash_key, val);
      time(&now);
      fclose(xmms_stream);
    }
   }
  }
  hash_key=R2S(arg1);
  val=hash_get(&xmms, hash_key);
  if (val==NULL) val="";

  SetResult(&result, R_STRING, val);
}


int plugin_init_xmms (void)
{

  // register xmms info
  AddFunction ("xmms", 1, my_xmms);

  return 0;
}
