/* $Id: imon.c,v 1.3 2004/01/09 04:16:06 reinelt Exp $
 *
 * imond/telmond data processing
 *
 * Copyright 2003 Nico Wallmeier <nico.wallmeier@post.rwth-aachen.de>
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
 * $Log: imon.c,v $
 * Revision 1.3  2004/01/09 04:16:06  reinelt
 * added 'section' argument to cfg_get(), but NULLed it on all calls by now.
 *
 * Revision 1.2  2004/01/06 22:33:14  reinelt
 * Copyright statements cleaned up
 *
 * Revision 1.1  2003/10/12 06:08:28  nicowallmeier
 * imond/telmond support
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>                          /* decl of inet_addr()      */
#include <sys/socket.h>

#include "cfg.h"
#include "debug.h"
#include "parser.h"
#include "imon.h"

#define TRUE                        1
#define FALSE                       0

static int      fd;

 /*----------------------------------------------------------------------------
 *  service_connect (host_name, port)       - connect to tcp-service
 *----------------------------------------------------------------------------
 */
static int
service_connect (char * host_name, int port)
{
    struct sockaddr_in  addr;
    struct hostent *    host_p;
    int                 fd;
    int                 opt = 1;

    (void) memset ((char *) &addr, 0, sizeof (addr));

    if ((addr.sin_addr.s_addr = inet_addr ((char *) host_name)) == INADDR_NONE)
    {
        host_p = gethostbyname (host_name);

        if (! host_p)
        {
            error ("%s: host not found\n", host_name);
            return (-1);
        }

        (void) memcpy ((char *) (&addr.sin_addr), host_p->h_addr,
                        host_p->h_length);
    }

    addr.sin_family  = AF_INET;
    addr.sin_port    = htons ((unsigned short) port);

    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {                                           /* open socket              */
        perror ("socket");
        return (-1);
    }
    
    (void) setsockopt (fd, IPPROTO_TCP, TCP_NODELAY,
                       (char *) &opt, sizeof (opt));

    if (connect (fd, (struct sockaddr *) &addr, sizeof (addr)) != 0)
    {
        (void) close (fd);
        perror (host_name);
        return (-1);
    }

    return (fd);
} /* service_connect (char * host_name, int port) */


/*----------------------------------------------------------------------------
 *  send_command (int fd, char * str)       - send command to imond
 *----------------------------------------------------------------------------
 */
static void
send_command (int fd, char * str)
{
    char    buf[256];
    int     len = strlen (str);

    sprintf (buf, "%s\r\n", str);
    write (fd, buf, len + 2);

    return;
} /* send_command (int fd, char * str) */


/*----------------------------------------------------------------------------
 *  get_answer (int fd)                     - get answer from imond
 *----------------------------------------------------------------------------
 */
static char *
get_answer (int fd)
{
    static char buf[8192];
    int         len;

    len = read (fd, buf, 8192);

    if (len <= 0)
    {
        return ((char *) NULL);
    }

    while (len > 1 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    {
        buf[len - 1] = '\0';
        len--;
    }

    if (! strncmp (buf, "OK ", 3))                      /* OK xxxx          */
    {
        return (buf + 3);
    }
    else if (len > 2 && ! strcmp (buf + len - 2, "OK"))
    {
        *(buf + len - 2) = '\0';
        return (buf);
    }
    else if (len == 2 && ! strcmp (buf + len - 2, "OK"))
    {
        return (buf);
    }

    return ((char *) NULL);                             /* ERR xxxx         */
} /* get_answer (int fd) */


/*----------------------------------------------------------------------------
 *  get_numerical_value (char * cmd)   - send cmd, get numval
 *----------------------------------------------------------------------------
 */
static int
get_numerical_value (char * cmd)
{
    char *  answer;
    int     rtc;

    send_command (fd, cmd);

    answer = get_answer (fd);

    if (answer)
    {
        rtc = atoi (answer);
    }
    else
    {
        rtc = -1;
    }
    return (rtc);
} /* get_numerical_value (char * cmd, int arg) */


/*----------------------------------------------------------------------------
 *  get_value (char * cmd)         - send command, get value
 *----------------------------------------------------------------------------
 */
static char *
get_value (char * cmd)
{
    char *  answer;

    send_command (fd, cmd);

    answer = get_answer (fd);

    if (answer)
    {
        return (answer);
    }

    return ("");
} /* get_value (char * cmd, int arg) */


int init(){
 char *s, *host;
 int port;
 int connect;

 host=cfg_get (NULL, "Imon_Host", "127.0.0.1");
 if (*host=='\0') {
  error ("Imon: no 'Imon_Host' entry in %s", cfg_source());
  return -1;
 } 

 if (cfg_number(NULL, "Imon_Port", 5000,1,65536,&port)<0){
   return -1;	
 }
 
 connect=service_connect(host,port);

 s=cfg_get (NULL, "Imon_Pass", NULL);
 if ((s!=NULL) && (*s!='\0')) { // Passwort senden
   char buf[40];
   sprintf(buf,"pass %s",s);
   send_command(connect,buf);
   s=get_answer(connect); 
 }
 
 return connect; 	
}
 
int ImonCh(int index, struct imonchannel *ch, int token_usage[]) {
 static int err[CHANNELS+1];
 char *s;
 char buf[40];
 int result=0;
 
 if (err[index]) return -1;
 if ((fd==0) && ((fd=init())<0)) return -1;
	 
 if ((*ch).max_in == 0){ // not initializied
  sprintf(buf, "Imon_%d_Dev", index);
  s=cfg_get(NULL, buf, NULL);
  if (s==NULL) {
   error ("Imon: no 'Imon_%i_Dev' entry in %s", index, cfg_source());
   err[index]=1;
   return -1;
  } 
  strcpy((*ch).dev,s);
  
  sprintf(buf, "Imon_%d_MaxIn", index);
  cfg_number(NULL, buf,768,1,65536,&(*ch).max_in);

  sprintf(buf, "Imon_%d_MaxOut", index);
  cfg_number(NULL, buf, 128,1,65536,&(*ch).max_out);
 }

 sprintf(buf, "status %s", (*ch).dev);
 s=get_value(buf);
 strcpy((*ch).status,s);
 
 if ((1<<index) & token_usage[T_IMON_CHARGE]) {
   sprintf(buf, "charge %s", (*ch).dev);
   s=get_value(buf);
   strcpy((*ch).charge,s);
 }
    
 
 if (strcmp("Online",(*ch).status)==0){
   if ((1<<index) & token_usage[T_IMON_PHONE]) {
     sprintf(buf, "phone %s", (*ch).dev);
     s=get_value(buf);
     strcpy((*ch).phone,s);
   }

   if (((1<<index) & token_usage[T_IMON_RIN]) ||
       ((1<<index) & token_usage[T_IMON_ROUT])) {
     sprintf(buf, "rate %s", (*ch).dev);
     s=get_value(buf);
     if (sscanf(s,"%d %d",&((*ch).rate_in), &((*ch).rate_out))!=2) result--;
   }
   
   if ((1<<index) & token_usage[T_IMON_IP]) {
     sprintf(buf, "ip %s", (*ch).dev);
     s=get_value(buf);
     strcpy((*ch).ip,s);
   }
   
   if ((1<<index) & token_usage[T_IMON_OTIME]) {
   	 sprintf(buf, "online-time %s", (*ch).dev);
     s=get_value(buf);
     strcpy((*ch).otime,s);
   }
 } else {
   if (strcmp("Dialing",(*ch).status)==0){
     if ((1<<index) & token_usage[T_IMON_PHONE]) {
       sprintf(buf, "phone %s", (*ch).dev);
       s=get_value(buf);
       strcpy((*ch).phone,s);
     }
   } else {
     if ((1<<index) & token_usage[T_IMON_PHONE]) (*ch).phone[0]='\0';
   }
   if ((1<<index) & token_usage[T_IMON_IP]) (*ch).ip[0]='\0';	
   if ((1<<index) & token_usage[T_IMON_OTIME]) (*ch).otime[0]='\0';
   if (((1<<index) & token_usage[T_IMON_RIN]) ||
       ((1<<index) & token_usage[T_IMON_ROUT])) {
   	 (*ch).rate_in=0;
     (*ch).rate_out=0;	
   }
 }
 return result;
} 
 
 
int Imon(struct imon *i, int cpu, int datetime){
 static int hb;
 static int tick;
 char *s;
 char day[4];
 char d[13];
 
 if (tick++ % 5 != 0) return 0;
 
 if ((fd==0) && ((fd=init())<0)) return -1;
 
 if (cpu) (*i).cpu = get_numerical_value("cpu");

 if (datetime){ 
   s = get_value ("date");
   sscanf (s, "%s %s %s", day, d, (*i).time);
   strncpy ((*i).date, d, 6);
   strcpy ((*i).date + 6, d + 8);
   (*i).date[2]='.';
   (*i).date[5]='.';
   if (hb) (*i).time[5] =' ';
   hb=!hb;
 }
 return 0;
}

char* ImonVer(){
  static char buffer[32]="";

  if (*buffer=='\0') {
   char *s;
   if ((fd==0) && ((fd=init())<0)) return "";
   s=get_value("version");
   for (;;){ // interne Versionsnummer killen
   	if (s[0]==' '){
     s=s+1;
     break;
    }
    s=s+1;		
   }
   strcpy(buffer,s);  
  }
  return buffer;
}

void phonebook(char *number){
  FILE *  fp;
  char line[256];
  
  fp = fopen (cfg_get (NULL, "Telmon_Phonebook","/etc/phonebook"), "r");
  
  if (! fp) return;
  
  while (fgets (line, 128, fp)){
  	if (*line == '#') continue;
  	if (!strncmp(line,number,strlen(number))){
  	  char *komma=strchr(line,',');
  	  char *beginn=strchr(line,'=');
  	  if (!beginn) return;
  	  while (strrchr(line,'\r')) strrchr(line,'\r')[0]='\0';
  	  while (strrchr(line,'\n')) strrchr(line,'\n')[0]='\0';
  	  if (komma) komma[0]='\0';
  	  strcpy(number,beginn+1);
  	  break;
  	}  	
  }
  
  fclose(fp);
}

int Telmon(struct telmon *t){
 static int tick;
 static int telmond_fd=-2;
 static char oldanswer[128];
 static char host[256];
 static int port;
 
 if (tick++ % 50 != 0) return 0;

 if (telmond_fd == -2){ //not initializied
  char *s=cfg_get (NULL, "Telmon_Host","127.0.0.1");
  if (*s=='\0') {
   error ("Telmon: no 'Telmon_Host' entry in %s", cfg_source());
   telmond_fd=-1;
   return -1;
  } 
  strcpy(host,s);
  
  if (cfg_number(NULL, "Telmon_Port",5000,1,65536,&port)<0){
   telmond_fd=-1;
   return -1;	
  }
 }
 
 if (telmond_fd != -1){
  char    telbuf[128];

  telmond_fd = service_connect (host, port);
  if (telmond_fd >= 0){
   int l = read (telmond_fd, telbuf, 127);
   if ((l > 0) && (strcmp(telbuf,oldanswer))){
	char date[11]; 
	sscanf(telbuf,"%s %s %s %s",date,(*t).time,(*t).number,(*t).msn);
	date[4]='\0';
	date[7]='\0';
	sprintf((*t).date,"%s.%s.%s",date+8,date+5,date);
	phonebook((*t).number);
	phonebook((*t).msn);
   }
   close (telmond_fd);
   strcpy(oldanswer,telbuf);
  }
 }
 return 0;
}
 

