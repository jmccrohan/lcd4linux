/* $Id: lcd4linux.c,v 1.18 2000/04/07 05:42:20 reinelt Exp $
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
 * Revision 1.18  2000/04/07 05:42:20  reinelt
 *
 * UUCP style lockfiles for the serial port
 *
 * Revision 1.17  2000/04/03 17:31:52  reinelt
 *
 * suppress welcome message if display is smaller than 20x2
 * change lcd4linux.ppm to 32 pixel high so KDE won't stretch the icon
 *
 * Revision 1.16  2000/04/03 04:46:38  reinelt
 *
 * added '-c key=val' option
 *
 * Revision 1.15  2000/04/01 22:40:42  herp
 * geometric correction (too many pixelgaps)
 * lcd4linux main should return int, not void
 *
 * Revision 1.14  2000/03/26 18:46:28  reinelt
 *
 * bug in pixmap.c that leaded to empty bars fixed
 * name conflicts with X11 resolved
 *
 * Revision 1.13  2000/03/26 12:55:03  reinelt
 *
 * enhancements to the PPM driver
 *
 * Revision 1.12  2000/03/25 05:50:43  reinelt
 *
 * memory leak in Raster_flush closed
 * driver family logic changed
 *
 * Revision 1.11  2000/03/24 11:36:56  reinelt
 *
 * new syntax for raster configuration
 * changed XRES and YRES to be configurable
 * PPM driver works nice
 *
 * Revision 1.10  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.9  2000/03/22 15:36:21  reinelt
 *
 * added '-l' switch (list drivers)
 * generic pixmap driver added
 * X11 Framework done
 *
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

char *release="LCD4Linux V" VERSION " (c) 2000 Michael Reinelt <reinelt@eunet.at>";
char *output=NULL;
int tick, tack;

static void usage(void)
{
  printf ("%s\n", release);
  printf ("usage: lcd4linux [-h] [-l] [-c key=value] [-f config-file] [-o output-file]\n");
}

static int hello (void)
{
  int x, y;
  
  lcd_query (&y, &x, NULL, NULL, NULL);
  
  if (x>=20) {
    lcd_put (1, 1, "* LCD4Linux V" VERSION " *");
    lcd_put (2, 1, " (c) 2000 M.Reinelt");
  } else if (x >=16) {
    lcd_put (1, 1, "LCD4Linux " VERSION);
    lcd_put (2, 1, "(c) M.Reinelt");
  } else if (x >=9) {
    lcd_put (1, 1, "LCD4Linux");
  } else if (x>=7) {
    lcd_put (1, 1, "L4Linux");
  } else return 0;
  
  lcd_put (1, 1, "* LCD4Linux V" VERSION " *");
  lcd_put (2, 1, " (c) 2000 M.Reinelt");
  lcd_flush();
  return 1;
}

int main (int argc, char *argv[])
{
  char *cfg="/etc/lcd4linux.conf";
  char *driver;
  int c, smooth;
  
  while ((c=getopt (argc, argv, "c:f:hlo:"))!=EOF) {
    switch (c) {
    case 'c':
      if (cfg_cmd (optarg)<0) {
	fprintf (stderr, "%s: illegal argument -c %s\n", argv[0], optarg);
	exit(2);
      }
      break;
    case 'h':
      usage();
      exit(0);
    case 'l':
      printf ("%s\n", release);
      lcd_list();
      exit(0);
    case 'f':
      cfg=optarg;
      break;
    case 'o':
      output=optarg;
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

  if (cfg_read (cfg)==-1)
    exit (1);
  
  driver=cfg_get("display");
  if (driver==NULL || *driver=='\0') {
    fprintf (stderr, "%s: missing 'display' entry!\n", cfg_file());
    exit (1);
  }
  if (lcd_init(driver)==-1) {
    exit (1);
  }

  tick=atoi(cfg_get("tick")?:"100");
  tack=atoi(cfg_get("tack")?:"500");

  process_init();
  lcd_clear();

  if (hello()) {
    sleep (3);
    lcd_clear();
  }
  
  smooth=0;
  while (1) {
    process (smooth);
    smooth+=tick;
    if (smooth>tack) smooth=0;
    usleep(tick*1000);
  }
}
