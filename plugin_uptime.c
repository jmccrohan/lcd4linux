/* $Id: plugin_uptime.c,v 1.3 2004/06/26 09:27:21 reinelt Exp $
 *
 * plugin for uptime
 *
 * Copyright 2003 Michael Reinelt <reinelt@eunet.at>
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
 * $Log: plugin_uptime.c,v $
 * Revision 1.3  2004/06/26 09:27:21  reinelt
 *
 * added '-W' to CFLAGS
 * changed all C++ comments to C ones ('//' => '/* */')
 * cleaned up a lot of signed/unsigned mistakes
 *
 * Revision 1.2  2004/06/20 10:09:56  reinelt
 *
 * 'const'ified the whole source
 *
 * Revision 1.1  2004/05/22 18:30:02  reinelt
 *
 * added plugin 'uptime'
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_uptime (void)
 *  adds functions for uptime
 *
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include "debug.h"
#include "plugin.h"

static int fd = -2;


static char *itoa(char* buffer, const size_t size, unsigned int value)
{
  char *p;
 
  /* sanity checks */
  if (buffer==NULL || size<2) return (NULL);
  
  /* p points to last char */
  p = buffer+size-1;
  
  /* set terminating zero */
  *p='\0';
  
  do {
    *--p = value%10 + '0';
    value = value/10;
  } while (value!=0 && p>buffer);
  
  return p;
} 


char *struptime (const unsigned int uptime, const char *format) 
{
  static char string[256];
  const char *src;
  char *dst;
  int len, size;
  
  src = format;
  dst = string;
  len = 0;
  
  /* leave room for terminating zero  */
  size = sizeof(string) - 1;
  
  while (len < size) {
    
    if (*src == '%') {
      src++;

      if (strchr ("sSmMhHd", *src) != NULL) {
	char buffer[12], *s;
	unsigned int value = 0;
	int leading_zero = 0;
	switch (*src++) {
	case 's':
	  value = uptime;
	  break;
	case 'S':
	  value = uptime % 60;
	  leading_zero = 1;
	  break;
	case 'm':
	  value = uptime / 60;
	  break;
	case 'M':
	  value = (uptime / 60) % 60;
	  leading_zero = 1;
	  break;
	case 'h':
	  value = uptime / 60 / 60;
	  break;
	case 'H':
	  value = (uptime / 60 / 60) % 24;
	  leading_zero = 1;
	  break;
	case 'd':
	  value = uptime / 60 / 60 / 24;
	  break;
	}
	
	if (leading_zero && value < 10) {
	  len++;
	  *dst++ = '0';
	}

	s = itoa (buffer, sizeof(buffer), value);
	while (len < size && *s != '\0') {
	  len++;
	  *dst++ = *s++;
	}

      } else if (*src == '%') {
	len++;
	*dst++ = '%';

      } else {
	len += 2;
	*dst++ = '%';
	*dst++ = *src++;
      }

    } else {
      len++;
      *dst++ = *src;
      if (*src++ == '\0') break;
    }
  }
  
  /* enforce terminating zero */
  if (len >= size && *(dst-1) != '\0') {
    len++;
    *dst = '\0';
  }

  return string;
}


double getuptime (void)
{
  char buffer[36];
  int i;
      
  if (fd == -2) fd = open ("/proc/uptime", O_RDONLY);
  if (fd < 0) return -1;

  lseek(fd, 0, SEEK_SET);

  i = read (fd, buffer, sizeof(buffer) - 1);
  if (i < 0) return -1;

  buffer[i-1] = '\0';
  
  /* ignore the 2nd value from /proc/uptime */
  return strtod(buffer, NULL);
}


static void my_uptime (RESULT *result, const int argc, RESULT *argv[])
{
  int age;
  static double uptime = 0.0;
  static struct timeval last_value;
  struct timeval now;

  if (argc>1) {
    error ("uptime(): wrong number of parameters");
    SetResult(&result, R_STRING, ""); 
    return;
  }

  gettimeofday(&now,NULL);
  
  age = (now.tv_sec - last_value.tv_sec)*1000 + (now.tv_usec - last_value.tv_usec)/1000;
  /* reread every 100 msec only */
  if (fd == -2 || age == 0 || age > 100) {
    uptime = getuptime();
    if (uptime < 0.0) {
      error ("parse(/proc/uptime) failed!");
      SetResult(&result, R_STRING, "");
      return;
    }

    last_value = now;
  }
  
  if (argc == 0) {
    SetResult (&result, R_NUMBER, &uptime);
  } else {
    SetResult (&result, R_STRING, struptime(uptime, R2S(argv[0])));
  }
  
  return;
  
}

int plugin_init_uptime (void)
{
  AddFunction ("uptime", -1, my_uptime);
  return 0;
}

void plugin_exit_uptime(void) 
{
  if (fd > 0) close(fd);
  fd = -2;
}
