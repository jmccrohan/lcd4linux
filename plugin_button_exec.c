/* $Id: plugin_sample.c 840 2007-09-09 12:17:42Z michael $
 * $URL: https://ssl.bulix.org/svn/lcd4linux/trunk/plugin_sample.c $
 *
 * plugin that forks and exec's once a key is pressed. the difference to the exec plugin is: this can also only be done once!
 *
 * Copyright (C) 2008, Wolfgang Henerbichler <wogri@wogri.com>
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
 * int plugin_init_button_exec(void)
 *  adds various functions
 *
 */


/* define the include files you need */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif



/* sample function 'button_exec' */
/* takes as many arguments as you want, first arguemnt is the command that will be called via exec, all the following are parameters you want to add to your command - you won't know what has happened after forking - no return codes available */
/* Note: all local functions should be declared 'static' */

static void my_button_exec(RESULT * result, int argc, RESULT * argv[])
{
    int pid;
    int i;
    int errsv;
    char *args[argc + 1];
    char *arg;
    char *prog;
    char *env[1];

    signal(SIGCHLD, SIG_IGN);
    prog = R2S(argv[0]);
    info(prog);
    for (i = 1; i < argc; i++) {
	arg = R2S(argv[i]);
	args[i] = arg;
	info(arg);
    }
    args[i] = (char *) 0;
    env[0] = (char *) 0;
    pid = fork();
    if (pid == 0) {		// child-process
	// char *args[] = {"-r", "-t", "-l", (char *) 0 };
	info("executing program");
	execve(prog, args, env);
	errsv = errno;
	info("executing program failed");
	info(strerror(errsv));
	exit(0);
    } else if (pid == -1) {
	info("weird error has occured. couldn't fork.");
    } else {
	SetResult(&result, R_STRING, "0");
    }
}


/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_button_exec(void)
{

    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("button_exec", -1, my_button_exec);
    return 0;
}

void plugin_exit_button_exec(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}
