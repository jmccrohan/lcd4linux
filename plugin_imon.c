/* $Id: plugin_imon.c,v 1.6 2004/03/13 14:58:15 nicowallmeier Exp $
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
 * $Log: plugin_imon.c,v $
 * Revision 1.6  2004/03/13 14:58:15  nicowallmeier
 * Added clean termination of imond-connection (now correctly)
 *
 * Revision 1.4  2004/03/03 04:44:16  reinelt
 * changes (cosmetics?) to the big patch from Martin
 * hash patch un-applied
 *
 * Revision 1.3  2004/03/03 03:47:04  reinelt
 * big patch from Martin Hejl:
 * - use qprintf() where appropriate
 * - save CPU cycles on gettimeofday()
 * - add quit() functions to free allocated memory
 * - fixed lots of memory leaks
 *
 * Revision 1.2  2004/02/22 17:35:41  reinelt
 * some fixes for generic graphic driver and T6963
 * removed ^M from plugin_imon (Nico, are you editing under Windows?)
 *
 * Revision 1.1  2004/02/18 14:45:43  nicowallmeier
 * Imon/Telmon plugin ported
 *
 */
 
#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "qprintf.h"
#include "cfg.h"
#include "hash.h"

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


static HASH TELMON = { 0, };
static HASH IMON = { 0, };

static char thost[256];
static int tport;
static char phoneb[256];

static char ihost[256];
static char ipass[256];
static int iport;

static int fd=0;
static int err=0;

/*----------------------------------------------------------------------------
 *  service_connect (host_name, port)       - connect to tcp-service
 *----------------------------------------------------------------------------
 */
static int service_connect (char * host_name, int port){
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

static void phonebook(char *number){
  FILE *  fp;
  char line[256];
  
  fp = fopen (phoneb, "r");
  
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


static int parse_telmon(){
 static int telmond_fd=-2;
 static char oldanswer[128];
 int age;
   
 // reread every 1 sec only
 age=hash_age(&TELMON, NULL, NULL);
 if (age>0 && age<=1000) return 0;

 if (telmond_fd != -1){
  char    telbuf[128];

  telmond_fd = service_connect (thost, tport);
  if (telmond_fd >= 0){
   int l = read (telmond_fd, telbuf, 127);
   if ((l > 0) && (strcmp(telbuf,oldanswer))){
	char date[11]; 
	char time[11];
	char number[256];
	char msn[256];
	sscanf(telbuf,"%s %s %s %s",date,time,number,msn);
	hash_set (&TELMON, "time", time);
	date[4]='\0';
	date[7]='\0';
	qprintf(time, sizeof(time), "%s.%s.%s",date+8,date+5,date);
	hash_set (&TELMON, "number", number);
	hash_set (&TELMON, "msn", msn);
	hash_set (&TELMON, "date", time);
	phonebook(number);
	phonebook(msn);
	hash_set (&TELMON, "name", number);
	hash_set (&TELMON, "msnname", msn);
   }
   close (telmond_fd);
   strcpy(oldanswer,telbuf);
  }
 }
 return 0;
}

static void my_telmon (RESULT *result, RESULT *arg1){
 char *val=NULL;
 if (parse_telmon()<0) {
  SetResult(&result, R_STRING, ""); 
  return;
 }
  
 val=hash_get(&TELMON, R2S(arg1));
 if (val==NULL) val="";
 SetResult(&result, R_STRING, val); 
}

void init(){
 if (fd!=0) return;
 
 fd=service_connect(ihost,iport);

 if (fd<0){
  err++;
 } else if ((ipass!=NULL) && (*ipass!='\0')) { // Passwort senden
  char buf[40];
  qprintf(buf,sizeof(buf),"pass %s",ipass);
  send_command(fd,buf);
  get_answer(fd); 
 }
}

static int parse_imon(char *cmd){
 // reread every half sec only
 int age=hash_age(&IMON, cmd, NULL);
 if (age>0 && age<=500) return 0;

 init(); // establish connection

 if (err) return -1;
 
 hash_set (&IMON, cmd , get_value(cmd));
 
 return 0;
}

static void my_imon_version (RESULT *result){
 char *val;
 // read only ones
 int age=hash_age(&IMON, "version", NULL);
 if (age<0){
  char *s;
  init();
  if (err){
   SetResult(&result, R_STRING, ""); 
   return;
  }
  s=get_value("version");
  for (;;){ // interne Versionsnummer killen
   if (s[0]==' '){
    s=s+1;
    break;
   }
   s=s+1;		
  }
  hash_set (&IMON, "version", s);
 }
	
 val=hash_get(&IMON, "version");
 if (val==NULL) val="";
 SetResult(&result, R_STRING, val); 
}

static int parse_imon_rates(char *channel){
 char buf[128],in[25],out[25];
 char *s;
 int age;
  
 qprintf(buf,sizeof(buf),"rate %s in",channel);
 
 // reread every half sec only
 age=hash_age(&IMON, buf, NULL);
 if (age>0 && age<=500) return 0;

 init(); // establish connection

 if (err) return -1;

 qprintf(buf, sizeof(buf), "rate %s", channel);
 s=get_value(buf);
 
 if (sscanf(s,"%s %s",in, out)!=2) return -1;

 qprintf(buf, sizeof(buf), "rate %s in", channel);
 hash_set (&IMON, buf , in);
 qprintf(buf, sizeof(buf), "rate %s out", channel);
 hash_set (&IMON, buf , out);
 
 return 0;
}


static void my_imon_rates (RESULT *result, RESULT *arg1, RESULT *arg2){
 char *val;
 char buf[128];
 
 if (parse_imon_rates(R2S(arg1))<0) {
  SetResult(&result, R_STRING, ""); 
  return;
 }
 
 qprintf(buf,sizeof(buf),"rate %s %s",R2S(arg1),R2S(arg2));

 val=hash_get(&IMON, buf);
 if (val==NULL) val="";
 SetResult(&result, R_STRING, val); 
}

static void my_imon (RESULT *result, RESULT *arg1){
 char *val=NULL,*cmd=R2S(arg1);
 
 if (parse_imon(cmd)<0) {
  SetResult(&result, R_STRING, ""); 
  return;
 }
  
 val=hash_get(&IMON, cmd);
 if (val==NULL) val="";
 SetResult(&result, R_STRING, val); 
}

int plugin_init_imon (void){
 char telmon='\1',imon='\1';	

 char *s=cfg_get ("Telmon", "Host","127.0.0.1");
 if (*s=='\0') {
  error ("[Telmon] no 'Host' entry in %s", cfg_source());
  telmon='\0';
 } 
 strcpy(thost,s);
 free(s);
  
 if ((telmon=='\01') && (cfg_number("Telmon", "Port",5001,1,65536,&tport)<0)){
  error ("[Telmon] no valid port definition");
  telmon='\0';	
 }
 
 s=cfg_get ("Telmon", "Phonebook","/etc/phonebook");
 strcpy(phoneb,s);
 free(s);
 
 s=cfg_get ("Imon", "Host", "127.0.0.1");
 if (*s=='\0') {
  error ("[Imon] no 'Host' entry in %s", cfg_source());
  imon='\0';
 }
 strcpy(ihost,s); 
 free(s);
 
 if ((imon=='\01') && (cfg_number("Imon", "Port",5000,1,65536,&iport)<0)){
  error ("[Imon] no valid port definition");
  imon='\0';	
 }
 
 s=cfg_get ("Imon", "Pass", "");
 strcpy(ipass,s);
 free(s);
	
 if (telmon=='\1') AddFunction ("telmon", 1, my_telmon);
 if (imon=='\1'){
   AddFunction ("imon", 1, my_imon);
   AddFunction ("version", 0, my_imon_version);
   AddFunction ("rates", 2, my_imon_rates);
 }

 return 0;
}

void plugin_exit_imon(void){
  if (fd>0){
   send_command(fd,"quit");
   close(fd);
  }
  hash_destroy(&TELMON);
  hash_destroy(&IMON);
}
