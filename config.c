#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

typedef struct {
  char *key;
  char *val;
} ENTRY;

static ENTRY *Config=NULL;
static int   nConfig=0;

static char *strip (char *s)
{
  char *p;
  
  while (isblank(*s)) s++;
  for (p=s; *p; p++) {
    if (*p=='"') do p++; while (*p && *p!='\n' && *p!='"');
    if (*p=='\'') do p++; while (*p && *p!='\n' && *p!='\'');
    if (*p=='#' || *p=='\n') {
      *p='\0';
      break;
    }
  }
  for (p--; p>s && isblank(*p); p--) *p='\0';
  return s;
}

void set_cfg (char *key, char *val)
{
  int i;
  
  for (i=0; i<nConfig; i++) {
    if (strcasecmp(Config[i].key, key)==0) {
      if (Config[i].val) free (Config[i].val);
      Config[i].val=strdup(val);
      return;
    }
  }
  nConfig++;
  Config=realloc(Config, nConfig*sizeof(ENTRY));
  Config[i].key=strdup(key);
  Config[i].val=strdup(val);
}

char *get_cfg (char *key)
{
  int i;

  for (i=0; i<nConfig; i++) {
    if (strcasecmp(Config[i].key, key)==0) {
      return Config[i].val;
    }
  }
  return NULL;
}


int read_cfg (char *file)
{
  FILE *stream;
  char buffer[256];
  char *line, *p, *s;
  
  stream=fopen (file, "r");
  if (stream==NULL) {
    fprintf (stderr, "open(%s) failed: %s\n", file, strerror(errno));
    return-1;
  }
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
    set_cfg (line, p);
  }
  fclose (stream);
  return 0;
}

