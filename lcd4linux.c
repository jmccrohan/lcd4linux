#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include "config.h"
#include "lcd2041.h"
#include "system.h"
#include "isdn.h"

int tick, tack, tau;
double overload;
double temp_min, temp_max;
char *sensor;
char *row[ROWS];

void usage(void)
{
  printf ("LCD4Linux " VERSION " (c) 1999 Michael Reinelt <reinelt@eunet.at>");
  printf ("usage: LCD4Linux [configuration]\n");
}

char *parse (char *string)
{
  static char buffer[256];
  char *s=string;
  char *p=buffer;

  do {
    if (*s=='%') {
      if (strchr("orpmlLbtdDnNiI%", *++s)==NULL) {
	fprintf (stderr, "WARNING: unknown format <%%%c> in <%s>\n", *s, string);
	continue;
      } 
      *p='%';
      *(p+1)=*s;
      switch (*s) {
      case 'd':
	if (strchr("rwmt", *++s)==NULL) {
	  fprintf (stderr, "WARNING: unknown disk tag <%%i%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=*s;
	p+=3;
	break;
      case 'n':
	if (strchr("rwmt", *++s)==NULL) {
	  fprintf (stderr, "WARNING: unknown net tag <%%i%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=*s;
	p+=3;
	break;
      case 'i':
	if (strchr("iomt", *++s)==NULL) {
	  fprintf (stderr, "WARNING: unknown ISDN tag <%%i%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+2)=*s;
	p+=3;
	break;
      default:
	p+=2;
      }
      
    } else if (*s=='$') {
      char hv, dir;
      int len=0;
      hv=*++s;
      if (tolower(hv)!='h' && tolower(hv)!='v') {
	fprintf (stderr, "invalid bar orientation '%c' in <%s>\n", hv, string);
	continue;
      }
      s++;
      if (isdigit(*s)) len=strtol(s, &s, 10);
      if (len<1 || len>255) {
	fprintf (stderr, "invalid bar length in <%s>\n", string);
	continue;
      }
      dir=*s++;
      if ((tolower(hv)=='h' && dir!='l' && dir !='r') || (tolower(hv)=='v' && dir!='u' && dir !='d')) {
	fprintf (stderr, "invalid bar direction '%c' in <%s>\n", dir, string);
	continue;
      }
      *p='$';
      *(p+1)=hv;
      *(p+2)=len;
      if (dir=='r' || dir=='u') *(p+3)='0';
      else *(p+3)='1';
      *(p+4)=*s;
      switch (*s) {
      case 'd':
	if (strchr("rwmt", *++s)==NULL) {
	  fprintf (stderr, "WARNING: unknown disk tag <$i*%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+5)=*s;
	p+=6;
	break;
      case 'n':
	if (strchr("rwmt", *++s)==NULL) {
	  fprintf (stderr, "WARNING: unknown net tag <$i*%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+5)=*s;
	p+=6;
	break;
      case 'i':
	if (strchr("iomt", *++s)==NULL) {
	  fprintf (stderr, "WARNING: unknown ISDN tag <$i*%c> in <%s>\n", *s, string);
	  continue;
	} 
	*(p+5)=*s;
	p+=6;
	break;
      default:
	p+=5;
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


void display (int smooth) {
  static double disk_max=1.0;
  static double net_max=1.0;
  double busy, load, temp, disk, net, isdn;
  int disk_r, disk_w;
  int net_tx, net_rx;
  int isdn_usage, isdn_in, isdn_out;
  char buffer[256];
  int i;
  
  busy=Busy();
  load=Load();
  Disk (&disk_r, &disk_w);
  Net (&net_rx, &net_tx);
  temp=Temperature();
  isdn_usage=Isdn(&isdn_in, &isdn_out);

  if (disk_r>disk_max) disk_max=disk_r;
  if (disk_w>disk_max) disk_max=disk_w;

  if (net_rx>net_max) net_max=net_rx;
  if (net_tx>net_max) net_max=net_tx;
  
  for (i=0; i<ROWS; i++) {
    char *s=row[i];
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
	  if (load<10.0)
	    p+=sprintf (p, "%4.2f", load);
	  else
	    p+=sprintf (p, "%4.1f", load);
	  break;
	case 'L':
	  *p++=load>overload?'!':' ';
	  break;
	case 'b':
	  p+=sprintf (p, "%3.0f", 100.0*busy);
	  break;
	case 'd':
	  switch (*++s) {
	  case 'r':
	    disk=disk_r;
	    break;
	  case 'w':
	    disk=disk_w;
	    break;
	  case 'm':
	    disk=disk_r>disk_w?disk_r:disk_w;
	    break;
	  default:
	    disk=disk_r+disk_w;
	    break;
	  }
	  disk/=1024;
	  if (disk<10.0) {
	    p+=sprintf (p, "%4.2f", disk);
	  } else if (disk<100.0) {
	    p+=sprintf (p, "%4.1f", disk);
	  } else {
	    p+=sprintf (p, "%4.0f", disk);
	  }
	  break;
	case 'D':
	  if (disk_r+disk_w==0)
	    *p++=' ';
	  else
	    *p++=disk_r>disk_w?'\176':'\177';
	  break;
	case 'n':
	  switch (*++s) {
	  case 'r':
	    net=net_rx;
	    break;
	  case 'w':
	    net=net_tx;
	    break;
	  case 'm':
	    net=net_rx>net_tx?net_rx:net_tx;
	    break;
	  default:
	    net=net_rx+net_tx;
	    break;
	  }
	  net/=1024.0;
	  if (net<10.0) {
	    p+=sprintf (p, "%4.2f", net);
	  } else if (net<100.0) {
	    p+=sprintf (p, "%4.1f", net);
	  } else {
	    p+=sprintf (p, "%4.0f", net);
	  }
	  break;
	case 'N':
	  if (net_rx+net_tx==0)
	    *p++=' ';
	  else
	    *p++=net_rx>net_tx?'\176':'\177';
	  break;
	case 't':
	  p+=sprintf (p, "%5.1f", temp);
	  break;
	case 'i':
	  if (isdn_usage) {
	    switch (*++s) {
	    case 'i':
	      isdn=isdn_in;
	      break;
	    case 'o':
	      isdn=isdn_out;
	      break;
	    case 'm':
	      isdn=isdn_in>isdn_out?isdn_in:isdn_out;
	      break;
	    default:
	      isdn=isdn_in+isdn_out;
	      break;
	    }
	    isdn/=1024.0;
	    if (isdn<10.0) {
	      p+=sprintf (p, "%4.2f", isdn);
	    } else if (isdn<100.0) {
	      p+=sprintf (p, "%4.1f", isdn);
	    } else {
	      p+=sprintf (p, "%4.0f", isdn);
	    }
	  } else {
	    p+=sprintf (p, "----");
	    s++;
	  }
	  break;
	case 'I':
	  if (isdn_in+isdn_out==0)
	    *p++=' ';
	  else
	    *p++=isdn_in>isdn_out?'\176':'\177';
	  break;
	}
	
      } else if (*s=='$') {
      double val;
      int hv, len, dir;
      hv=*++s;
      len=*++s;
      dir=*++s;
      switch (*++s) {
      case 'l':
	val=load/overload;
	break;
      case  'b':
	val=busy;
	break;
      case 'd':
	switch (*++s) {
	case 'r':
	  val=disk_r/disk_max;
	  break;
	case 'w':
	  val=disk_w/disk_max;
	  break;
	case 'm':
	  val=(disk_r>disk_w?disk_r:disk_w)/disk_max;
	  break;
	default:
	  val=(disk_r+disk_w)/(2*disk_max);
	  break;
	}
	break;
      case 'n':
	switch (*++s) {
	case 'r':
	  val=net_rx/net_max;
	  break;
	case 'w':
	  val=net_tx/net_max;
	  break;
	case 'm':
	  val=(net_rx>net_tx?net_rx:net_tx)/net_max;
	  break;
	default:
	  val=(net_rx+net_tx)/(2*net_max);
	  break;
	}
	break;
      case 't':
	val=(temp-temp_min)/(temp_max-temp_min);
	break;
      case 'i':
	switch (*++s) {
	case 'i':
	  val=isdn_in;
	  break;
	case 'o':
	  val=isdn_out;
	  break;
	case 'm':
	  val=isdn_in>isdn_out?isdn_in:isdn_out;
	  break;
	default:
	  val=isdn_in+isdn_out;
	  break;
	}
	val/=8000.0;
	break;
      default:
	val=0.0;
      }
      switch (hv) {
      case 'h':
	lcd_hbar (p-buffer+1, i+1, dir-'0', len*XRES, val*len*XRES); 
	break;
      case 'H':
	lcd_hbar (p-buffer+1, i+1, dir-'0', len*XRES, (double)len*XRES*log(val*len*XRES+1)/log(len*XRES)); 
	break;
      case 'v':
	lcd_vbar (p-buffer+1, i+1, dir-'0', len*YRES, val*len*XRES); 
	break;
      }
      while (len-->0) {
	*p++='\t'; 
      }
      
    } else {
      *p++=*s;
    }
  } while (*s++);
    
    if (smooth==0) {
      p=buffer;
      while (*p) {
	while (*p=='\t') p++;
	for (s=p; *s; s++) {
	  if (*s=='\t') {
	    *s++='\0';
	    break;
	  }
	}
	if (*p) {
	  lcd_put (p-buffer+1, i+1, p);
	}
	p=s;
      }
    }
  }
}


void main (int argc, char *argv[])
{
  char *cfg_file="/etc/lcd4linux.conf";
  char *port;
  int i;
  int contrast;
  int smooth;
  
  if (argc>2) {
    usage();
    exit (2);
  }
  if (argc==2) {
    cfg_file=argv[1];
  }

  set_cfg ("row1", "*** %o %r ***");
  set_cfg ("row2", "%p CPU  %m MB RAM");
  set_cfg ("row3", "Busy %b%% $r50b");
  set_cfg ("row4", "Load %l$r50l");

  set_cfg ("tick", "100");
  set_cfg ("tack", "500");
  set_cfg ("tau", "500");
  set_cfg ("contrast", "140");
  set_cfg ("overload", "2.0");
  set_cfg ("temp_min", "20");
  set_cfg ("temp_max", "70");
  
  set_cfg ("fifo", "/var/run/LCD4Linux");

  if (read_cfg (cfg_file)==-1)
    exit (1);

  port=get_cfg("port");
  sensor=get_cfg("temperature");
  tick=atoi(get_cfg("tick"));
  tack=atoi(get_cfg("tack"));
  tau=atoi(get_cfg("tau"));
  contrast=atoi(get_cfg("contrast"));
  overload=atof(get_cfg("overload"));
  temp_min=atof(get_cfg("temp_min"));
  temp_max=atof(get_cfg("temp_max"));

  if (port==NULL || *port=='\0') {
    fprintf (stderr, "%s: missing 'port' entry!\n", cfg_file);
    exit (1);
  }
  
  for (i=0; i<ROWS; i++) {
    char buffer[8];
    snprintf (buffer, sizeof(buffer), "row%d", i+1);
    row[i]=strdup(parse(get_cfg(buffer)));
  }
  
  lcd_init(port);
  lcd_clear();
  lcd_contrast (contrast);

  lcd_put (1, 1, "** LCD4Linux " VERSION " **");
  lcd_put (1, 3, "(c) 1999 M. Reinelt");


  { 
    int a, b, c, d;

    lcd_dbar_init();
    for (a=0; a<40; a++) {
      b=40-a;
      c=2*a;
      d=c+a;
      lcd_dbar (2, 1, 0, 80, a, b);
      lcd_dbar (2, 2, 0, 80, c, d);
      lcd_dbar_flush();
      usleep( 300*1000);
    }
  }

  sleep (2);
  lcd_clear();

  smooth=0;
  while (1) {
    display(smooth);
    smooth+=tick;
    if (smooth>tack) smooth=0;
    usleep(1000*tick);
  }
}
