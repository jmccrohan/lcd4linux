/* $Id: lcd4linux.c,v 1.69 2004/03/14 07:11:42 reinelt Exp $
 *
 * LCD4Linux
 *
 * Copyright 1999-2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: lcd4linux.c,v $
 * Revision 1.69  2004/03/14 07:11:42  reinelt
 * parameter count fixed for plugin_dvb()
 * plugin_APM (battery status) ported
 *
 * Revision 1.68  2004/03/11 06:39:59  reinelt
 * big patch from Martin:
 * - reuse filehandles
 * - memory leaks fixed
 * - earlier busy-flag checking with HD44780
 * - reuse memory for strings in RESULT and hash
 * - netdev_fast to wavid time-consuming regex
 *
 * Revision 1.67  2004/03/06 20:31:16  reinelt
 * Complete rewrite of the evaluator to get rid of the code
 * from mark Morley (because of license issues).
 * The new Evaluator does a pre-compile of expressions, and
 * stores them in trees. Therefore it should be reasonable faster...
 *
 * Revision 1.66  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.65  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.64  2004/02/27 07:06:25  reinelt
 * new function 'qprintf()' (simple but quick snprintf() replacement)
 *
 * Revision 1.63  2004/02/10 07:42:35  reinelt
 * cut off all old-style files which are no longer used with NextGeneration
 *
 * Revision 1.62  2004/01/30 20:57:56  reinelt
 * HD44780 patch from Martin Hejl
 * dmalloc integrated
 *
 * Revision 1.61  2004/01/14 11:33:00  reinelt
 * new plugin 'uname' which does what it's called
 * text widget nearly finished
 * first results displayed on MatrixOrbital
 *
 * Revision 1.60  2004/01/13 08:18:19  reinelt
 * timer queues added
 * liblcd4linux deactivated turing transformation to new layout
 *
 * Revision 1.59  2004/01/12 03:51:01  reinelt
 * evaluating the 'Variables' section in the config file
 *
 * Revision 1.58  2004/01/10 20:22:33  reinelt
 * added new function 'cfg_list()' (not finished yet)
 * added layout.c (will replace processor.c someday)
 * added widget_text.c (will be the first and most important widget)
 * modified lcd4linux.c so that old-style configs should work, too
 *
 * Revision 1.57  2004/01/10 17:45:26  reinelt
 * changed initialization order so cfg() gets initialized before plugins.
 * This way a plugin's init() can use cfg_get().
 * Thanks to Xavier for reporting this one!
 *
 * Revision 1.56  2004/01/09 17:03:07  reinelt
 * initiated transfer to new driver architecture
 * new file 'drv.c' will someday replace 'display.c'
 * new file 'drv_MatrixOrbital.c' will replace 'MatrixOrbital.c'
 * due to this 'soft' transfer lcd4linux should stay usable during the switch
 * (at least I hope so)
 *
 * Revision 1.55  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.54  2004/01/08 05:28:12  reinelt
 * Luk Claes added to AUTHORS
 * cfg: group handling ('{}') added
 *
 * Revision 1.53  2003/12/19 05:35:14  reinelt
 * renamed 'client' to 'plugin'
 *
 * Revision 1.52  2003/12/01 07:08:50  reinelt
 *
 * Patches from Xavier:
 *  - WiFi: make interface configurable
 *  - "quiet" as an option from the config file
 *  - ignore missing "MemShared" on Linux 2.6
 *
 * Revision 1.51  2003/11/16 09:45:49  reinelt
 * Crystalfontz changes, small glitch in getopt() fixed
 *
 * Revision 1.50  2003/10/22 04:19:16  reinelt
 * Makefile.in for imon.c/.h, some MatrixOrbital clients
 *
 * Revision 1.49  2003/10/11 06:01:53  reinelt
 *
 * renamed expression.{c,h} to client.{c,h}
 * added config file client
 * new functions 'AddNumericVariable()' and 'AddStringVariable()'
 * new parameter '-i' for interactive mode
 *
 * Revision 1.48  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.47  2003/09/10 08:37:09  reinelt
 * icons: reorganized tick_* again...
 *
 * Revision 1.46  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.45  2003/09/09 05:30:34  reinelt
 * even more icons stuff
 *
 * Revision 1.44  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.43  2003/08/17 16:37:39  reinelt
 * more icon framework
 *
 * Revision 1.42  2003/08/14 03:47:40  reinelt
 * remove PID file if driver initialisation fails
 *
 * Revision 1.41  2003/08/08 08:05:23  reinelt
 * added PID file handling
 *
 * Revision 1.40  2003/08/08 06:58:06  reinelt
 * improved forking
 *
 * Revision 1.39  2003/07/24 04:48:09  reinelt
 * 'soft clear' needed for virtual rows
 *
 * Revision 1.38  2003/06/13 05:11:11  reinelt
 * error message cosmetics
 *
 * Revision 1.37  2003/04/07 06:03:01  reinelt
 * further parallel port abstraction
 *
 * Revision 1.36  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.35  2003/02/13 10:40:17  reinelt
 *
 * changed "copyright" to "2003"
 * added slightly different protocol for MatrixOrbital "LK202" displays
 *
 * Revision 1.34  2002/04/29 11:00:28  reinelt
 *
 * added Toshiba T6963 driver
 * added ndelay() with nanosecond resolution
 *
 * Revision 1.33  2001/04/27 05:04:57  reinelt
 *
 * replaced OPEN_MAX with sysconf()
 * replaced mktemp() with mkstemp()
 * unlock serial port if open() fails
 *
 * Revision 1.32  2001/03/13 07:41:22  reinelt
 *
 * added NEWS file
 *
 * Revision 1.31  2001/03/12 12:39:36  reinelt
 *
 * reworked autoconf a lot: drivers may be excluded, #define's went to config.h
 *
 * Revision 1.30  2001/03/08 15:25:38  ltoetsch
 * improved exec
 *
 * Revision 1.29  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.28  2000/10/25 08:10:48  reinelt
 *
 * added restart funnctionality
 * (lots of this code was stolen from sendmail.c)
 *
 * Revision 1.27  2000/08/10 18:42:20  reinelt
 *
 * fixed some bugs with the new syslog code
 *
 * Revision 1.26  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.25  2000/08/09 14:14:11  reinelt
 *
 * new switch -F (do not fork)
 * added automatic forking if -F not specified
 *
 * Revision 1.24  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.23  2000/04/17 05:14:27  reinelt
 *
 * added README.44780
 *
 * Revision 1.22  2000/04/15 16:56:52  reinelt
 *
 * moved delay loops to udelay.c
 * renamed -d (debugging) switch to -v (verbose)
 * new switch -d to calibrate delay loop
 * 'Delay' entry for HD44780 back again
 * delay loops will not calibrate automatically, because this will fail with hich CPU load
 *
 * Revision 1.21  2000/04/15 11:56:35  reinelt
 *
 * more debug messages
 *
 * Revision 1.20  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 * Revision 1.19  2000/04/10 04:40:53  reinelt
 *
 * minor changes and cleanups
 *
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "cfg.h"
#include "debug.h"
#include "qprintf.h"
#include "pid.h"
#include "udelay.h"
#include "drv.h"
#include "timer.h"
#include "layout.h"
#include "plugin.h"


#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define PIDFILE "/var/run/lcd4linux.pid"

static char *release="LCD4Linux " VERSION " (c) 2003 Michael Reinelt <reinelt@eunet.at>";
static char **my_argv;
static int got_signal=0;

int tick, tack;
extern char* output;


static void usage(void)
{
  printf ("%s\n", release);
  printf ("usage: lcd4linux [-h]\n");
  printf ("       lcd4linux [-l]\n");
  printf ("       lcd4linux [-c key=value] [-i] [-f config-file] [-v]\n");
  printf ("       lcd4linux [-c key=value] [-F] [-f config-file] [-o output-file] [-q] [-v]\n");
}

#if 0
// Fixme: how to hello() with new layout?
int hello (void)
{
  int i, x, y, flag;
  char *line1[] = { "* LCD4Linux " VERSION " *",
		    "LCD4Linux " VERSION,
		    "LCD4Linux",
		    "L4Linux",
		    NULL };
  
  char *line2[] = { "(c) 2003 M.Reinelt",
		    "(c) M.Reinelt",
		    NULL };
  
  lcd_query (&y, &x, NULL, NULL, NULL, NULL, NULL);
  
  flag=0;
  for (i=0; line1[i]; i++) {
    if (strlen(line1[i])<=x) {
      lcd_put (1, (x-strlen(line1[i]))/2+1, line1[i]);
      flag=1;
      break;
    }
  }
  
  for (i=0; line2[i]; i++) {
    if (strlen(line2[i])<=x) {
      lcd_put (2, (x-strlen(line2[i]))/2+1, line2[i]);
      flag=1;
      break;
    }
  }
  
  if (flag) lcd_flush();
  return flag;
}
#endif


static void interactive_mode (void)
{
  char line[1024];
  void *tree;
  RESULT result = {0, 0, 0, NULL};
    
  printf("\neval> ");
  for(fgets(line, 1024, stdin); !feof(stdin); fgets(line, 1024, stdin)) {
    if (line[strlen(line)-1]=='\n') line[strlen(line)-1]='\0';
    if (strlen(line)>0) {
      if (Compile(line, &tree)!=-1) {
	Eval (tree, &result);
	if (result.type==R_NUMBER) {
	  printf ("%g\n", R2N(&result));
	} else if (result.type==R_STRING) {
	  printf ("'%s'\n", R2S(&result));
	}
	DelResult (&result);
      }
      DelTree(tree);
    }
    printf("eval> ");
  }
  printf ("\n");
}


void handler (int signal)
{
  debug ("got signal %d", signal);
  got_signal=signal;
}


int main (int argc, char *argv[])
{
  char *cfg="/etc/lcd4linux.conf";
  char *display, *driver, *layout;
  char  section[32];
  int c;
  int quiet=0;
  int interactive=0;
  

  // save arguments for restart
  my_argv=malloc(sizeof(char*)*(argc+1));
  for (c=0; c<argc; c++) {
    my_argv[c]=strdup(argv[c]);
  }
  my_argv[c]=NULL;

  running_foreground=0;
  running_background=0;
  
  while ((c=getopt (argc, argv, "c:Ff:hilo:qv"))!=EOF) {

    switch (c) {
    case 'c':
      if (cfg_cmd (optarg)<0) {
	fprintf (stderr, "%s: illegal argument -c '%s'\n", argv[0], optarg);
	exit(2);
      }
      break;
    case 'F':
      running_foreground++;
      break;
    case 'f':
      cfg=optarg;
      break;
    case 'h':
      usage();
      exit(0);
    case 'i':
      interactive++;
      break;
    case 'l':
      printf ("%s\n", release);
      drv_list();
      exit(0);
    case 'o':
      output=optarg;
      break;
    case 'q':
      quiet++;
      break;
    case 'v':
      verbose_level++;
      break;
    default:
      exit(2);
    }
  }

  if (optind < argc) {
    fprintf (stderr, "%s: illegal option %s\n", argv[0], argv[optind]);
    exit(2);
  }

  // do not fork in interactive mode
  if (interactive) {
    running_foreground=1;
  }

  info ("Version " VERSION " starting");
  if (!running_foreground && (my_argv[0]==NULL || my_argv[0][0]!='/')) {
    info ("invoked without full path; restart may not work!");
  }

  if (cfg_init(cfg)==-1)
    exit (1);
  
  if (plugin_init()==-1)
    exit (1);
  
  display=cfg_get(NULL, "Display", NULL);
  if (display==NULL || *display=='\0') {
    error ("missing 'Display' entry in %s!", cfg_source());
    exit (1);
  }
  
  qprintf(section, sizeof(section), "Display:%s", display);
  free(display);
  driver=cfg_get(section, "Driver", NULL);
  if (driver==NULL || *driver=='\0') {
    error ("missing '%s.Driver' entry in %s!", section, cfg_source());
    exit (1);
  }
  
  if (!running_foreground) {
    pid_t i;
    int fd;
    debug ("going background...");
    i=fork();
    if (i<0) {
      error ("fork() failed: %s", strerror(errno));
      exit (1);
    }
    if (i!=0) exit (0);
    
    // ignore nasty signals
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    // chdir("/")
    if (chdir("/")!=0) {
      error ("chdir(\"/\") failed: %s", strerror(errno));
      exit (1);
    }
    
    // we want full control over permissions
    umask (0);
    
    // detach stdin
    if (freopen("/dev/null", "r", stdin)==NULL) {
      error ("freopen (/dev/null) failed: %s", strerror(errno));
      exit (1);
    }

    // detach stdout and stderr
    fd=open("/dev/null", O_WRONLY, 0666);
    if (fd==-1) {
      error ("open (/dev/null) failed: %s", strerror(errno));
      exit (1);
    }
    fflush(stdout);
    fflush(stderr);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // create PID file
    if (pid_init(PIDFILE)!=0) {
      error ("PID file creation failed!");
      exit (1);
    }

    // now we are a daemon
    running_background=1;
  }
  
  // go into interactive mode before display initialization
  if (interactive >= 2) {
    interactive_mode();
    pid_exit(PIDFILE);
    cfg_exit();
    exit (0);
  }
  
  debug ("initializing driver %s", driver);
  if (drv_init(section, driver)==-1) {
    pid_exit(PIDFILE);
    exit (1);
  }
  free(driver);
  
  // check for new-style layout
  layout=cfg_get(NULL, "Layout", NULL);
  if (layout==NULL || *layout=='\0') {
    error ("missing 'Layout' entry in %s!", cfg_source());
    exit (1);
  }
  
  layout_init(layout);
  free(layout);

  // go into interactive mode (display has been initialized)
  if (interactive >= 1) {
    interactive_mode();
    drv_quit();
    pid_exit(PIDFILE);
    cfg_exit();
    exit (0);
  }

  // check the conf to see if quiet startup is wanted 
  if (!quiet) {
    cfg_number(NULL, "Quiet", 0, 0, 1, &quiet);
  }
  
#if 0
  // Fixme: how to hello() with new layout?
  if (!quiet && hello()) {
    sleep (3);
    lcd_clear(1);
  }
#endif

  debug ("starting main loop");
  
  // now install our own signal handler
  signal(SIGHUP,  handler);
  signal(SIGINT,  handler);
  signal(SIGQUIT, handler);
  signal(SIGTERM, handler);
  
  while (got_signal==0) {
    struct timespec delay;
    if (timer_process(&delay)<0) break;
    nanosleep(&delay, NULL);
  }
  
  debug ("leaving main loop");
  
#if 0
  // Fixme: how to hello() with new layout?
  lcd_clear(1);
  if (!quiet) hello();
#endif
  drv_quit();
  pid_exit(PIDFILE);
  cfg_exit();
  plugin_exit();    
  timer_exit();
  if (got_signal==SIGHUP) {
    long fd;
    debug ("restarting...");
    // close all files on exec
    for (fd=sysconf(_SC_OPEN_MAX); fd>2; fd--) {
      int flag;
      if ((flag=fcntl(fd,F_GETFD,0))!=-1)
	fcntl(fd,F_SETFD,flag|FD_CLOEXEC);
    }
    execv (my_argv[0], my_argv);
    error ("execv() failed: %s", strerror(errno));
    exit(1);
  }
  
  for(c=0;my_argv[c]!=NULL;c++) {
    free(my_argv[c]);
  }
  free(my_argv);
  exit (0);
}
  
