/* $Id: cfg.c,v 1.22 2004/01/08 06:00:28 reinelt Exp $^
 *
 * config file stuff
 *
 * Copyright 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 * $Log: cfg.c,v $
 * Revision 1.22  2004/01/08 06:00:28  reinelt
 * allowed '.' in key names
 * allowed empty group keys (not only "group anything {", but "anything {")
 *
 * Revision 1.21  2004/01/08 05:28:12  reinelt
 * Luk Claes added to AUTHORS
 * cfg: group handling ('{}') added
 *
 * Revision 1.20  2004/01/07 10:15:41  reinelt
 * small glitch in evaluator fixed
 * made config table sorted and access with bsearch(),
 * which should be much faster
 *
 * Revision 1.19  2003/12/19 05:35:14  reinelt
 * renamed 'client' to 'plugin'
 *
 * Revision 1.18  2003/10/11 06:01:52  reinelt
 *
 * renamed expression.{c,h} to client.{c,h}
 * added config file client
 * new functions 'AddNumericVariable()' and 'AddStringVariable()'
 * new parameter '-i' for interactive mode
 *
 * Revision 1.17  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.16  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.15  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.14  2003/08/14 03:47:40  reinelt
 * remove PID file if driver initialisation fails
 *
 * Revision 1.13  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.12  2001/03/09 12:14:24  reinelt
 *
 * minor cleanups
 *
 * Revision 1.11  2001/03/08 15:25:38  ltoetsch
 * improved exec
 *
 * Revision 1.10  2001/03/07 18:10:21  ltoetsch
 * added e(x)ec commands
 *
 * Revision 1.9  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.8  2000/07/31 06:46:35  reinelt
 *
 * eliminated some compiler warnings with glibc
 *
 * Revision 1.7  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 * Revision 1.6  2000/04/03 04:46:38  reinelt
 *
 * added '-c key=val' option
 *
 * Revision 1.5  2000/03/28 07:22:15  reinelt
 *
 * version 0.95 released
 * X11 driver up and running
 * minor bugs fixed
 *
 * Revision 1.4  2000/03/26 20:00:44  reinelt
 *
 * README.Raster added
 *
 * Revision 1.3  2000/03/26 19:03:52  reinelt
 *
 * more Pixmap renaming
 * quoting of '#' in config file
 *
 * Revision 1.2  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 * Revision 1.1  2000/03/10 11:40:47  reinelt
 * *** empty log message ***
 *
 * Revision 1.3  2000/03/07 11:01:34  reinelt
 *
 * system.c cleanup
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
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
 * cfg_get (key, defval) 
 *   return the a value for a given key 
 *   or <defval> if key does not exist
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <unistd.h>
#include <sys/stat.h>

#include "debug.h"
#include "cfg.h"
#include "plugin.h"

typedef struct {
  char *key;
  char *val;
  int lock;
} ENTRY;


static char  *Config_File=NULL;
static ENTRY *Config=NULL;
static int   nConfig=0;


// bsearch compare function for config entries
static int c_lookup (const void *a, const void *b)
{
  char *key=(char*)a;
  ENTRY *entry=(ENTRY*)b;

  return strcasecmp(key, entry->key);
}


// qsort compare function for variables
static int c_sort (const void *a, const void *b)
{
  ENTRY *ea=(ENTRY*)a;
  ENTRY *eb=(ENTRY*)b;

  return strcasecmp(ea->key, eb->key);
}


// remove leading and trailing whitespace
static char *strip (char *s, int strip_comments)
{
  char *p;
  
  while (isblank(*s)) s++;
  for (p=s; *p; p++) {
    if (*p=='"')  do p++; while (*p && *p!='\n' && *p!='"');
    if (*p=='\'') do p++; while (*p && *p!='\n' && *p!='\'');
    if (*p=='\n' || (strip_comments && *p=='#' && (p==s || *(p-1)!='\\'))) {
      *p='\0';
      break;
    }
  }
  for (p--; p>s && isblank(*p); p--) *p='\0';
  return s;
}


// unquote a string
static char *dequote (char *string)
{
  char *s=string;
  char *p=string;
  
  do {
    if (*s=='\\' && *(s+1)=='#') {
      *p++=*++s;
    } else {
      *p++=*s;
    }
  } while (*s++);
  
  return string;
}


// which if a string contains only valid chars
// i.e. start with a char and contains chars and nums
static int validchars (char *string)
{
  char *c;

  for (c=string; *c; c++) {
    // first and following chars
    if ((*c>='A' && *c<='Z') || (*c>='a' && *c<='z') || (*c=='_')) continue;
    // only following chars
    if ((c>string) && ((*c>='0' && *c<='9') || (*c=='.'))) continue;
    return 0;
  }
  return 1;
}


static void cfg_add (char *group, char *key, char *val, int lock)
{
  char *buffer;
  ENTRY *entry;
  
  // allocate buffer 
  buffer=malloc(strlen(group)+strlen(key)+2);
  *buffer='\0';
  
  // prepare group:key
  if (*group!='\0') {
    strcpy(buffer, group);
    strcat(buffer, ".");
  }
  strcat (buffer, key);
  
  entry=bsearch(buffer, Config, nConfig, sizeof(ENTRY), c_lookup);
  
  if (entry!=NULL) {
    if (entry->lock>lock) return;
    if (entry->val) free (entry->val);
    entry->val=dequote(strdup(val));
    return;
  }
  
  nConfig++;
  Config=realloc(Config, nConfig*sizeof(ENTRY));
  Config[nConfig-1].key=strdup(buffer);
  Config[nConfig-1].val=dequote(strdup(val));
  Config[nConfig-1].lock=lock;

  qsort(Config, nConfig, sizeof(ENTRY), c_sort);

}


int l4l_cfg_cmd (char *arg)
{
  char *key, *val;
  char buffer[256];
  
  strncpy (buffer, arg, sizeof(buffer));
  key=strip(buffer, 0);
  for (val=key; *val; val++) {
    if (*val=='=') {
      *val++='\0';
      break;
    }
  }
  if (*key=='\0' || *val=='\0') return -1;
  if (!validchars(key)) return -1;
  cfg_add ("", key, val, 1);
  return 0;
}


char *l4l_cfg_get (char *key, char *defval)
{
  ENTRY *entry;

  entry=bsearch(key, Config, nConfig, sizeof(ENTRY), c_lookup);
  
  if (entry!=NULL)
    return entry->val;

  return defval;
}


int l4l_cfg_number (char *key, int defval, int min, int max, int *value) 
{
  
  char *s, *e;
  
  s=cfg_get(key, NULL);
  if (s==NULL) {
    *value=defval;
    return 0;
  }
  
  *value=strtol(s, &e, 0);
  if (*e!='\0') {
    error ("bad %s entry '%s' in %s", key, s, cfg_source());
    return -1;
  }
  
  if (*value<min) {
    error ("bad %s value '%s' in %s, minimum is %d", key, s, cfg_source(), min);
    return -1;
  }
  
  if (*value>max) {
    error ("bad %s value '%s' in %s, maximum is %d", key, s, cfg_source(), max);
    return -1;
  }

  return 0;
}


static int cfg_check_source(char *file)
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
    error ("stat(%s) failed: %s", file, strerror(errno));
    return -1;
  }
  if (S_ISCHR(stbuf.st_mode) && strcmp(file, "/dev/null") == 0)
    return 0;
  
  error=0;
  if (!S_ISREG(stbuf.st_mode)) {
    error ("security error: '%s' is not a regular file", file);
    error=-1;
  }
  if (stbuf.st_uid != uid || stbuf.st_gid != gid) {
    error ("security error: owner and/or group of '%s' don't match", file);
    error=-1;
  }
  if (stbuf.st_mode & S_IRWXG || stbuf.st_mode & S_IRWXO) {
    error ("security error: group or other have access to '%s'", file);
    error=-1;
  }
  return error;
}


static int cfg_read (char *file)
{
  FILE *stream;
  char buffer[256];
  char group[256];
  char *line, *key, *val, *end;
  int group_open, group_close;
  int error, lineno;
  
  stream=fopen (file, "r");
  if (stream==NULL) {
    error ("open(%s) failed: %s", file, strerror(errno));
    return -1;
  }

  // start with empty group
  strcpy(group, "");
  
  error=0;
  lineno=0;
  while ((line=fgets(buffer,256,stream))!=NULL) {

    // increment line number
    lineno++;
  
    // skip empty lines
    if (*(line=strip(line, 1))=='\0') continue;

    // reset group flags
    group_open=0;
    group_close=0;
    
    // key is first word
    key=line;
    
    // search first blank between key and value
    for (val=line; *val; val++) {
      if (isblank(*val)) {
	*val++='\0';
	break;
      }
    }

    // strip value
    val=strip(val, 1);

    // search end of value
    if (*val) for (end=val; *(end+1); end++);
    else end=val;

    // if last char is '{', a group has been opened
    if (*end=='{') {
      group_open=1;
      *end='\0';
      val=strip(val, 0);
    }

    // provess "value" in double-quotes
    if (*val=='"' && *end=='"') {
      *end='\0';
      val++;
    }

    // provess 'value' in single-quotes
    else if (*val=='\'' && *end=='\'') {
      *end='\0';
      val++;
    }
    
    // if key is '}', a group has been closed
    if (strcmp(key, "}")==0) {
      group_close=1;
      *key='\0';
    }

    // sanity check: '}' should be the only char in a line
    if (group_close && (group_open || *val!='\0')) {
      error ("error in config file '%s' line %d: garbage after '}'", file, lineno);
      error=1;
      break;
    }
    
    // check key for valid chars
    if (!validchars(key)) {
      error ("error in config file '%s' line %d: key '%s' is invalid", file, lineno, key);
      error=1;
      break;
    }

    // on group-open, check value for valid chars
    if (group_open && !validchars(val)) {
      error ("error in config file '%s' line %d: group '%s' is invalid", file, lineno, val);
      error=1;
      break;
    }

    // on group-open, append new group name
    if (group_open) {
      // is the group[] array big enough?
      if (strlen(group)+strlen(key)+3 > sizeof(group)) {
	error ("error in config file '%s' line %d: group buffer overflow", file, lineno);
	error=1;
	break;
      }
      if (*group!='\0') strcat (group, ".");
      strcat (group, key);
      if (*val!='\0') {
	strcat (group, ":");
	strcat (group, val);
      }
      continue;
    }

    // on group-close, remove last group name
    if (group_close) {
      // sanity check: group already empty?
      if (*group=='\0') {
	error ("error in config file '%s' line %d: unmatched closing brace", file, lineno);
	error=1;
	break;
      }
	
      end=strrchr(group, '.');
      if (end==NULL)
	*group='\0';
      else
	*end='\0';
      continue;
    }

    // finally: add key
    debug ("Michi: add group=<%s> key=<%s> val=<%s>", group, key, val);
    cfg_add (group, key, val, 0);
    
  }
  
  // sanity check: are the braces balanced?
  if (!error && *group!='\0') {
    error ("error in config file '%s' line %d: unbalanced braces", file, lineno);
    error=1;
  }

  fclose (stream);

  return -error;
}


static void cfg_plugin (RESULT *result, RESULT *arg1)
{
  char *value=cfg_get(R2S(arg1), "");
  SetResult(&result, R_STRING, value); 
}


int l4l_cfg_init (char *file)
{
  if (cfg_check_source(file) == -1) {
    error("config file '%s' is insecure, aborting", file);
    return -1;
  }
  
  if (cfg_read(file)<0) return -1;
  
  if (Config_File) free (Config_File);
  Config_File=strdup(file);
  
  // register as a plugin
  AddFunction ("cfg", 1, cfg_plugin);
  
  return 0;
}


char *l4l_cfg_source (void)
{
  if (Config_File)
    return Config_File;
  else
    return "";
}


int   (*cfg_init)   (char *source)                 = l4l_cfg_init;
char *(*cfg_source) (void)                         = l4l_cfg_source;
int   (*cfg_cmd)    (char *arg)                    = l4l_cfg_cmd;
char *(*cfg_get)    (char *key, char *defval)      = l4l_cfg_get;
int   (*cfg_number) (char *key, int   defval, 
		     int min, int max, int *value) = l4l_cfg_number;
