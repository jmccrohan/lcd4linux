#
# $Id: README.Plugins,v 1.2 2004/06/01 06:04:25 reinelt Exp $
#


This file contains instructions for writing plugins to lcd4linux.

- use the file 'plugin_sample.c' as a template
- copy the file to 'plugin_yourname.c' and edit
- replace the "$Id..." in the first line with "$Id\$" (without backslash)
- add a short description what this plugin is for
- add your copyright notice (important: your name and email)
- replace the "$Log..." with "$Log\$" (without backslash)
- remove all Log lines until "*/"
- do some documentation (I know that real programmers write programs, not documentation)
- use one or more of the example functions as templates for your own functions
- register your new functions to the init() function, delete the sample ones
- edit 'plugin.c', add a prototype and the call to your plugin_init_* function
- edit 'Makefile.am' and add your 'plugin_*.c' to lcd4linux_SOURCES
- compile and test with interactive mode ('-i')
- send me a patch (or check in if you have developer CVS access)
- enjoy

