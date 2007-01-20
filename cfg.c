/* $Id$
 * $URL$
 * $URL$
 *
 * config file stuff
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * cfg_init (source)
 *   read configuration from source
 *   returns  0 if successful
 *   returns -1 in case of an error
 * 
 * cfg_source (void)
 *   returns the file the configuration was read from
 * 
 * cfg_cmd (arg)
 *   allows us to overwrite entries in the 
 *   config-file from the command line.
 *   arg is 'key=value'
 *   cfg_cmd can be called _before_ cfg_read()
 *   returns 0 if ok, -1 if arg cannot be parsed
 *
 * cfg_list (section)
 *   returns a list of all keys in the specified section
 *   This list was allocated be cfg_list() and must be 
 *   freed by the caller!
 *
 * cfg_get_raw (section, key, defval) 
 *   return the a value for a given key in a given section 
 *   or <defval> if key does not exist. Does NOT evaluate
 *   the expression. Therefore used to get the expression 
 *   itself!
 *
 * cfg_get (section, key, defval) 
 *   return the a value for a given key in a given section 
 *   or <defval> if key does not exist. The specified
 *   value in the config is treated as a expression and 
 *   is evaluated!
 *
 * cfg_number (section, key, defval, min, int max, *value) 
 *   return the a value for a given key in a given section 
 *   convert it into a number with syntax checking
 *   check if its in a given range. As it uses cfg_get()
 *   internally, the evaluator is used here, too.
 * 
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <unistd.h>
#include <sys/stat.h>

#include "debug.h"
#include "evaluator.h"
#include "cfg.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

typedef struct {
    char *key;
    char *val;
    int lock;
} ENTRY;


static char *Config_File = NULL;
static ENTRY *Config = NULL;
static int nConfig = 0;


/* bsearch compare function for config entries */
static int c_lookup(const void *a, const void *b)
{
    char *key = (char *) a;
    ENTRY *entry = (ENTRY *) b;

    return strcasecmp(key, entry->key);
}


/* qsort compare function for variables */
static int c_sort(const void *a, const void *b)
{
    ENTRY *ea = (ENTRY *) a;
    ENTRY *eb = (ENTRY *) b;

    return strcasecmp(ea->key, eb->key);
}


/* remove leading and trailing whitespace */
static char *strip(char *s, const int strip_comments)
{
    char *p;

    while (isblank(*s))
	s++;

    for (p = s; *p; p++) {
	if (*p == '"')
	    do
		p++;
	    while (*p && *p != '\n' && *p != '"');
	if (*p == '\'')
	    do
		p++;
	    while (*p && *p != '\n' && *p != '\'');
	if (*p == '\n' || (strip_comments && *p == '#' && (p == s || *(p - 1) != '\\'))) {
	    *p = '\0';
	    break;
	}
    }

    for (p--; p > s && isblank(*p); p--)
	*p = '\0';

    return s;
}


/* unquote a string */
static char *dequote(char *string)
{
    int quote = 0;
    char *s = string;
    char *p = string;

    do {
	if (*s == '\'') {
	    quote = !quote;
	    *p++ = *s;
	} else if (quote && *s == '\\') {
	    s++;
	    if (*s >= '0' && *s <= '7') {
		int n;
		unsigned int c = 0;
		sscanf(s, "%3o%n", &c, &n);
		if (c == 0 || c > 255) {
		    error("WARNING: illegal '\\' in <%s>", string);
		} else {
		    *p++ = c;
		    s += n - 1;
		}
	    } else {
		*p++ = *s;
	    }
	} else {
	    *p++ = *s;
	}
    } while (*s++);

    return string;
}


/* which if a string contains only valid chars */
/* i.e. start with a char and contains chars and nums */
static int validchars(const char *string, const int numstart)
{
    const char *c;

    for (c = string; *c; c++) {
	/* first and following chars */
	if ((*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (*c == '_'))
	    continue;
	/* number as first or following char */
	if ((numstart || c > string) && *c >= '0' && *c <= '9')
	    continue;
	/* only following chars */
	if ((c > string) && ((*c == '.') || (*c == '-')))
	    continue;
	return 0;
    }
    return 1;
}


static void cfg_add(const char *section, const char *key, const char *val, const int lock)
{
    char *buffer;
    ENTRY *entry;

    /* allocate buffer  */
    buffer = malloc(strlen(section) + strlen(key) + 2);
    *buffer = '\0';

    /* prepare section.key */
    if (section != NULL && *section != '\0') {
	strcpy(buffer, section);
	strcat(buffer, ".");
    }
    strcat(buffer, key);

    /* does the key already exist? */
    entry = bsearch(buffer, Config, nConfig, sizeof(ENTRY), c_lookup);

    if (entry != NULL) {
	if (entry->lock > lock)
	    return;
	debug("Warning: key <%s>: value <%s> overwritten with <%s>", buffer, entry->val, val);
	free(buffer);
	if (entry->val)
	    free(entry->val);
	entry->val = dequote(strdup(val));
	return;
    }

    nConfig++;
    Config = realloc(Config, nConfig * sizeof(ENTRY));
    Config[nConfig - 1].key = buffer;
    Config[nConfig - 1].val = dequote(strdup(val));
    Config[nConfig - 1].lock = lock;

    qsort(Config, nConfig, sizeof(ENTRY), c_sort);

}


int cfg_cmd(const char *arg)
{
    char *key, *val;
    char *buffer;

    buffer = strdup(arg);
    key = strip(buffer, 0);
    for (val = key; *val; val++) {
	if (*val == '=') {
	    *val++ = '\0';
	    break;
	}
    }
    if (*key == '\0' || *val == '\0') {
	free(buffer);
	return -1;
    }

    if (!validchars(key, 0)) {
	free(buffer);
	return -1;
    }

    cfg_add("", key, val, 1);

    free(buffer);
    return 0;
}


char *cfg_list(const char *section)
{
    int i, len;
    char *key, *list;

    /* calculate key length */
    len = strlen(section) + 1;

    /* prepare search key */
    key = malloc(len + 1);
    strcpy(key, section);
    strcat(key, ".");

    /* start with empty string */
    list = malloc(1);
    *list = '\0';

    /* search matching entries */
    for (i = 0; i < nConfig; i++) {
	if (strncasecmp(Config[i].key, key, len) == 0) {
	    list = realloc(list, strlen(list) + strlen(Config[i].key) - len + 2);
	    if (*list != '\0')
		strcat(list, "|");
	    strcat(list, Config[i].key + len);
	}
    }

    free(key);
    return list;
}


static char *cfg_lookup(const char *section, const char *key)
{
    int len;
    char *buffer;
    ENTRY *entry;

    /* calculate key length */
    len = strlen(key) + 1;
    if (section != NULL)
	len += strlen(section) + 1;

    /* allocate buffer  */
    buffer = malloc(len);
    *buffer = '\0';

    /* prepare section:key */
    if (section != NULL && *section != '\0') {
	strcpy(buffer, section);
	strcat(buffer, ".");
    }
    strcat(buffer, key);

    /* search entry */
    entry = bsearch(buffer, Config, nConfig, sizeof(ENTRY), c_lookup);

    /* free buffer again */
    free(buffer);

    if (entry != NULL)
	return entry->val;

    return NULL;
}


char *cfg_get_raw(const char *section, const char *key, const char *defval)
{
    char *val = cfg_lookup(section, key);

    if (val != NULL)
	return val;

    return (char *) defval;
}


char *cfg_get(const char *section, const char *key, const char *defval)
{
    char *expression;
    char *retval;
    void *tree = NULL;
    RESULT result = { 0, 0, 0, NULL };

    expression = cfg_lookup(section, key);

    if (expression != NULL) {
	if (*expression == '\0')
	    return "";
	if (Compile(expression, &tree) == 0 && Eval(tree, &result) == 0) {
	    retval = strdup(R2S(&result));
	    DelTree(tree);
	    DelResult(&result);
	    return (retval);
	}
	DelTree(tree);
	DelResult(&result);
    }
    if (defval)
	return strdup(defval);
    return NULL;
}


int cfg_number(const char *section, const char *key, const int defval, const int min, const int max, int *value)
{
    char *expression;
    void *tree = NULL;
    RESULT result = { 0, 0, 0, NULL };

    /* start with default value */
    /* in case of an (uncatched) error, you have the */
    /* default value set, which may be handy... */
    *value = defval;

    expression = cfg_get_raw(section, key, NULL);
    if (expression == NULL || *expression == '\0') {
	return 0;
    }

    if (Compile(expression, &tree) != 0) {
	DelTree(tree);
	return -1;
    }
    if (Eval(tree, &result) != 0) {
	DelTree(tree);
	DelResult(&result);
	return -1;
    }
    *value = R2N(&result);
    DelTree(tree);
    DelResult(&result);

    if (*value < min) {
	error("bad '%s.%s' value '%d' in %s, minimum is %d", section, key, *value, cfg_source(), min);
	*value = min;
	return -1;
    }

    if (max > min && max != -1 && *value > max) {
	error("bad '%s.%s' value '%d' in %s, maximum is %d", section, key, *value, cfg_source(), max);
	*value = max;
	return -1;
    }

    return 1;
}


static int cfg_check_source(const char *file)
{
    /* as passwords and commands are stored in the config file,
     * we will check that:
     * - file is a normal file (or /dev/null)
     * - file owner is owner of program
     * - file is not accessible by group
     * - file is not accessible by other
     */

    struct stat stbuf;
    uid_t uid, gid;
    int error;

    uid = geteuid();
    gid = getegid();

    if (stat(file, &stbuf) == -1) {
	error("stat(%s) failed: %s", file, strerror(errno));
	return -1;
    }
    if (S_ISCHR(stbuf.st_mode) && strcmp(file, "/dev/null") == 0)
	return 0;

    error = 0;
    if (!S_ISREG(stbuf.st_mode)) {
	error("security error: '%s' is not a regular file", file);
	error = -1;
    }
    if (stbuf.st_uid != uid || stbuf.st_gid != gid) {
	error("security error: owner and/or group of '%s' don't match", file);
	error = -1;
    }
    if (stbuf.st_mode & S_IRWXG || stbuf.st_mode & S_IRWXO) {
	error("security error: group or other have access to '%s'", file);
	error = -1;
    }
    return error;
}


static int cfg_read(const char *file)
{
    FILE *stream;
    char buffer[256];
    char section[256];
    char *line, *key, *val, *end;
    int section_open, section_close;
    int error, lineno;

    stream = fopen(file, "r");
    if (stream == NULL) {
	error("open(%s) failed: %s", file, strerror(errno));
	return -1;
    }

    /* start with empty section */
    strcpy(section, "");

    error = 0;
    lineno = 0;
    while ((line = fgets(buffer, 256, stream)) != NULL) {

	/* increment line number */
	lineno++;

	/* skip empty lines */
	if (*(line = strip(line, 1)) == '\0')
	    continue;

	/* reset section flags */
	section_open = 0;
	section_close = 0;

	/* key is first word */
	key = line;

	/* search first blank between key and value */
	for (val = line; *val; val++) {
	    if (isblank(*val)) {
		*val++ = '\0';
		break;
	    }
	}

	/* strip value */
	val = strip(val, 1);

	/* search end of value */
	if (*val)
	    for (end = val; *(end + 1); end++);
	else
	    end = val;

	/* if last char is '{', a section has been opened */
	if (*end == '{') {
	    section_open = 1;
	    *end = '\0';
	    val = strip(val, 0);
	}

	/* provess "value" in double-quotes */
	if (*val == '"' && *end == '"') {
	    *end = '\0';
	    val++;
	}

	/* if key is '}', a section has been closed */
	if (strcmp(key, "}") == 0) {
	    section_close = 1;
	    *key = '\0';
	}

	/* sanity check: '}' should be the only char in a line */
	if (section_close && (section_open || *val != '\0')) {
	    error("error in config file '%s' line %d: garbage after '}'", file, lineno);
	    error = 1;
	    break;
	}

	/* check key for valid chars */
	if (!validchars(key, 0)) {
	    error("error in config file '%s' line %d: key '%s' is invalid", file, lineno, key);
	    error = 1;
	    break;
	}

	/* on section-open, check value for valid chars */
	if (section_open && !validchars(val, 1)) {
	    error("error in config file '%s' line %d: section '%s' is invalid", file, lineno, val);
	    error = 1;
	    break;
	}

	/* on section-open, append new section name */
	if (section_open) {
	    /* is the section[] array big enough? */
	    if (strlen(section) + strlen(key) + 3 > sizeof(section)) {
		error("error in config file '%s' line %d: section buffer overflow", file, lineno);
		error = 1;
		break;
	    }
	    if (*section != '\0')
		strcat(section, ".");
	    strcat(section, key);
	    if (*val != '\0') {
		strcat(section, ":");
		strcat(section, val);
	    }
	    continue;
	}

	/* on section-close, remove last section name */
	if (section_close) {
	    /* sanity check: section already empty? */
	    if (*section == '\0') {
		error("error in config file '%s' line %d: unmatched closing brace", file, lineno);
		error = 1;
		break;
	    }

	    end = strrchr(section, '.');
	    if (end == NULL)
		*section = '\0';
	    else
		*end = '\0';
	    continue;
	}

	/* finally: add key */
	cfg_add(section, key, val, 0);

    }

    /* sanity check: are the braces balanced? */
    if (!error && *section != '\0') {
	error("error in config file '%s' line %d: unbalanced braces", file, lineno);
	error = 1;
    }

    fclose(stream);

    return -error;
}


int cfg_init(const char *file)
{
    if (cfg_check_source(file) == -1) {
	return -1;
    }

    if (cfg_read(file) < 0)
	return -1;

    if (Config_File)
	free(Config_File);
    Config_File = strdup(file);

    return 0;
}


char *cfg_source(void)
{
    if (Config_File)
	return Config_File;
    else
	return "";
}


int cfg_exit(void)
{
    int i;
    for (i = 0; i < nConfig; i++) {
	if (Config[i].key)
	    free(Config[i].key);
	if (Config[i].val)
	    free(Config[i].val);
    }

    if (Config) {
	free(Config);
	Config = NULL;
    }

    if (Config_File) {
	free(Config_File);
	Config_File = NULL;
    }

    return 0;
}
