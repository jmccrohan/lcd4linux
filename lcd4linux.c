/* $Id: lcd4linux.c,v 1.5 2000/03/18 08:07:04 reinelt Exp $
 *
 * LCD4Linux
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
 * $Log: lcd4linux.c,v $
 * Revision 1.5  2000/03/18 08:07:04  reinelt
 *
 * vertical bars implemented
 * bar compaction improved
 * memory information implemented
 *
 * Revision 1.4  2000/03/17 09:21:42  reinelt
 *
 * various memory statistics added
 *
 * Revision 1.3  2000/03/13 15:58:24  reinelt
 *
 * release 0.9
 * moved row parsing to parser.c
 * all basic work finished
 *
 * Revision 1.2  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "cfg.h"
#include "display.h"
#include "parser.h"
#include "system.h"
#include "isdn.h"

#define ROWS 16

double overload;
int tick, tack, tau;
int rows, cols, xres, yres, supported_bars;

struct { int total, used, free, shared, buffer, cache, apps; } ram;
struct { double load1, load2, load3, overload; } load;
struct { double user, nice, system, idle; } busy;
struct { int read, write, total, max, peak; } disk;
struct { int rx, tx, total, max, peak; } net;
struct { int usage, in, out, total, max, peak; } isdn;

static void usage(void)
{
  printf ("LCD4Linux V" VERSION " (c) 2000 Michael Reinelt <reinelt@eunet.at>");
  printf ("usage: lcd4linux [config file]\n");
}

static void collect_data (void) 
{
  Ram (&ram.total, &ram.free, &ram.shared, &ram.buffer, &ram.cache); 
  ram.used=ram.total-ram.free;
  ram.apps=ram.used-ram.buffer-ram.cache;

  Load (&load.load1, &load.load2, &load.load3);
  Busy (&busy.user, &busy.nice, &busy.system, &busy.idle);

  Disk (&disk.read, &disk.write);
  disk.total=disk.read+disk.write;
  disk.max=disk.read>disk.write?disk.read:disk.write;
  if (disk.max>disk.peak) disk.peak=disk.max;

  Net (&net.rx, &net.tx);
  net.total=net.rx+net.tx;
  net.max=net.rx>net.tx?net.rx:net.tx;
  if (net.max>net.peak) net.peak=net.max;

  Isdn (&isdn.in, &isdn.out, &isdn.usage);
  isdn.total=isdn.in+isdn.out;
  isdn.max=isdn.in>isdn.out?isdn.in:isdn.out;
  if (isdn.max>isdn.peak) isdn.peak=isdn.max;
}

static double query (int token)
{
  switch (token) {
    
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
  case T_MEM_APP:
    return ram.apps;

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
    
  case T_NET_RX:
    return net.rx;
  case T_NET_TX:
    return net.tx;
  case T_NET_TOTAL:
    return net.total;
  case T_NET_MAX:
    return net.max;
    
  case T_ISDN_IN:
    return isdn.in;
  case T_ISDN_OUT:
    return isdn.out;
  case T_ISDN_TOTAL:
    return isdn.total;
  case T_ISDN_MAX:
    return isdn.max;

  case T_SENSOR_1:
  case T_SENSOR_2:
  case T_SENSOR_3:
  case T_SENSOR_4:
  case T_SENSOR_5:
  case T_SENSOR_6:
  case T_SENSOR_7:
  case T_SENSOR_8:
  case T_SENSOR_9:
  }
  return 0.0;
}

static double query_bar (int token)
{
  double value=query(token);
  
  switch (token) {

  case T_MEM_TOTAL:
  case T_MEM_USED:
  case T_MEM_FREE:
  case T_MEM_SHARED:
  case T_MEM_BUFFER:
  case T_MEM_CACHE:
  case T_MEM_APP:
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
    
  case T_NET_RX:
  case T_NET_TX:
  case T_NET_MAX:
    return value/net.peak;
  case T_NET_TOTAL:
    return value/net.peak/2.0;
    
  case T_ISDN_IN:
  case T_ISDN_OUT:
  case T_ISDN_MAX:
    return value/isdn.peak;
  case T_ISDN_TOTAL:
    return value/isdn.peak/2.0;

  }
  return value;
}

void print_token (int token, char **p)
{
  double val;
  
  switch (token) {
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
  case T_MEM_APP:
    *p+=sprintf (*p, "%6.0f", query(token));
    break;
  case T_LOAD_1:
  case T_LOAD_2:
  case T_LOAD_3:
    val=query(token);
    if (val<10.0) {
      *p+=sprintf (*p, "%4.2f", val);
    } else if (val<100.0) {
      *p+=sprintf (*p, "%4.1f", val);
    } else {
      *p+=sprintf (*p, "%4.0f", val);
    }
    break;
  case T_CPU_USER:
  case T_CPU_NICE:
  case T_CPU_SYSTEM:
  case T_CPU_BUSY:
  case T_CPU_IDLE:
    *p+=sprintf (*p, "%3.0f", 100.0*query(token));
    break;
  case T_ISDN_IN:
  case T_ISDN_OUT:
  case T_ISDN_MAX:
  case T_ISDN_TOTAL:
    if (isdn.usage)
      *p+=sprintf (*p, "%4.0f", query(token));
    else
      *p+=sprintf (*p, "----");
    break;
  default:
      *p+=sprintf (*p, "%4.0f", query(token));
  }
}

char *process_row (int r, char *s)
{
  static char buffer[256];
  char *p=buffer;
  
  do {
    if (*s=='%') {
      print_token (*(unsigned char*)++s, &p);
	
    } else if (*s=='$') {
      int i;
      int type=*++s;
      int len=*++s;
      double val1=query_bar(*(unsigned char*)++s);
      double val2=val1;
      if (type & (BAR_H2 | BAR_V2))
	val2=query_bar(*(unsigned char*)++s);
      if (type & BAR_H)
	lcd_bar (type, r, p-buffer+1, len*xres, val1*len*xres, val2*len*xres);
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


void main (int argc, char *argv[])
{
  char *cfg="/etc/lcd4linux.conf";
  char *display;
  char *row[ROWS];
  int i, smooth;

  if (argc>2) {
    usage();
    exit (2);
  }
  if (argc==2) {
    cfg=argv[1];
  }

  cfg_set ("row1", "*** %o %r ***");
  cfg_set ("row2", "%p CPU  %m MB RAM");
  cfg_set ("row3", "Busy %cu%% $r10cu");
  cfg_set ("row4", "Load %l1%L$r10l1");

  cfg_set ("tick", "100");
  cfg_set ("tack", "500");
  cfg_set ("tau", "500");
  
  cfg_set ("fifo", "/var/run/LCD4Linux");
  cfg_set ("overload", "2.0");

  if (cfg_read (cfg)==-1)
    exit (1);
  
  display=cfg_get("display");
  if (display==NULL || *display=='\0') {
    fprintf (stderr, "%s: missing 'display' entry!\n", cfg_file());
    exit (1);
  }
  if (lcd_init(display)==-1) {
    exit (1);
  }
  lcd_query (&rows, &cols, &xres, &yres, &supported_bars);

  tick=atoi(cfg_get("tick"));
  tack=atoi(cfg_get("tack"));
  tau=atoi(cfg_get("tau"));

  load.overload=atof(cfg_get("overload"));

  for (i=1; i<=rows; i++) {
    char buffer[8];
    snprintf (buffer, sizeof(buffer), "row%d", i);
    row[i]=strdup(parse(cfg_get(buffer), supported_bars));
  }
  
  lcd_clear();
  lcd_put (1, 1, "** LCD4Linux V" VERSION " **");
  lcd_put (2, 1, " (c) 2000 M.Reinelt");
  lcd_flush();
  
  sleep (3);
  lcd_clear();

  smooth=0;
  while (1) {
    collect_data();
    for (i=1; i<=rows; i++) {
      row[0]=process_row (i, row[i]);
      if (smooth==0)
	lcd_put (i, 1, row[0]);
    }
    lcd_flush();
    smooth+=tick;
    if (smooth>tack) smooth=0;
    usleep(tick*1000);
  }
}
