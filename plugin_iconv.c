/* $Id$
 * $URL$
 *
 * iconv charset conversion plugin
 *
 * Copyright (C) 2006 Ernst Bachmann <e.bachmann@xebec.de>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * int plugin_init_iconv (void)
 * int plugin_exit_iconv (void)
 *
 */


#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iconv.h>
#include <errno.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif



/* iconv function, convert charsets */
/* valid "to" and "from" charsets can be listed by running "iconv --list" from a shell */
/* utf16 & utf32 encodings won't work, as they contain null bytes, confusing strlen */
static void my_iconv(RESULT * result, RESULT * charset_from, RESULT * charset_to, RESULT * arg)
{
    char *source;
    size_t source_left;
    char *dest;
    char *dest_pos;
    size_t dest_left;
    iconv_t cd;

    source = R2S(arg);
    source_left = strlen(source);

    /* use twice the memory needed in best case, but save lots of reallocs in worst case */
    /* increase to 4 if most conversions are to utf32 (quite unlikely) */
    /* also alloc a "safety byte" so we can always zero-terminate the string. */

    dest_left = 2 * source_left;
    dest = malloc(dest_left + 1);
    dest_pos = dest;

    cd = iconv_open(R2S(charset_to), R2S(charset_from));
    if (cd != (iconv_t) (-1)) {

	do {

	    /* quite spammy: debug("plugin_iconv: calling iconv with %ld,[%s]/%ld,%ld", cd, source, source_left, dest_left); */
	    if (iconv(cd, &source, &source_left, &dest_pos, &dest_left) == (size_t) (-1)) {
		switch (errno) {
		case EILSEQ:
		    /* illegal bytes in input sequence */
		    /* try to fix by skipping a byte */
		    info("plugin_iconv: illegal character in input string: %c", *source);
		    source_left--;
		    source++;
		    break;
		case EINVAL:
		    /* input string ends during a multibyte sequence */
		    /* try to fix by simply ignoring */
		    info("plugin_iconv: illegal character at end of input");
		    source_left = 0;
		    break;
		case E2BIG:
		    /* not enough bytes in outbuf. */
		    /* TODO: Realloc output buffer, probably doubling its size? */
		    /* for now, just bail out. For lcd4linux 99% of all conversions will go to ascii or latin1 anyways */
		    error
			("plugin_iconv: out of memory in destination buffer. Seems like Ernst was too lazy, complain to him!");
		    source_left = 0;
		    break;
		default:
		    error("plugin_iconv: strange errno state (%d) occured", errno);
		    source_left = 0;
		}
	    }
	} while (source_left > 0);	/* don't check for == 0, could be negative in EILSEQ case */

	/* terminate the string, we're sure to have that byte left, see above */
	*dest_pos = 0;
	dest_pos++;

	iconv_close(cd);
    } else {
	error("plugin_iconv: could not open conversion descriptor. Check if your charsets are supported!");
	/* guaranteed to fit. */
	strcpy(dest, source);
    }

    SetResult(&result, R_STRING, dest);

    free(dest);
}


/* plugin initialization */
int plugin_init_iconv(void)
{

    AddFunction("iconv", 3, my_iconv);

    return 0;
}

void plugin_exit_iconv(void)
{
    /* nothing to clean */
}
