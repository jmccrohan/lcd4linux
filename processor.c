/* $Id: processor.c,v 1.1 2000/03/22 07:33:50 reinelt Exp $
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

#include "cfg.h"
#include "system.h"
#include "isdn.h"
#include "parser.h"
#include "display.h"
#include "processor.h"


#define ROWS 16

char *row[ROWS];
int   rows, cols, xres, yres, supported_bars;
int   token_usage[256]={0,};

struct { int total, used, free, shared, buffer, cache, avail; } ram;
struct { double load1, load2, load3, overload; } load;
struct { double user, nice, system, idle; } busy;
struct { int read, write, total, max, peak; } disk;
struct { int rx, tx, total, max, peak; } net;
struct { int usage, in, out, total, max, peak; } isdn;
struct { double val, min, max; } sensor[SENSORS];


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
    return sensor[token-T_SENSOR_1+1].val;
  }
  return 0.0;
}

static double query_bar (int token)
{
  int i;
  double value=query(token);
  
  switch (token) {

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

  case T_SENSOR_1:
  case T_SENSOR_2:
  case T_SENSOR_3:
  case T_SENSOR_4:
  case T_SENSOR_5:
  case T_SENSOR_6:
  case T_SENSOR_7:
  case T_SENSOR_8:
  case T_SENSOR_9:
    i=token-T_SENSOR_1+1;
    return (value-sensor[i].min)/(sensor[i].max-sensor[i].min);
  }
  return value;
}

static void print_token (int token, char **p)
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
  case T_MEM_AVAIL:
    *p+=sprintf (*p, "%6.0f", query(token));
    break;
  case T_LOAD_1:
  case T_LOAD_2:
  case T_LOAD_3:
  case T_SENSOR_1:
  case T_SENSOR_2:
  case T_SENSOR_3:
  case T_SENSOR_4:
  case T_SENSOR_5:
  case T_SENSOR_6:
  case T_SENSOR_7:
  case T_SENSOR_8:
  case T_SENSOR_9:
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
  
  if (token_usage[C_NET]) {
    Net (&net.rx, &net.tx);
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

  for (i=1; i<SENSORS; i++) {
    if (token_usage[T_SENSOR_1+i-1]) {
      Sensor (i, &sensor[i].val, &sensor[i].min, &sensor[i].max);
    }
  }
}

static char *process_row (int r)
{
  static char buffer[256];
  char *s=row[r];
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

void process_init (void)
{
  int i;
  char buffer[8];

  load.overload=atof(cfg_get("overload"));
  lcd_query (&rows, &cols, &xres, &yres, &supported_bars);

  for (i=1; i<=rows; i++) {
    snprintf (buffer, sizeof(buffer), "row%d", i);
    row[i]=strdup(parse(cfg_get(buffer), supported_bars, token_usage));
  }
}

void process (int smooth)
{
  int i;
  char *txt;

  collect_data();
  for (i=1; i<=rows; i++) {
    txt=process_row (i);
    if (smooth==0)
      lcd_put (i, 1, txt);
  }
  lcd_flush();
}
