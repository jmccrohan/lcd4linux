/* $Id: processor.c,v 1.31 2003/06/13 06:35:56 reinelt Exp $
 *
 * main data processing
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
 * $Log: processor.c,v $
 * Revision 1.31  2003/06/13 06:35:56  reinelt
 * added scrolling capability
 *
 * Revision 1.30  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.29  2003/02/05 04:31:38  reinelt
 *
 * T_EXEC: remove trailing CR/LF
 * T_EXEC: deactivated maxlen calculation (for I don't understand what it is for :-)
 *
 * Revision 1.28  2002/12/05 19:23:01  reinelt
 * fixed undefined operations found by gcc3
 *
 * Revision 1.27  2002/08/19 04:41:20  reinelt
 * introduced bar.c, moved bar stuff from display.h to bar.h
 *
 * Revision 1.26  2001/05/06 10:01:27  reinelt
 *
 * fixed a bug which prevented extendet tokens to be used for GPO's
 * many thanks to Carsten Nau!
 *
 * Revision 1.25  2001/03/17 11:11:31  ltoetsch
 * bugfix: max for BAR_T
 *
 * Revision 1.24  2001/03/16 16:40:17  ltoetsch
 * implemented time bar
 *
 * Revision 1.23  2001/03/16 09:28:08  ltoetsch
 * bugfixes
 *
 * Revision 1.22  2001/03/15 15:49:23  ltoetsch
 * fixed compile HD44780.c, cosmetics
 *
 * Revision 1.21  2001/03/15 09:47:13  reinelt
 *
 * some fixes to ppdef
 * off-by-one bug in processor.c fixed
 *
 * Revision 1.20  2001/03/14 13:19:29  ltoetsch
 * Added pop3/imap4 mail support
 *
 * Revision 1.19  2001/03/13 08:34:15  reinelt
 *
 * corrected a off-by-one bug with sensors
 *
 * Revision 1.18  2001/03/08 15:25:38  ltoetsch
 * improved exec
 *
 * Revision 1.17  2001/03/08 08:39:55  reinelt
 *
 * fixed two typos
 *
 * Revision 1.16  2001/03/07 18:10:21  ltoetsch
 * added e(x)ec commands
 *
 * Revision 1.15  2001/03/02 10:18:03  ltoetsch
 * added /proc/apm battery stat
 *
 * Revision 1.14  2001/02/19 00:15:46  reinelt
 *
 * integrated mail and seti client
 * major rewrite of parser and tokenizer to support double-byte tokens
 *
 * Revision 1.13  2001/02/16 14:15:11  reinelt
 *
 * fixed type in processor.c
 * GPO documentation update from Carsten
 *
 * Revision 1.12  2001/02/16 08:23:09  reinelt
 *
 * new token 'ic' (ISDN connected) by Carsten Nau <info@cnau.de>
 *
 * Revision 1.11  2001/02/14 07:40:16  reinelt
 *
 * first (incomplete) GPO implementation
 *
 * Revision 1.10  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.9  2001/02/11 23:34:07  reinelt
 *
 * fixed a small bug where the throughput of an offline ISDN connection 
 * is displayed as '----', but the online value is 5 chars long. 
 * corrected to ' ----'. Thanks to Carsten Nau <info@cnau.de> 
 *
 * Revision 1.8  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.7  2000/07/31 10:43:44  reinelt
 *
 * some changes to support kernel-2.4 (different layout of various files in /proc)
 *
 * Revision 1.6  2000/05/21 06:20:35  reinelt
 *
 * added ppp throughput
 * token is '%t[iomt]' at the moment, but this will change in the near future
 *
 * Revision 1.5  2000/04/15 11:56:35  reinelt
 *
 * more debug messages
 *
 * Revision 1.4  2000/04/15 11:13:54  reinelt
 *
 * added '-d' (debugging) switch
 * added several debugging messages
 * removed config entry 'Delay' for HD44780 driver
 * delay loop for HD44780 will be calibrated automatically
 *
 * Revision 1.3  2000/04/01 16:22:38  reinelt
 *
 * bug that caused a segfault in processor.c fixed (thanks to herp)
 *
 * Revision 1.2  2000/03/23 07:24:48  reinelt
 *
 * PPM driver up and running (but slow!)
 *
 * Revision 1.1  2000/03/22 07:33:50  reinelt
 *
 * FAQ added
 * new modules 'processor.c' contains all data processing
 *
 */

/* 
 * exported functions:
 *
 * void process_init (void);
 *   does all necessary initializations
 *
 * void process (int smooth);
 *   processes a whole screen
 *   bars will always be processed
 *   texts only if smooth=0
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include "debug.h"
#include "cfg.h"
#include "system.h"
#include "isdn.h"
#include "parser.h"
#include "display.h"
#include "bar.h"
#include "processor.h"
#include "mail.h"
#include "battery.h"
#include "seti.h"
#include "exec.h"

#define ROWS 64
#define GPOS 16

static char *row[ROWS+1];
static int   gpo[GPOS+1];
static int   rows, cols, xres, yres, supported_bars, gpos;
static int   lines, scroll, turn;
static int   token_usage[256]={0,};

static struct { int total, used, free, shared, buffer, cache, avail; } ram;
static struct { double load1, load2, load3, overload; } load;
static struct { double user, nice, system, idle; } busy;
static struct { int read, write, total, max, peak; } disk;
static struct { int rx, tx, total, max, peak, bytes; } net;
static struct { int usage, in, out, total, max, peak; } isdn;
static struct { int rx, tx, total, max, peak; } ppp;
static struct { int perc, stat; double dur; } batt;
static struct { double perc, cput; } seti;
static struct { int num, unseen;} mail[MAILBOXES+1];
static struct { double val, min, max; } sensor[SENSORS+1];

static double query (int token)
{
  switch (token&255) {
    
  case T_MEM_TOTAL:
    return ram.total;
  case T_MEM_USED:
    return ram.used;
  case T_MEM_FREE:
    return ram.free;
  case T_MEM_SHARED:
    return ram.shared;
  case T_MEM_BUFFER:
    return ram.buffer;
  case T_MEM_CACHE:
    return ram.cache;
  case T_MEM_AVAIL:
    return ram.avail;

  case T_LOAD_1:
    return load.load1;
  case T_LOAD_2:
    return load.load2;
  case T_LOAD_3:
    return load.load3;
    
  case T_CPU_USER:
    return busy.user;
  case T_CPU_NICE:
    return busy.nice;
  case T_CPU_SYSTEM:
    return busy.system;
  case T_CPU_BUSY:
    return 1.0-busy.idle;
  case T_CPU_IDLE:
    return busy.idle;
    
  case T_DISK_READ:
    return disk.read;
  case T_DISK_WRITE:
    return disk.write;
  case T_DISK_TOTAL:
    return disk.total;
  case T_DISK_MAX:
    return disk.max;
    
  case T_ETH_RX:
    return net.rx;
  case T_ETH_TX:
    return net.tx;
  case T_ETH_TOTAL:
    return net.total;
  case T_ETH_MAX:
    return net.max;
    
  case T_ISDN_IN:
    return isdn.in;
  case T_ISDN_OUT:
    return isdn.out;
  case T_ISDN_TOTAL:
    return isdn.total;
  case T_ISDN_MAX:
    return isdn.max;
  case T_ISDN_USED:
    return isdn.usage;

  case T_PPP_RX:
    return ppp.rx;
  case T_PPP_TX:
    return ppp.tx;
  case T_PPP_TOTAL:
    return ppp.total;
  case T_PPP_MAX:
    return ppp.max;
    
  case T_SETI_PRC:
    return seti.perc;
  case T_SETI_CPU:
    return seti.cput;
    
  case T_BATT_PERC:
    return batt.perc;
  case T_BATT_STAT:
    return batt.stat;
  case T_BATT_DUR:
    return batt.dur;
    
  case T_MAIL:
    return mail[(token>>8)-'0'].num;

  case T_MAIL_UNSEEN:
    return mail[(token>>8)-'0'].unseen;

  case T_SENSOR:
    return sensor[(token>>8)-'0'].val;

  case T_EXEC:
    return exec[(token>>8)-'0'].val;
  }
  return 0.0;
}

/* return a value 0..1 */
static double query_bar (int token)
{
  int i;
  double value=query(token);
  
  switch (token & 255) {

  case T_MEM_TOTAL:
  case T_MEM_USED:
  case T_MEM_FREE:
  case T_MEM_SHARED:
  case T_MEM_BUFFER:
  case T_MEM_CACHE:
  case T_MEM_AVAIL:
    return value/ram.total;

  case T_LOAD_1:
  case T_LOAD_2:
  case T_LOAD_3:
    return value/load.overload;
    
  case T_DISK_READ:
  case T_DISK_WRITE:
  case T_DISK_MAX:
    return value/disk.peak;
  case T_DISK_TOTAL:
    return value/disk.peak/2.0;
    
  case T_ETH_RX:
  case T_ETH_TX:
  case T_ETH_MAX:
    return value/net.peak;
  case T_ETH_TOTAL:
    return value/net.peak/2.0;
    
  case T_ISDN_IN:
  case T_ISDN_OUT:
  case T_ISDN_MAX:
    return value/isdn.peak;
  case T_ISDN_TOTAL:
    return value/isdn.peak/2.0;

  case T_PPP_RX:
  case T_PPP_TX:
  case T_PPP_MAX:
    return value/ppp.peak;
  case T_PPP_TOTAL:
    return value/ppp.peak/2.0;
    
  case T_SETI_PRC:
    return value;
    
  case T_BATT_PERC:
    {
      static int alarm;
      alarm=((alarm+1) % 3);
      if(value < atoi(cfg_get("battwarning","10")) && !alarm) /* flash bar */
	value = 0;
      return value/100;
    }
  case T_SENSOR:
    i=(token>>8)-'0';
    return (value-sensor[i].min)/(sensor[i].max-sensor[i].min);
  }
  return value;
}

static void print_token (int token, char **p, char *start, int maxlen)
{
  double val;
  int i;
  
  switch (token & 255) {
  case T_PERCENT:
    *(*p)++='%';
    break;
  case T_DOLLAR:
    *(*p)++='$';
    break;
  case T_OS:
    *p+=sprintf (*p, "%s", System());
    break;
  case T_RELEASE:
    *p+=sprintf (*p, "%s", Release());
    break;
  case T_CPU:
    *p+=sprintf (*p, "%s", Processor());
    break;
  case T_RAM:
    *p+=sprintf (*p, "%d", Memory());
    break;
  case T_OVERLOAD:
    *(*p)++=load.load1>load.overload?'!':' ';
    break;
  case T_MEM_TOTAL:
  case T_MEM_USED:
  case T_MEM_FREE:
  case T_MEM_SHARED:
  case T_MEM_BUFFER:
  case T_MEM_CACHE:
  case T_MEM_AVAIL:
    *p+=sprintf (*p, "%6.0f", query(token));
    break;
  case T_LOAD_1:
  case T_LOAD_2:
  case T_LOAD_3:
  case T_SENSOR:
    val=query(token);
    if (val<10.0)
      *p+=sprintf (*p, "%5.2f", val);
    else if (val<100.0)
      *p+=sprintf (*p, "%5.1f", val);
    else
      *p+=sprintf (*p, "%5.0f", val);
    break;
  case T_CPU_USER:
  case T_CPU_NICE:
  case T_CPU_SYSTEM:
  case T_CPU_BUSY:
  case T_CPU_IDLE:
    *p+=sprintf (*p, "%3.0f", 100.0*query(token));
    break;
  case T_ETH_RX:
  case T_ETH_TX:
  case T_ETH_MAX:
  case T_ETH_TOTAL:
    val=query(token);
    if (net.bytes)
      val/=1024.0;
    *p+=sprintf (*p, "%5.0f", val);
    break;
  case T_ISDN_IN:
  case T_ISDN_OUT:
  case T_ISDN_MAX:
  case T_ISDN_TOTAL:
    if (isdn.usage)
      *p+=sprintf (*p, "%5.0f", query(token));
    else
      *p+=sprintf (*p, " ----");
    break;
  case T_ISDN_USED:
    if (isdn.usage)
      *p+=sprintf (*p, "*");
    else
      *p+=sprintf (*p, " ");
    break;
  case T_SETI_PRC:
    val=100.0*query(token);
    if (val<100.0) 
      *p+=sprintf (*p, "%4.1f", val);
    else
      *p+=sprintf (*p, " 100");
    break;
  case T_SETI_CPU:
    val=query(token);
    *p+=sprintf (*p, "%2d.%2.2d:%2.2d", (int)val/86400, 
		 (int)((int)val%86400)/3600,
		 (int)(((int)val%86400)%3600)/60 );
    break;
    
  case T_BATT_PERC:
    *p+=sprintf(*p, "%3.0f", query(token));
    break;
  case T_BATT_STAT:  
    { int ival = (int) query(token);
      switch (ival) {
        case 0: **p = '='; break;
        case 1: **p = '+'; break;
        case 2: **p = '-'; break;
        default: **p = '?'; break;
      }
    }
    (*p)++;
    break;
  case T_BATT_DUR:
    {
      char eh = 's';
      val = query(token);
      if (val > 99) {
	val /= 60;
	eh = 'm';
      }
      if (val > 99) {
	val /= 60;
	eh = 'h';
      }
      if (val > 99) {
	val /= 24;
	eh = 'd';
      }
      *p+=sprintf(*p, "%2.0f%c", val, eh);
    }
    break;

  case T_EXEC:
    i = (token>>8)-'0';
    *p+=sprintf (*p, "%.*s",cols-(*p-start), exec[i].s);
#if 0
    // Fixme: this does not really work as it should...
    // Remove param 'maxlen' sometimes..
    for (i=*p-start; i<cols && maxlen--; i++) /* clear right of text */
      *(*p)++=' ';
#endif
    break;
    
  default:
      *p+=sprintf (*p, "%5.0f", query(token));
  }
}

static void collect_data (void) 
{
  int i;

  if (token_usage[C_MEM]) {
    Ram (&ram.total, &ram.free, &ram.shared, &ram.buffer, &ram.cache); 
    ram.used=ram.total-ram.free;
    ram.avail=ram.free+ram.buffer+ram.cache;
  }
  
  if (token_usage[C_LOAD]) {
    Load (&load.load1, &load.load2, &load.load3);
  }
  
  if (token_usage[C_CPU]) {
    Busy (&busy.user, &busy.nice, &busy.system, &busy.idle);
  }
  
  if (token_usage[C_DISK]) {
    Disk (&disk.read, &disk.write);
    disk.total=disk.read+disk.write;
    disk.max=disk.read>disk.write?disk.read:disk.write;
    if (disk.max>disk.peak) disk.peak=disk.max;
  }
  
  if (token_usage[C_ETH]) {
    Net (&net.rx, &net.tx, &net.bytes);
    net.total=net.rx+net.tx;
    net.max=net.rx>net.tx?net.rx:net.tx;
    if (net.max>net.peak) net.peak=net.max;
  }

  if (token_usage[C_ISDN]) {
    Isdn (&isdn.in, &isdn.out, &isdn.usage);
    isdn.total=isdn.in+isdn.out;
    isdn.max=isdn.in>isdn.out?isdn.in:isdn.out;
    if (isdn.max>isdn.peak) isdn.peak=isdn.max;
  }

  if (token_usage[C_PPP]) {
    PPP (0, &ppp.rx, &ppp.tx);
    ppp.total=ppp.rx+ppp.tx;
    ppp.max=ppp.rx>ppp.tx?ppp.rx:ppp.tx;
    if (ppp.max>ppp.peak) ppp.peak=ppp.max;
  }

  if (token_usage[C_SETI]) {
    Seti (&seti.perc, &seti.cput);
  }

  if (token_usage[C_BATT]) {
    Battery (&batt.perc, &batt.stat, &batt.dur);
  }
  
  for (i=0; i<=MAILBOXES; i++) {
    if (token_usage[T_MAIL]&(1<<i) || token_usage[T_MAIL_UNSEEN]&(1<<i) ) {
      Mail (i, &mail[i].num, &mail[i].unseen);
    }
  }
  
  for (i=0; i<=SENSORS; i++) {
    if (token_usage[T_SENSOR]&(1<<i)) {
      Sensor (i, &sensor[i].val, &sensor[i].min, &sensor[i].max);
    }
  }

  for (i=0; i<=EXECS; i++) {
    if (token_usage[T_EXEC]&(1<<i)) {
      Exec (i, exec[i].s, &exec[i].val);
    }
  }
}

static char *process_row (int r)
{
  static char buffer[256];
  char *s=row[r];
  char *p=buffer;
  int token;
  int len;
  char *q;
  
  do {
    if (*s=='%') {
      token = *(unsigned char*)++s;
      if (token>T_EXTENDED) token += (*(unsigned char*)++s)<<8;
#if 0
      // Fixme: I don't understand this one...
      if (!s[1]) 
	len = cols - (s - row[r] - 1);
      else
        for (q = s+1, len=0; *q && isspace(*q); q++)
  	  len++;
#endif
      print_token (token, &p, buffer, len);
	
    } else if (*s=='$') {
      double val1, val2;
      int i, type, len;
      type=*++s;
      len=*++s;
      token = *(unsigned char*)++s;
      if (token>T_EXTENDED) token += (*(unsigned char*)++s)<<8;
      val1=query_bar(token);
      val2=val1;
      if (type & (BAR_H2 | BAR_V2)) {
	token = *(unsigned char*)++s;
	if (token>T_EXTENDED) token += (*(unsigned char*)++s)<<8;
	val2=query_bar(token);
      }
      else if (type & BAR_T)
	val2 = *(unsigned char*)++s; /* width */
      if (type & BAR_H)
	lcd_bar (type, r, p-buffer+1, len*xres, val1*len*xres, val2*len*xres);
      else if (type & BAR_T)
	lcd_bar (type, r, p-buffer+1, len*yres, val1*len*yres, val2*xres);
      else
	lcd_bar (type, r, p-buffer+1, len*yres, val1*len*yres, val2*len*yres);
      
      if (type & BAR_H) {
	for (i=0; i<len && p-buffer<cols; i++)
	  *p++='\t';
      } else {
	*p++='\t';
      }
      
    } else {
      *p++=*s;
    }
    
  } while (*s++); 
    
  return buffer;
}

static int process_gpo (int r)
{
  int token;
  double val;

  token=gpo[r];
  val=query(token);

  return (val > 0.0);
}

static int Turn (void)
{
  struct timeval now;
  static struct timeval old = {tv_sec:0, tv_usec:0};
  static struct timeval new = {tv_sec:0, tv_usec:0};
  
  if (turn<=0) return 0;
  
  gettimeofday (&now, NULL);
  if (now.tv_sec==new.tv_sec ? now.tv_usec>new.tv_usec : now.tv_sec>new.tv_sec) {
    old=now;
    new.tv_sec =old.tv_sec;
    new.tv_usec=old.tv_usec+turn*1000;
    while (new.tv_usec>=1000000) {
      new.tv_usec-=1000000;
      new.tv_sec++;
    }
    return 1;
  }
  return 0;
}

void process_init (void)
{
  int i;

  load.overload=atof(cfg_get("overload","2.0"));


  lcd_query (&rows, &cols, &xres, &yres, &supported_bars, &gpos);
  if (rows>ROWS) {
    error ("%d rows exceeds limit, reducing to %d rows", rows, ROWS);
    rows=ROWS;
  }
  if (gpos>GPOS) {
    error ("%d gpos exceeds limit, reducing to %d gpos", gpos, GPOS);
    gpos=GPOS;
  }
  debug ("Display: %d rows, %d columns, %dx%d pixels, %d GPOs", rows, cols, xres, yres, gpos);


  lines=atoi(cfg_get("Rows","1"));
  if (lines<1) {
    error ("bad 'Rows' entry in %s, ignoring.", cfg_file());
    lines=1;
  }
  if (lines>ROWS) {
    error ("%d virtual rows exceeds limit, reducing to %d rows", lines, ROWS);
    lines=ROWS;
  }
  if (lines>rows) {
    scroll=atoi(cfg_get("Scroll","1"));
    if (scroll<1) {
      error ("bad 'Scroll' entry in %s, ignoring and using '1'", cfg_file());
      scroll=1;
    }
    if (scroll>rows) {
      error ("'Scroll' entry in %s is %d, > %d display rows.", cfg_file(), scroll, rows);
      error ("This may lead to unexpected results!");
    }
    turn=atoi(cfg_get("Turn","1000"));
    if (turn<1) {
      error ("bad 'Turn' entry in %s, ignoring and using '1000'", cfg_file());
      turn=1;
    }
    debug ("Virtual: %d rows, scroll %d lines every %d msec", lines, scroll, turn);
  } else {
    lines=rows;
    scroll=0;
    turn=0;
  }


  for (i=1; i<=lines; i++) {
    char buffer[8], *p;
    snprintf (buffer, sizeof(buffer), "Row%d", i);
    p=cfg_get(buffer,"");
    debug ("%s: %s", buffer, p);
    row[i]=strdup(parse_row(p, supported_bars, token_usage));
  }


  for (i=1; i<=gpos; i++) {
    char buffer[8], *p;
    snprintf (buffer, sizeof(buffer), "GPO%d", i);
    p=cfg_get(buffer,"");
    debug ("%s: %s", buffer, p);
    gpo[i]=parse_gpo(p, token_usage);
  }
}

void process (int smooth)
{
  int i, j, val;
  char *txt;
  static int offset=0;
  
  collect_data();

  if (Turn()) {
    offset+=scroll;
    while (offset>=lines) {
      offset-=lines;
    }
    // Fixme: this is ugly!
    smooth=1;
    lcd_clear();
  }
  
  for (i=1; i<=rows; i++) {
    j=i+offset;
    while (j>lines) {
      j-=lines;
    }
    txt=process_row (j);
    if (smooth==0)
      lcd_put (i, 1, txt);
  }
  for (i=1; i<=gpos; i++) {
    val=process_gpo (i);
    lcd_gpo (i, val);
  }
  lcd_flush();
}
