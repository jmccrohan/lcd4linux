/* $Id: plugin_xmms.c,v 1.2 2004/01/06 22:33:14 reinelt Exp $
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
 *
 * ATTENTION: This is a convert of my Plugin that i wrote in C++! It might by very buggy!
 *
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

#include "debug.h"
#include "plugin.h"

static void my_xmms (RESULT *result, RESULT *arg1)
{	
  FILE *xmms;
  char zeile[200];
  char *key=R2S(arg1);
  int len=strlen(key);
  
  //Open Filestream for '/tmp/xmms-info'
  xmms = fopen("/tmp/xmms-info","r");

  //Check for File
  if( !xmms ) {
    error("Error: Cannot open XMMS-Info Stream! Is XMMS started?");
    SetResult(&result, R_STRING, "");
    return;
  }
  
  //Read Lines from the Stream
  while(fgets(zeile,200,xmms)) {
    if (strncmp(key, zeile, len)==0 && zeile[len]==':') {
      // remove trailing newline
      zeile[strlen(zeile)-1]='\0';
      SetResult(&result, R_STRING, zeile+len+2);
      return;
    }
  }
  SetResult(&result, R_STRING, "");
}


int plugin_init_xmms (void)
{

  // register xmms info
  AddFunction ("xmms", 1, my_xmms);

  return 0;
}
