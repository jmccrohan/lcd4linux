/* $Id: lcd4linux.c,v 1.8 2000/03/22 07:33:50 reinelt Exp $
 *
 * LCD4Linux
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: lcd4linux.c,v $
 * Revision 1.8  2000/03/22 07:33:50  reinelt
 *
 * FAQ added
 * new modules 'processor.c' contains all data processing
 *
 * Revision 1.7  2000/03/19 08:41:28  reinelt
 *
 * documentation available! README, README.MatrixOrbital, README.Drivers
 * added Skeleton.c as a starting point for new drivers
 *
 * Revision 1.6  2000/03/18 10:31:06  reinelt
 *
 * added sensor handling (for temperature etc.)
 * made data collecting happen only if data is used
 * (reading /proc/meminfo takes a lot of CPU!)
 * released lcd4linux-0.92
 *
 * Revision 1.5  2000/03/18 08:07:04  reinelt
 *
 * vertical bars implemented
 * bar compaction improved
 * memory information implemented
 *
 * Revision 1.4  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.3  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.2  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "cfg.h"
#include "display.h"
#include "processor.h"

int tick, tack;

static void usage(void)
{
  printf ("LCD4Linux V" VERSION " (c) 2000 Michael Reinelt <reinelt@eunet.at>\n");
  printf ("usage: lcd4linux [-h] [-l] [-f config-file]\n");
}

void main (int argc, char *argv[])
{
  char *cfg="/etc/lcd4linux.conf";
  char *display;
  int c, smooth;

  while ((c=getopt (argc, argv, "hlf:"))!=EOF) {
    switch (c) {
    case 'h':
      usage();
      exit(0);
    case 'l':
      usage();
      exit(0);
    case 'f':
      cfg=optarg;
      break;
    default:
      exit(2);
    }
  }

  if (optind < argc) {
    fprintf (stderr, "%s: illegal option %s\n", argv[0], argv[optind]);
    exit(2);
  }

  // set default values
 
  cfg_set ("row1", "*** %o %v ***");
  cfg_set ("row2", "%p CPU  %r MB RAM");
  cfg_set ("row3", "Busy %cu%% $r10cu");
  cfg_set ("row4", "Load %l1%L$r10l1");

  cfg_set ("tick", "100");
  cfg_set ("tack", "500");
  cfg_set ("tau", "500");
  
  cfg_set ("fifo", "/var/run/LCD4Linux");
  cfg_set ("overload", "2.0");

  if (cfg_read (cfg)==-1)
    exit (1);
  
  display=cfg_get("display");
  if (display==NULL || *display=='\0') {
    fprintf (stderr, "%s: missing 'display' entry!\n", cfg_file());
    exit (1);
  }
  if (lcd_init(display)==-1) {
    exit (1);
  }

  tick=atoi(cfg_get("tick"));
  tack=atoi(cfg_get("tack"));

  process_init();

  lcd_clear();
  lcd_put (1, 1, "** LCD4Linux V" VERSION " **");
  lcd_put (2, 1, " (c) 2000 M.Reinelt");
  lcd_flush();
  
  sleep (3);
  lcd_clear();

  smooth=0;
  while (1) {
    process (smooth);
    smooth+=tick;
    if (smooth>tack) smooth=0;
    usleep(tick*1000);
  }
}
