/* $Id: cfg.c,v 1.4 2000/03/26 20:00:44 reinelt Exp $
 *
 * config file stuff
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
 * $Log: cfg.c,v $
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
 * cfg_set (key, value)
 *   pre-set key's value
 *   should be called before cfg_read()
 *   so we can specify 'default values'
 *
 * cfg_get (key) 
 *   return the a value for a given key 
 *   or NULL if key does not exist
 *
 * cfg_read (file)
 *   read configuration from file   
 *   returns  0 if successful
 *   returns -1 in case of an error
 * 
 * cfg_file (void)
 *   returns the file the configuration was read from
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cfg.h"

typedef struct {
  char *key;
  char *val;
} ENTRY;

static char  *Config_File=NULL;
static ENTRY *Config=NULL;
static int   nConfig=0;


static char *strip (char *s)
{
  char *p;
  
  while (isblank(*s)) s++;
  for (p=s; *p; p++) {
    if (*p=='"') do p++; while (*p && *p!='\n' && *p!='"');
    if (*p=='\'') do p++; while (*p && *p!='\n' && *p!='\'');
    if (*p=='\n' || (*p=='#' && (p==s || *(p-1)!='\\'))) {
      *p='\0';
      break;
    }
  }
  for (p--; p>s && isblank(*p); p--) *p='\0';
  return s;
}

static char *dequote (char *string)
{
  char *s=string;
  char *p=string;
  
  do {
    if (*s!='\\') *p++=*s;
  } while (*s++);
  
  return string;
}

void cfg_set (char *key, char *val)
{
  int i;
  
  for (i=0; i<nConfig; i++) {
    if (strcasecmp(Config[i].key, key)==0) {
      if (Config[i].val) free (Config[i].val);
      Config[i].val=dequote(strdup(val));
      return;
    }
  }
  nConfig++;
  Config=realloc(Config, nConfig*sizeof(ENTRY));
  Config[i].key=strdup(key);
  Config[i].val=dequote(strdup(val));
}

char *cfg_get (char *key)
{
  int i;

  for (i=0; i<nConfig; i++) {
    if (strcasecmp(Config[i].key, key)==0) {
      return Config[i].val;
    }
  }
  return NULL;
}

int cfg_read (char *file)
{
  FILE *stream;
  char buffer[256];
  char *line, *p, *s;
  
  stream=fopen (file, "r");
  if (stream==NULL) {
    fprintf (stderr, "open(%s) failed: %s\n", file, strerror(errno));
    return-1;
  }

  if (Config_File) free (Config_File);
  Config_File=strdup(file);
    
  while ((line=fgets(buffer,256,stream))!=NULL) {
    if (*(line=strip(line))=='\0') continue;
    for (p=line; *p; p++) {
      if (isblank(*p)) {
	*p++='\0';
	break;
      }
    }
    p=strip(p);
    if (*p) for (s=p; *(s+1); s++);
    else s=p;
    if (*p=='"' && *s=='"') {
      *s='\0';
      p++;
    }
    else if (*p=='\'' && *s=='\'') {
      *s='\0';
      p++;
    }
    cfg_set (line, p);
  }
  fclose (stream);
  return 0;
}

char *cfg_file (void)
{
  if (Config_File)
    return Config_File;
  else
    return "";
}
