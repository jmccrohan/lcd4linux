/* $Id: socket.c,v 1.3 2001/09/12 05:37:22 reinelt Exp $
 *
 * simple socket functions
 *
 * Copyright 2001 by Leopold Tötsch (lt@toetsch.at)
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
 * $Log: socket.c,v $
 * Revision 1.3  2001/09/12 05:37:22  reinelt
 *
 * fixed a bug in seti.c (file was never closed, lcd4linux run out of fd's
 *
 * improved socket debugging
 *
 * Revision 1.2  2001/03/15 14:25:05  ltoetsch
 * added unread/total news
 *
 * Revision 1.1  2001/03/14 13:19:29  ltoetsch
 * Added pop3/imap4 mail support
 *
 *
 */

/*
 * Exported Functions:
 * 
 * int open_socket(char *machine, int port);
 *
 *   open and connect to socket on machine:port
 *      returns fd on success or -1 on error
 *
 * 
 * int read_socket(int fd, char *buf, size_t size);
 * 
 *   read maximum size chars into buf
 *      returns n byte read, 0 on timeout, -1 on error
 * 
 *
 * int read_socket_match(int fd, char *buf, size_t size, char *match);
 * 
 *   read maximum size chars into buf and check if the start of line
 *   matches 'match'
 *      returns n on successful match, 0 on timeout/mismatch, -1 on error
 *     
 * 
 * int write_socket(int fd, char *string);
 *   
 *   write string to socket fd
 *      returns n byte written, -1 on error
 * 
 * with debuglevel 3, traffic on socket is logged
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "debug.h"

#define TIMEOUT 5 /* 5 seconds timeout */


static char *quotemeta (char *string)
{
  char buffer[256];
  char *s, *p;

  p=buffer;
  for (s=string; *s; s++) {
    if (isprint(*s)) {
      *p++=*s;
    } else {
      switch (*s) {
      case '\r':
	*p++='\\';
	*p++='r';
	break;
      case '\n':
	*p++='\\';
	*p++='n';
	break;
      default:
	p+=sprintf(p, "<\\%03o>", (int)*s);
      }
    }
    if ((p-buffer)>240) {
      *p++='.';
      *p++='.';
      *p++='.';
      break;
    }
  }

  *p='\0';
  return buffer;
}

static char *del_pass(char *s) 
{
  char *p;
  /* del pop3 pass from log */
  if (memcmp(s, "PASS ", 5) == 0)
    for (p = s+5; *p && *p != '\r'; p++)
      *p = '*';
  /* del imap4 pass from log */
  else if (memcmp(s, ". LOGIN", 7) == 0)
    for (p = s + strlen(s)-3 ; p > s && *p != ' '; p--)
      *p = '*';
  return s;
}


static void sockdebug (char dir, int len, char *string) {

  // delete passwords from log
  if (dir=='>') {
    del_pass (string);
  }
  message (3, "%c[%2d] %s", dir, len, quotemeta(string)); 
}


int open_socket(char *machine, int port)
{
  struct hostent	*addr;
  struct sockaddr_in s;
  int fd;

  addr = gethostbyname (machine);
  if (addr)
    memcpy (&s.sin_addr, addr->h_addr, sizeof (struct in_addr));
  else
    return -1;

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;

  s.sin_family = AF_INET;
  s.sin_port = htons (port);

  if (connect (fd, (struct sockaddr *)&s, sizeof (s)) < 0)
    return -1;
  return fd;
}

int read_socket(int fd, char *buf, size_t size)
{
  fd_set readfds;
  struct timeval tv;
  int n;

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  if (select(fd+1, &readfds, NULL, NULL, &tv) > 0)
    n = read(fd, buf, size);
  else
    n = 0;

  if (n >= 0)
    buf[n] = '\0';
  else
    buf[0] = '\0';
  sockdebug('<', n, buf);
  return n;
}

int read_socket_match(int fd, char *buf, size_t size, char *match)
{
  int n = read_socket(fd, buf, size);
  int len;
  if (n <= 0)
    return n;
  len = strlen(match);
  if (n >= len && memcmp(buf, match, len) == 0)
    return n;
  return 0;
}

int write_socket(int fd, char *buf) 
{
  int n = write(fd, buf, strlen(buf));
  sockdebug('>', n, del_pass(buf));
  return n;
}

