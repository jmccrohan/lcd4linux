/* $Id: parser.c,v 1.10 2001/02/19 00:15:46 reinelt Exp $
 *
 * row definition parser
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
 * $Log: parser.c,v $
 * Revision 1.10  2001/02/19 00:15:46  reinelt
 *
 * integrated mail and seti client
 * major rewrite of parser and tokenizer to support double-byte tokens
 *
 * Revision 1.9  2001/02/16 08:23:09  reinelt
 *
 * new token 'ic' (ISDN connected) by Carsten Nau <info@cnau.de>
 *
 * Revision 1.8  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.7  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.6  2000/05/21 06:20:35  reinelt
 *
 * added ppp throughput
 * token is '%t[iomt]' at the moment, but this will change in the near future
 *
 * Revision 1.5  2000/03/24 11:36:56  reinelt
 *
 * new syntax for raster configuration
 * changed XRES and YRES to be configurable
 * PPM driver works nice
 *
 * Revision 1.4  2000/03/19 08:41:28  reinelt
 *
 * documentation available! README, README.MatrixOrbital, README.Drivers
 * added Skeleton.c as a starting point for new drivers
 *
 * Revision 1.3  2000/03/18 10:31:06  reinelt
 *
 * added sensor handling (for temperature etc.)
 * made data collecting happen only if data is used
 * (reading /proc/meminfo takes a lot of CPU!)
 * released lcd4linux-0.92
 *
 * Revision 1.2  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.1  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 */

/* 
 * exported functions:
 *
 * char *parse_row (char *string, int supported_bars, int usage[])
 *    converts a row definition from the config file
 *    into the internal form using tokens
 *    sets the array usage[token] to usage count
 *
 * char parse_gpo (char *string, int usage[])
 *    converts a GPO definition from the config file
 *    into the internal form using tokens
 *    sets the array usage[token] to usage count
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "debug.h"
#include "display.h"
#include "parser.h"

typedef struct {
  char *symbol;
  TOKEN token;
  CLASS class;
  int bar;
} SYMTAB;

static SYMTAB Symtab[] = {{ "%",  T_PERCENT,    C_GENERIC, 0 },
			  { "$",  T_DOLLAR,     C_GENERIC, 0 },
			  { "o",  T_OS,         C_GENERIC, 0 },
			  { "v",  T_RELEASE,    C_GENERIC, 0 },
			  { "p",  T_CPU,        C_GENERIC, 0 },
			  { "r",  T_RAM,        C_GENERIC, 0 },
			  { "mt", T_MEM_TOTAL,  C_MEM,     1 },
			  { "mu", T_MEM_USED,   C_MEM,     1 },
			  { "mf", T_MEM_FREE,   C_MEM,     1 },
			  { "ms", T_MEM_SHARED, C_MEM,     1 },
			  { "mb", T_MEM_BUFFER, C_MEM,     1 },
			  { "mc", T_MEM_CACHE,  C_MEM,     1 },
			  { "ma", T_MEM_AVAIL,  C_MEM,     1 },
			  { "l1", T_LOAD_1,     C_LOAD,    1 },
			  { "l2", T_LOAD_2,     C_LOAD,    1 },
			  { "l3", T_LOAD_3,     C_LOAD,    1 },
			  { "L",  T_OVERLOAD,   C_LOAD,    0 },
			  { "cu", T_CPU_USER,   C_CPU,     1 },
			  { "cn", T_CPU_NICE,   C_CPU,     1 },
			  { "cs", T_CPU_SYSTEM, C_CPU,     1 },
			  { "cb", T_CPU_BUSY,   C_CPU,     1 },
			  { "ci", T_CPU_IDLE,   C_CPU,     1 },
			  { "dr", T_DISK_READ,  C_DISK,    1 },
			  { "dw", T_DISK_WRITE, C_DISK,    1 },
			  { "dt", T_DISK_TOTAL, C_DISK,    1 },
			  { "dm", T_DISK_MAX,   C_DISK,    1 },
			  { "nr", T_ETH_RX,     C_ETH,     1 },
			  { "nw", T_ETH_TX,     C_ETH,     1 },
			  { "nt", T_ETH_TOTAL,  C_ETH,     1 },
			  { "nm", T_ETH_MAX,    C_ETH,     1 },
			  { "ic", T_ISDN_USED,  C_ISDN,    0 },
			  { "ii", T_ISDN_IN,    C_ISDN,    1 },
			  { "io", T_ISDN_OUT,   C_ISDN,    1 },
			  { "it", T_ISDN_TOTAL, C_ISDN,    1 },
			  { "im", T_ISDN_MAX,   C_ISDN,    1 },
			  { "ti", T_PPP_RX,     C_PPP,     1 },
			  { "to", T_PPP_TX,     C_PPP,     1 },
			  { "tt", T_PPP_TOTAL,  C_PPP,     1 },
			  { "tm", T_PPP_MAX,    C_PPP,     1 },
			  { "hc", T_SETI_PRC,   C_SETI,    1 },
			  { "ht", T_SETI_CPU,   C_SETI,    0 },
			  { "e*", T_MAIL,       C_MAIL,    0 },
			  { "s*", T_SENSOR,     C_SENSOR,  1 },
			  { "",  -1,            0 }};

static int bar_type (char tag)
{
  switch (tag) {
  case 'l':
    return BAR_L;
  case 'r':
    return BAR_R;
  case 'u':
    return BAR_U;
  case 'd':
    return BAR_D;
  default:
    return 0;
  }
}

static int get_token (char *s, char **p, int bar, int usage[])
{
  char c;
  int  i, l;
  
  for (i=0; Symtab[i].token!=-1; i++) {
    if (bar && !Symtab[i].bar) continue;
    l=strlen(Symtab[i].symbol);
    if (Symtab[i].symbol[l-1]=='*') {
      if (Symtab[i].token<T_EXTENDED) {
	error ("ERROR: internal error in symbol '%s'", Symtab[i].symbol);
	return -1;
      }
      c=*(s+l-1);
      if (strncmp(Symtab[i].symbol, s, l-1)==0 && c>='1' && c<='9') {
	*p=s+l;
	usage[Symtab[i].token]|=(1<<(c-'0'));
	usage[Symtab[i].class]=1;
	return Symtab[i].token+(c<<8);
      }
    } else if (strncmp(Symtab[i].symbol, s, l)==0) {
      *p=s+l;
      usage[Symtab[i].token]=1;
      usage[Symtab[i].class]=1;
      return Symtab[i].token;
    }
  }
  return -1;
}

char *parse_row (char *string, int supported_bars, int usage[])
{
  static char buffer[256];
  char *s=string;
  char *p=buffer;
  int token, token2;
  int type, len;

  do {
    switch (*s) {
      
    case '%':
      if ((token=get_token (++s, &s, 0, usage))==-1) {
	error ("WARNING: unknown token <%%%c> in <%s>", *s, string);
      } else {
	*p++='%';
	*p++=token&255;
	if (token>256) *p++=token>>8;
      }
      break;
      
    case '$':
      type=bar_type(tolower(*++s));
      if (type==0) {
	error ("WARNING: invalid bar type <$%c> in <%s>", *s, string);
	break;
      }
      if (!(type & supported_bars)) {
	error ("WARNING: driver does not support bar type '%c'", *s);
	break;
      }
      if (isupper(*s)) type |= BAR_LOG;
      len=strtol(++s, &s, 10);
      if (len<1 || len>127) {
	error ("WARNING: invalid bar length in <%s>", string);
	break;
      }
      if ((token=get_token (s, &s, 0, usage))==-1) {
	error ("WARNING: unknown token <$%c> in <%s>", *s, string);
	break;
      }
      token2=-1;
      if (*s=='+') {
	if ((type & BAR_H) && (supported_bars & BAR_H2)) {
	  type |= BAR_H2;
	} else if ((type & BAR_V) && (supported_bars & BAR_V2)) {
	  type |= BAR_V2;
	} else {
	  error ("WARNING: driver does not support double bars");
	  break;
	}
	if ((token2=get_token (s+1, &s, 0, usage))==-1) {
	  error ("WARNING: unknown token <$%c> in <%s>", *s, string);
	  break;
	}
      }
      *p++='$';
      *p++=type;
      *p++=len;
      *p++=token&255;
      if (token>256) *p++=token>>8;
      if (token2!=-1) {
	*p++=token2&255;
	if (token>256) *p++=token2>>8;
      }
      break;
      
    case '\\':
      if (*(s+1)=='\\') {
	*p++='\\';
	s++;
      } else {
	unsigned int c=0; int n;
	sscanf (s+1, "%3o%n", &c, &n);
	if (c==0 || c>255) {
	  error ("WARNING: illegal '\\' in <%s>", string);
	} else {
	  *p++=c;
	  s+=n+1;
	}
      }
      break;
      
    default:
      if ((*p++=*s)!='\0') s++;
    }
    
  } while (*s);

  *p='\0';
  return buffer;
}

int parse_gpo (char *string, int usage[])
{
  char *s=string;
  int token=-1;
  
  if (*s=='%') {
    if ((token=get_token (++s, &s, 0, usage))==-1) {
      error ("WARNING: unknown token <%%%c> in <%s>", *s, string);
    }
  }
  
  if (*s!='\0') {
    error ("WARNING: error while parsing <%s>", string);
  }
  
  return token;
}
