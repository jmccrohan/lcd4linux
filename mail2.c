/* $Id: mail2.c,v 1.1 2001/03/14 13:19:29 ltoetsch Exp $
 *
 * mail: pop3, imap functions
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
 * $Log: mail2.c,v $
 * Revision 1.1  2001/03/14 13:19:29  ltoetsch
 * Added pop3/imap4 mail support
 *
 *
 * Exported Functions:
 *
 * int Mail_pop_imap(char *mbox, int *total_mails, int *unseen);
 *     returns -1 on error, 0 on success
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>

#include "debug.h"
#include "socket.h"

#define PROTO_UNKNOWN -1
#define PROTO_POP3 110
#define PROTO_IMAP4 143

/*
 * parse_proto()
 *
 * parse a MailboxN entry in 's' as
 *
 * proto:[user[:pass]@]machine[:port][/dir]
 *
 * 's' get's destroyed
 * returns 0 on success, -1 on error
 *
 */

static int parse_proto(char *s, int *proto, char **user, char **pass,
		       char **machine, int *port, char **dir)
{
  struct
    {
      char *prefix;
      int proto;
    }
  protos[] =
    {
	{ "pop3:", PROTO_POP3 },
	{ "imap4:", PROTO_IMAP4 },
    };
  int i;
  char *p, *q;
  static char empty[] = "";
  static char INBOX[] = "INBOX";

  *proto = *port = PROTO_UNKNOWN;
  for (i=0; i< sizeof(protos)/sizeof(protos[0]); i++)
    {
      if (memcmp(s, protos[i].prefix, strlen(protos[i].prefix)) == 0)
	{
	  *proto = *port = protos[i].proto;
	  break;
	}
    }
  if (*proto == PROTO_UNKNOWN)
    return -1;

  p = s + strlen(protos[i].prefix);
  /*
   * this might fail if user or pass contains a '/'
   * 
   */
  if ((q = strchr(p, '/')) != NULL)
    {
      /* /dir */
      *dir = q + 1;
      *q = '\0';
    }
  else
    *dir = empty;

  if ((q = strchr(p, '@')) != NULL)
    {
      /* user, pass is present */
      *machine = q + 1;
      *q = '\0';
      *user = p;
      if ((q = strchr(p, ':')) != NULL)
	{
	  /* user[:pass] */
	  *q = '\0';
	  *pass = q+1;
	}
      else
	*pass = empty;
    }
  else
    {
      *machine = p;
      *user = *pass = empty;
    }

  if ((q = strchr(*machine, ':')) != NULL)
    {
      /* machine[:pass] */
      *q = '\0';
      *port = atoi(q+1);
      if (*port <= 0 || *port >= 0xffff)
	return -1;
    }
  if (!**machine)
    return -1;
  if (*proto == PROTO_POP3 && **dir)
    return -1;
  if (*proto == PROTO_IMAP4 && !**dir)
    *dir = INBOX;
  return 0;
}


/* write buffer, compare with match */

#define BUFLEN 256
static int wr_rd(int fd, char *buf, char *match, 
		 char *err, char *machine, int port)
{
  int n;
  n = write_socket(fd, buf);
  if (n <= 0)
    {
      error("Couldn't write to %s:%d (%s)", machine, port, strerror(errno));
      close(fd);
      return -1;
    }
  n = read_socket_match(fd, buf, BUFLEN-1, match);
  if (n <= 0)
    {
      error("%s %s:%d (%s)", err, machine, port, strerror(errno));
      close(fd);
      return -1;
    }
  return n;
}

static int check_imap4(char *user, char *pass, char *machine,
		       int port, char *dir, int *total, int *unseen)
{
  int fd = open_socket(machine, port);
  int n;
  char buf[BUFLEN];
  char *p;

  if (fd < 0)
    {
      error("Couldn't connect to %s:%d (%s)", machine, port, strerror(errno));
      return -1;
    }
  n = read_socket_match(fd, buf, BUFLEN-1, "* OK");
  if (n <= 0) {
    error("Server doesn't respond %s:%d (%s)", machine, port, strerror(errno));
    close(fd);
    return -1;
  }
  sprintf(buf, ". LOGIN %s %s\r\n", user, pass);
  if (wr_rd(fd, buf, ". OK", "Wrong User/PASS?", machine, port) <= 0)
    return -1;
  sprintf(buf, ". STATUS %s (MESSAGES UNSEEN)\r\n", dir);
  if (wr_rd(fd, buf, "*", "Wrong dir?", machine, port) <= 0)
    return -1;
  if ((p = strstr(buf, "MESSAGES")) != NULL)
    sscanf(p, "%*s %d", total);
  else {
    error("Server doesn't provide MESSAGES (%s)", machine, port, strerror(errno));
    close(fd);
    return -1;
  }
  if ((p = strstr(buf, "UNSEEN")) != NULL)
    sscanf(p, "%*s %d", unseen);
  else {
    error("Server doesn't provide UNSEEN (%s)", machine, port, strerror(errno));
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

static int check_pop3(char *user, char *pass, char *machine,
		      int port, int *total, int *unseen)
{
  int fd = open_socket(machine, port);
  int n;
  char buf[BUFLEN];

  if (fd < 0)
    {
      error("Couldn't connect to %s:%d (%s)", machine, port, strerror(errno));
      return -1;
    }
  n = read_socket_match(fd, buf, BUFLEN-1, "+OK");
  if (n <= 0) {
    error("Server doesn't respond %s:%d (%s)", machine, port, strerror(errno));
    close(fd);
    return -1;
  }
  if (*user)
    {
      sprintf(buf, "USER %s\r\n", user);
      if (wr_rd(fd, buf, "+OK", "Wrong USER?", machine, port) <= 0)
	return -1;
    }
  if (*pass)
    {
      sprintf(buf, "PASS %s\r\n", pass);
      if (wr_rd(fd, buf, "+OK", "Wrong PASS?", machine, port) <= 0)
	return -1;
    }
  sprintf(buf, "STAT\r\n");
  if (wr_rd(fd, buf, "+OK", "Wrong STAT answer?", machine, port) <= 0)
    return -1;
  sscanf(buf, "+OK %d", total);
  if (*total) {
    sprintf(buf, "LAST\r\n");
    if (wr_rd(fd, buf, "+OK", "Wrong LAST answer?", machine, port) <= 0)
      return -1;
    sscanf(buf, "+OK %d", unseen);
    *unseen = *total - *unseen;
  }
  close(fd);
  return 0;
}

int Mail_pop_imap(char *s, int *total, int *unseen)
{
  int proto, port, ret;
  char *user, *pass, *machine, *dir, *ds;
  
  ds = strdup(s);
  if (ds == NULL)
    {
      error("Out of mem");
      return -1;
    }
  ret = parse_proto(ds, &proto, &user, &pass,
		    &machine, &port, &dir)  ;
  if (ret < 0)
    error("Not a pop3/imap4 mailbox");
  else
    ret = (proto == PROTO_POP3) ?
    check_pop3(user, pass, machine, port, total, unseen) :
  check_imap4(user, pass, machine, port, dir, total, unseen);
  free(ds);
  return ret;
}

#ifdef STANDALONE

/*
 * test parse_proto with some garbage
 *
 */

int foreground = 1;
int debugging = 3;

/*
 * for STANDALONE tests, disable Text driver and
 * 
 * cc -DSTANDALONE mail2.c socket.c debug.c -g -Wall -o mail2
 * 
 */ 

#ifdef CHECK_PARSER

static int test(char *s)
{
  int ret;
  int proto, port;
  char *user, *pass, *machine, *dir, *ds;

  ds = strdup(s);
  ret = parse_proto(ds, &proto, &user, &pass,
		    &machine, &port, &dir)  ;
  printf("parse_proto(%s) ret=%d\n", s, ret);
  if (!ret)
    printf(
	   "\tproto='%d'\n"
	   "\tuser='%s'\n"
	   "\tpass='%s'\n"
	   "\tmachine='%s'\n"
	   "\tport='%d'\n"
	   "\tdir='%s'\n",
	   proto,user,pass,machine,port,dir);
  free(ds);
  return ret;
}
#endif

int main(int argc, char *argv[])
{

#ifdef CHECK_PARSER
  int i, ret;
  /* proto:[user[:pass]@]machine[:port][/dir] */
  char *t[] =
    {
      "pop3:sepp:Geheim@Rechner:123",
	"pop3:sepp@Rechner",
	"pop3:Rechner:123",
	"imap4:sepp:Geheim@Rechner/dir@/:pfad",
	"imap4:sepp:Geheim@Rechner",
	"imap4sepp:Geheim@Rechner/dir@/:pfad",
	"imap4:sepp:Geheim/Rechner/dir@/:pfad",
	"pop3:sepp@:",
	0
    };

  ret = 0;
  if (argc > 1)
    ret |= test(argv[1]);
  else
    for (i = 0; t[i]; i++)
      ret |= test(t[i]);
  return ret;
# else

  int total = -1, unseen = -1;
  char *mbx = "imap4:user:pass@server/folder/file";
  int ret = Mail_pop_imap(mbx, &total, &unseen);
  printf("ret = %d, total = %d unseen = %d\n", ret, total, unseen);
  return ret;
  
# endif
}

#endif
