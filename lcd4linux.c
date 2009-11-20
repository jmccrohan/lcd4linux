/* $Id$
 * $URL$
 *
 * LCD4Linux
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>		/* umask() */
#include <sys/stat.h>		/* umask() */

#include "svn_version.h"
#include "cfg.h"
#include "debug.h"
#include "qprintf.h"
#include "pid.h"
#include "udelay.h"
#include "drv.h"
#include "timer.h"
#include "layout.h"
#include "plugin.h"
#include "thread.h"
#include "event.h"
#include "widget.h"
#include "widget_timer.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define PIDFILE "/var/run/lcd4linux.pid"

static char *release = "LCD4Linux " VERSION "-" SVN_VERSION;
static char *copyright =
    "Copyright (C) 2005, 2006, 2007, 2008, 2009 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>";
static char **my_argv;
extern char *output;

int got_signal = 0;


static void usage(void)
{
    printf("%s\n", release);
    printf("%s\n", copyright);
    printf("\n");
    printf("usage:\n");
    printf(" lcd4linux [-h]\n");
    printf(" lcd4linux [-l]\n");
    printf(" lcd4linux [-c key=value] [-i] [-f config-file] [-v] [-p pid-file]\n");
    printf(" lcd4linux [-c key=value] [-F] [-f config-file] [-o output-file] [-q] [-v]\n");
    printf("\n");
    printf("options:\n");
    printf("  -h               help\n");
    printf("  -l               list available display drivers and plugins\n");
    printf("  -c <key>=<value> overwrite entries from the config-file\n");
    printf("  -i               enter interactive mode (after display initialisation)\n");
    printf("  -ii              enter interactive mode (before display initialisation)\n");
    printf("  -f <config-file> use configuration from <config-file> instead of /etc/lcd4linux.conf\n");
    printf("  -v               generate info messages\n");
    printf("  -vv              generate debugging messages\n");
    printf("  -p <pid-file>    specify a different pid-file location (default is /var/run/lcd4linux.pid)\n");
    printf("  -F               do not fork and detach (run in foreground)\n");
    printf("  -o <output-file> write picture to file (raster driver only)\n");
    printf("  -q               suppress startup and exit splash screen\n");
#ifdef WITH_X11
    printf("special X11 options:\n");
    printf("  -display <X11 display name>  preceeds X connection given in $DISPLAY\n");
    printf("  -synchronous                 use synchronized communication with X server (for debugging)\n");
    printf("\n");
    printf("\n");
    printf("\n");
#endif
}

static void interactive_mode(void)
{
    char line[1024];
    void *tree;
    RESULT result = { 0, 0, 0, NULL };

    printf("\neval> ");
    for (fgets(line, sizeof(line), stdin); !feof(stdin); fgets(line, sizeof(line), stdin)) {
	if (line[strlen(line) - 1] == '\n')
	    line[strlen(line) - 1] = '\0';
	if (strlen(line) > 0) {
	    if (Compile(line, &tree) != -1) {
		Eval(tree, &result);
		if (result.type == R_NUMBER) {
		    printf("%g\n", R2N(&result));
		} else if (result.type == R_STRING) {
		    printf("'%s'\n", R2S(&result));
		} else if (result.type == (R_NUMBER | R_STRING)) {
		    printf("'%s' (%g)\n", R2S(&result), R2N(&result));
		} else {
		    printf("internal error: unknown result type %d\n", result.type);
		}
		DelResult(&result);
	    }
	    DelTree(tree);
	}
	printf("eval> ");
    }
    printf("\n");
}


void handler(int signal)
{
    debug("got signal %d", signal);
    got_signal = signal;
}


static void daemonize(void)
{

    /* thanks to Petri Damsten, we now follow the guidelines from the UNIX Programming FAQ */
    /* 1.7 How do I get my program to act like a daemon? */
    /* http://www.erlenstar.demon.co.uk/unix/faq_2.html#SEC16 */
    /* especially the double-fork solved the 'lcd4linux dying when called from init' problem */

    pid_t i;
    int fd;


    /* Step 1: fork() so that the parent can exit */
    i = fork();
    if (i < 0) {
	error("fork(#1) failed: %s", strerror(errno));
	exit(1);
    }
    if (i != 0)
	exit(0);

    /* Step 2: setsid() to become a process group and session group leader */
    setsid();

    /* Step 3: fork() again so the parent (the session group leader) can exit */
    i = fork();
    if (i < 0) {
	error("fork(#2) failed: %s", strerror(errno));
	exit(1);
    }
    if (i != 0)
	exit(0);

    /* Step 4: chdir("/") to ensure that our process doesn't keep any directory in use */
    if (chdir("/") != 0) {
	error("chdir(\"/\") failed: %s", strerror(errno));
	exit(1);
    }

    /* Step 5: umask(0) so that we have complete control over the permissions of anything we write */
    umask(0);

    /* Step 6: Establish new open descriptors for stdin, stdout and stderr */
    /* detach stdin */
    if (freopen("/dev/null", "r", stdin) == NULL) {
	error("freopen (/dev/null) failed: %s", strerror(errno));
	exit(1);
    }

    /* detach stdout and stderr */
    fd = open("/dev/null", O_WRONLY, 0666);
    if (fd == -1) {
	error("open (/dev/null) failed: %s", strerror(errno));
	exit(1);
    }
    fflush(stdout);
    dup2(fd, STDOUT_FILENO);
    fflush(stderr);
    dup2(fd, STDERR_FILENO);
    close(fd);

}


int main(int argc, char *argv[])
{
    char *cfg = "/etc/lcd4linux.conf";
    char *pidfile = PIDFILE;
    char *display, *driver, *layout;
    char section[32];
    int c;
    int quiet = 0;
    int interactive = 0;
    int list_mode = 0;
    int pid;

    /* save arguments for restart */
    my_argv = malloc(sizeof(char *) * (argc + 1));
    for (c = 0; c < argc; c++) {
	my_argv[c] = strdup(argv[c]);
    }
    my_argv[c] = NULL;

    /* save original arguments pointer for threads */
    thread_argv = argv;
    thread_argc = argc;

    running_foreground = 0;
    running_background = 0;

#ifdef WITH_X11
    drv_X11_parseArgs(&argc, argv);
    if (argc != thread_argc) {
	/* info() will not work here because verbose level is not known */
	printf("recognized special X11 parameters\n");
    }
#endif
    while ((c = getopt(argc, argv, "c:Ff:hilo:qvp:")) != EOF) {

	switch (c) {
	case 'c':
	    if (cfg_cmd(optarg) < 0) {
		fprintf(stderr, "%s: illegal argument -c '%s'\n", argv[0], optarg);
		exit(2);
	    }
	    break;
	case 'F':
	    running_foreground++;
	    break;
	case 'f':
	    cfg = optarg;
	    break;
	case 'h':
	    usage();
	    exit(0);
	case 'i':
	    interactive++;
	    break;
	case 'l':
	    list_mode++;
	    break;
	case 'o':
	    output = optarg;
	    break;
	case 'q':
	    quiet++;
	    break;
	case 'v':
	    verbose_level++;
	    break;
	case 'p':
	    pidfile = optarg;
	    break;
	default:
	    exit(2);
	}
    }

    if (optind < argc) {
	fprintf(stderr, "%s: illegal option %s\n", argv[0], argv[optind]);
	exit(2);
    }

    /* do not fork in interactive mode */
    if (interactive) {
	running_foreground = 1;
    }

    if (list_mode > 0) {
	printf("%s\n", release);
	printf("%s\n", copyright);
	printf("\n");
	drv_list();
	printf("\n");
	plugin_list();
	printf("\n");
	exit(0);
    }

    info("%s starting", release);
    if (!running_foreground && (my_argv[0] == NULL || my_argv[0][0] != '/')) {
	info("invoked without full path; restart may not work!");
    }

    if (cfg_init(cfg) == -1) {
	error("Error reading configuration. Exit!");
	exit(1);
    }

    if (plugin_init() == -1) {
	error("Error initializing plugins. Exit!");
	exit(1);
    }

    display = cfg_get(NULL, "Display", NULL);
    if (display == NULL || *display == '\0') {
	error("missing 'Display' entry in %s!", cfg_source());
	exit(1);
    }

    qprintf(section, sizeof(section), "Display:%s", display);
    free(display);
    driver = cfg_get(section, "Driver", NULL);
    if (driver == NULL || *driver == '\0') {
	error("missing '%s.Driver' entry in %s!", section, cfg_source());
	exit(1);
    }

    if (!running_foreground) {

	debug("going background...");

	daemonize();

	/* ignore nasty signals */
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	/* create PID file */
	if ((pid = pid_init(pidfile)) != 0) {
	    error("lcd4linux already running as process %d", pid);
	    exit(1);
	}

	/* now we are a daemon */
	running_background = 1;
    }

    /* go into interactive mode before display initialization */
    if (interactive >= 2) {
	interactive_mode();
	pid_exit(pidfile);
	cfg_exit();
	exit(0);
    }

    /* check the conf to see if quiet startup is wanted  */
    if (!quiet) {
	cfg_number(NULL, "Quiet", 0, 0, 1, &quiet);
    }

    debug("initializing driver %s", driver);
    if (drv_init(section, driver, quiet) == -1) {
	error("Error initializing driver %s: Exit!", driver);
	pid_exit(pidfile);
	exit(1);
    }
    free(driver);

    /* register timer widget */
    widget_timer_register();

    /* go into interactive mode (display has been initialized) */
    if (interactive >= 1) {
	interactive_mode();
	drv_quit(quiet);
	pid_exit(pidfile);
	cfg_exit();
	exit(0);
    }

    /* check for new-style layout */
    layout = cfg_get(NULL, "Layout", NULL);
    if (layout == NULL || *layout == '\0') {
	error("missing 'Layout' entry in %s!", cfg_source());
	exit(1);
    }

    layout_init(layout);
    free(layout);

    debug("starting main loop");


    /* now install our own signal handler */
    signal(SIGHUP, handler);
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGTERM, handler);

    while (got_signal == 0) {
	struct timespec delay;
	if (timer_process(&delay) < 0)
	    break;
	event_process(&delay);
    }

    debug("leaving main loop");

    drv_quit(quiet);
    pid_exit(pidfile);
    cfg_exit();
    plugin_exit();
    timer_exit();

    if (got_signal == SIGHUP) {
	long fd;
	debug("restarting...");
	/* close all files on exec */
	for (fd = sysconf(_SC_OPEN_MAX); fd > 2; fd--) {
	    int flag;
	    if ((flag = fcntl(fd, F_GETFD, 0)) != -1)
		fcntl(fd, F_SETFD, flag | FD_CLOEXEC);
	}
	execv(my_argv[0], my_argv);
	error("execv() failed: %s", strerror(errno));
	exit(1);
    }

    for (c = 0; my_argv[c] != NULL; c++) {
	free(my_argv[c]);
    }
    free(my_argv);

    exit(0);
}
