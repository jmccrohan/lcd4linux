/* $Id: lcd4linux.c,v 1.2 2000/03/10 17:36:02 reinelt Exp $
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
 * Revision 1.2  2000/03/10 17:36:02  reinelt
 *
 * first unstable but running release
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include "cfg.h"
#include "display.h"
#include "system.h"
#include "isdn.h"

#define ROWS 16

double overload;
int tick, tack, tau;
int rows, cols, xres, yres, bars;
char *row[ROWS];

void usage(void)
{
  printf ("LCD4Linux " VERSION " (c) 2000 Michael Reinelt <reinelt@eunet.at>");
  printf ("usage: lcd4linux [configuration]\n");
}

int bar_type (char tag)
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

int strpos (char *s, int c)
{
  int i;
  char *p;
  for (p=s, i=0; *p; p++, i++) {
    if (*p==c) return i;
  }
  return -1;
}
  
int print4f (char *p, double val)
{
  if (val<10.0) {
    return sprintf (p, "%4.2f", val);
  } else if (val<100.0) {
    return sprintf (p, "%4.1f", val);
  } else {
    return sprintf (p, "%4.0f", val);
  }
}

char *parse (char *string)
{
  int pos;
  static char buffer[256];
  char *s=string;
  char *p=buffer;

  do {
    if (*s=='%') {
      if (strchr("orpmlLcdDnNiIs%", *++s)==NULL) {
	fprintf (stderr, "WARNING: unknown format <%%%c> in <%s>\n", *s, string);
	continue;
      } 
      *p='%';
      *(p+1)=*s;
      switch (*s) {
      case 'l':
	pos=strpos("123", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown Load tag <%%l%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=pos+1;
	p+=3;
	break;
      case 'c':
	pos=strpos("unsi", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown CPU tag <%%c%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=pos+1;
	p+=3;
	break;
      case 'd':
	pos=strpos("rwtm", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown disk tag <%%d%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=pos+1;
	p+=3;
	break;
      case 'n':
	pos=strpos("iotm", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown net tag <%%n%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=pos+1;
	p+=3;
	break;
      case 'i':
	pos=strpos("iotm", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown ISDN tag <%%i%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=pos+1;
	p+=3;
	break;
      case 's':
	pos=strpos("123456789", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown sensor <%%s%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=pos+1;
	p+=3;
	break;
      default:
	p+=2;
      }
      
    } else if (*s=='$') {
      char dir;
      int type;
      int len=0;
      dir=*++s;
      if (dir=='$') {
	*p++='$';
	*p++='$';
	continue;
      }
      if (strchr("lrud", tolower(dir))==NULL) {
	fprintf (stderr, "invalid bar direction '%c' in <%s>\n", dir, string);
	continue;
      }
      type=bar_type(tolower(dir));
      if (type==0) {
	fprintf (stderr, "driver does not support bar type '%c'\n", dir);
	continue;
      }
      if (isdigit(*++s)) len=strtol(s, &s, 10);
      if (len<1 || len>255) {
	fprintf (stderr, "invalid bar length in <%s>\n", string);
	continue;
      }
      *p='$';
      *(p+1)=isupper(dir)?-type:type;
      *(p+2)=len;
      *(p+3)=*s;
      switch (*s) {
      case 'l':
	pos=strpos("123", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown Load tag <$l%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+4)=pos+1;
	p+=5;
	break;
      case 'c':
	pos=strpos("unsi", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown CPU tag <$d%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+4)=pos+1;
	p+=5;
	break;
      case 'd':
	pos=strpos("rwm", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown disk tag <$d%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+4)=pos+1;
	p+=5;
	break;
      case 'n':
	pos=strpos("iom", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown net tag <$n%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+4)=pos+1;
	p+=5;
	break;
      case 'i':
	pos=strpos("iom", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown ISDN tag <$i%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+4)=pos+1;
	p+=5;
	break;
      case 's':
	pos=strpos("123456789", *++s);
	if (pos<0) {
	  fprintf (stderr, "WARNING: unknown sensor <$s%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+4)=pos+1;
	p+=5;
	break;
      default:
	fprintf (stderr, "WARNING: unknown bar format <$%c> in <%s>\n", *s, string);
	p+=4;
      }
      
    } else if (*s=='\\') {
      unsigned int c=0; int n;
      if (*(s+1)=='\\') {
	*p++='\\';
	s+=2;
      } else {
	sscanf (s+1, "%3o%n", &c, &n);
	if (c==0 || c>255) {
	  fprintf (stderr, "WARNING: illegal '\\' in <%s> <%s>\n", string, s);
	  continue;
	}
	*p++=c;
	s+=n;
      }
      
    } else {
      *p++=*s;
    }
    
  } while (*s++);
  
  return buffer;
}


void draw (int smooth)
{
  double load[3];
  double busy[4];
  int disk[4]; static int disk_peak=1;
  int net[4]; static int net_peak=1;
  int isdn[4]; int isdn_usage; static int isdn_peak=1;
  char buffer[256];
  double val;
  int i, r;
  
  Busy (&busy[0], &busy[1], &busy[2], &busy[3]);
  Load (&load[0], &load[1], &load[2]);

  Disk (&disk[0], &disk[1]);
  disk[2]=disk[0]+disk[1];
  disk[3]=disk[0]>disk[1]?disk[0]:disk[1];
  if (disk[3]>disk_peak) disk_peak=disk[3];

  Net (&net[0], &net[1]);
  net[2]=net[0]+net[1];
  net[3]=net[0]>net[1]?net[0]:net[1];
  if (net[3]>net_peak) net_peak=net[3];

  Isdn (&isdn[0], &isdn[1], &isdn_usage);
  isdn[2]=isdn[0]+isdn[1];
  isdn[3]=isdn[0]>isdn[1]?isdn[0]:isdn[1];
  if (isdn[3]>isdn_peak) isdn_peak=isdn[3];
  
  for (r=1; r<=rows; r++) {
    char *s=row[r];
    char *p=buffer;
    do {
      if (*s=='%') {
	switch (*++s) {
	case '%':
	  *p++='%';
	  break;
	case 'o':
	  p+=sprintf (p, "%s", System());
	  break;
	case 'r':
	  p+=sprintf (p, "%s", Release());
	  break;
	case 'p':
	  p+=sprintf (p, "%s", Processor());
	  break;
	case 'm':
	  p+=sprintf (p, "%d", Memory());
	  break;
	case 'l':
	  p+=print4f (p, load[*++s-1]);
	  break;
	case 'L':
	  *p++=load[0]>overload?'!':' ';
	  break;
	case 'c':
	  p+=sprintf (p, "%3.0f", 100.0*busy[*++s-1]);
	  break;
	case 'd':
	  p+=print4f (p, disk[*++s-1]);
	  break;
	case 'D':
	  if (disk[0]+disk[1]==0)
	    *p++=' ';
	  else
	    *p++=disk[0]>disk[1]?'\176':'\177';
	  break;
	case 'n':
	  p+=print4f (p, net[*++s-1]/1024.0);
	  break;
	case 'N':
	  if (net[0]+net[1]==0)
	    *p++=' ';
	  else
	    *p++=net[0]>net[1]?'\176':'\177';
	  break;
	case 'i':
	  if (isdn_usage) {
	    p+=print4f (p, isdn[*++s-1]/1024.0);
	  } else {
	    p+=sprintf (p, "----");
	    s++;
	  }
	  break;
	case 'I':
	  if (isdn[0]+isdn[1]==0)
	    *p++=' ';
	  else
	    *p++=isdn[0]>isdn[1]?'\176':'\177';
	  break;
	}
	
      } else if (*s=='$') {
	int dir, len;
	if ((dir=*++s)=='$') {
	  *p++='$';
	  continue;
	}
	len=*++s;
	switch (*++s) {
	case 'l':
	  val=load[*++s-1]/overload;
	  break;
	case  'c':
	  val=busy[*++s-1];
	  break;
	case 'd':
	  val=disk[*++s-1]/disk_peak;
	  break;
	case 'n':
	  val=net[*++s-1]/net_peak;
	  break;
	case 'i':
	  val=isdn[*++s-1]/8000.0;
	  break;
	default:
	  val=0.0;
	}

	if (dir>0) {
	  int bar=val*len*xres;
	  lcd_bar (dir, r, p-buffer+1, len*xres, bar, bar);
	} else {
	  double bar=len*xres*log(val*len*xres+1)/log(len*xres); 
	  lcd_bar (-dir, r, p-buffer+1, len*xres, bar, bar);
	}
	
	for (i=0; i<len && p-buffer<cols; i++)
	  *p++='\t';
	
      } else {
	*p++=*s;
      }
    } while (*s++);
    
    if (smooth==0) {
      lcd_put (r, 1, buffer);
    }
  }
  lcd_flush();
}


void main (int argc, char *argv[])
{
  char *cfg="/etc/lcd4linux.conf";
  char *display;
  int i;
  int smooth;
  
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
  lcd_query (&rows, &cols, &xres, &yres, &bars);

  tick=atoi(cfg_get("tick"));
  tack=atoi(cfg_get("tack"));
  tau=atoi(cfg_get("tau"));
  overload=atof(cfg_get("overload"));

  for (i=1; i<=rows; i++) {
    char buffer[8];
    snprintf (buffer, sizeof(buffer), "row%d", i);
    row[i]=strdup(parse(cfg_get(buffer)));
  }
  
  lcd_clear();
  
  lcd_put (1,     1, "** LCD4Linux V" VERSION " **");
  lcd_put (rows-1,1, "(c) 2000 M. Reinelt");
  lcd_flush();
  
  sleep (2);
  lcd_clear();

  smooth=0;
  while (1) {
    draw(smooth);
    smooth+=tick;
    if (smooth>tack) smooth=0;
    usleep(1000*tick);
  }
}
