#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "lcd2041.h"

#define BAR_HORI 1
#define BAR_VERT 2
#define BAR_DUAL 3

static int lcd;
static char *lcd_device=NULL;
static int bar_mode=0;

typedef struct {
  int l1;
  int l2;
  int chr;
} SEGMENT;

typedef struct {
  int l1;
  int l2;
  int lru;
} CHARACTER;

static SEGMENT Segment[COLS+1][ROWS+1];
static CHARACTER Character[CHAR];

static int lcd_open(void)
{
  int fd;
  struct termios portset;
  
  fd = open(lcd_device, O_RDWR | O_NOCTTY | O_NDELAY); 
  if (fd==-1) {
    fprintf (stderr, "open(%s) failed: %s\n", lcd_device, strerror(errno));
    return -1;
  }
  if (tcgetattr(fd, &portset)==-1) {
    fprintf (stderr, "tcgetattr(%s) failed: %s\n", lcd_device, strerror(errno));
    return -1;
  }
  cfmakeraw(&portset);
  cfsetospeed(&portset, B19200);
  if (tcsetattr(fd, TCSANOW, &portset)==-1) {
    fprintf (stderr, "tcsetattr(%s) failed: %s\n", lcd_device, strerror(errno));
    return -1;
  }
  return fd;
}


static void lcd_write (char *string, int len)
{
  if (lcd==-1) return;
  if (write (lcd, string, len)==-1) {
    if (errno==EAGAIN) {
      usleep(1000);
      if (write (lcd, string, len)>=0) return;
    }
    fprintf (stderr, "write(%s) failed: %s\n", lcd_device, strerror(errno));
  }
}


void lcd_clear (void)
{
  lcd_write ("\014", 1);
}


void lcd_put (int x, int y, char *string)
{
  char buffer[256];
  snprintf (buffer, sizeof(buffer), "\376G%c%c%s", x, y, string);
  lcd_write (buffer, strlen(buffer));
}
 
void lcd_hbar (int x, int y, int dir, int max, int len)
{
  char buffer[COLS+5];
  char *p;

  if (bar_mode!=BAR_HORI) {
    lcd_write ("\376h", 2);
    bar_mode=BAR_HORI;
  }

  if (len<1) len=1;
  else if (len>max) len=max;
  if (dir!=0) len=max-len;

  snprintf (buffer, sizeof(buffer), "\376G%c%c", x, y);
  p=buffer+4;
  
  while (max>0 && p-buffer<sizeof(buffer)) {
    if (len==0) {
      *p=dir?255:32;
    } else if (len>=XRES) {
      *p=dir?32:255;
      len-=XRES;
    } else {
      *p=dir?8-len:len-1;
      len=0;
    }
    max-=XRES;
    p++;
  }
  lcd_write (buffer, p-buffer);
}


void lcd_vbar (int x, int y, int dir, int max, int len)
{
  char buffer[6];
  unsigned char c;

  if (bar_mode!=BAR_VERT) {
    lcd_write ("\376v", 2);
    bar_mode=BAR_VERT;
  }

  if (len<1) len=1;
  else if (len>max) len=max;

  while (max>0 && y>0) {
    if (len==0) {
      c=32;
    } else if (len>=XRES) {
      c=255;
      len-=XRES;
    } else {
      c=len;
      len=0;
    }
    snprintf (buffer, sizeof(buffer), "\376G%c%c%c", x, y, c);
    lcd_write (buffer, 5);
    max-=XRES;
    y--;
  }
}


static void lcd_dbar_init (void)
{
  int x, y;
  
  bar_mode=BAR_DUAL;

  for (x=0; x<CHAR; x++) {
    Character[x].l1=-1;
    Character[x].l2=-1;
    Character[x].lru=0;
  }
  
  for (x=0; x<COLS; x++) {
    for (y=0; y<ROWS; y++) {
      Segment[x][y].l1=-1;
      Segment[x][y].l2=-1;
    }
  }
}

static int lcd_dbar_char (int l1, int l2)
{
  int i, j, min;

  if (l1==127) l1=0;
  if (l2==127) l2=0;
  
  if (l1==0 && l2==0) return 32;
  if (l1==XRES && l2==XRES) return 255;

  for (i=0; i<CHAR; i++) {
    if (Character[i].l1==l1 && Character[i].l2==l2) {
      Character[i].lru=2;
      return i;
    }
  }

  for (i=0; i<CHAR; i++) {
    if (Character[i].lru==0) {
      printf ("creating char %d (%d/%d)\n", i, l1, l2);
      Character[i].l1=l1;
      Character[i].l2=l2;
      Character[i].lru=2;
      return i;
    }
  }

  min=XRES*YRES;
  for (i=0; i<CHAR; i++) {
    int diff;
    if (l1==0 && Character[i].l1!=0) continue;
    if (l2==0 && Character[i].l2!=0) continue;
    if (l1==XRES && Character[i].l1!=XRES) continue;
    if (l2==XRES && Character[i].l2!=XRES) continue;
    diff=abs(Character[i].l1-l1)+abs(Character[i].l2-l2);
    if (diff<min) {
      min=diff;
      j=i;
    }
  }
  printf ("lcd_dbar: diff=%d\n", min);
  return j;
}

void lcd_dbar (int x, int y, int dir, int max, int len1, int len2)
{
  if (bar_mode!=BAR_DUAL)
    lcd_dbar_init();
  
  if (len1<1) len1=1;
  else if (len1>max) len1=max;

  if (len2<1) len2=1;
  else if (len2>max) len2=max;

  while (max>0 && x<=COLS) {
    if (len1==0) {
      Segment[x][y].l1=0;
    } else if (len1>=XRES) {
      Segment[x][y].l1=XRES;
      len1-=XRES;
    } else {
      Segment[x][y].l1=len1;
      len1=0;
    }
    if (len2==0) {
      Segment[x][y].l2=0;
    } else if (len2>=XRES) {
      Segment[x][y].l2=XRES;
      len2-=XRES;
    } else {
      Segment[x][y].l2=len2;
      len2=0;
    }
    max-=XRES;
    x++;
  }
}

void lcd_dbar_flush (void)
{
  int i, x, y;
  
  for (y=0; y<=ROWS; y++) {
    for (x=0; x<=COLS; x++) {
      if ((Segment[x][y].l1==0 && Segment[x][y].l2==XRES) || (Segment[x][y].l1==XRES && Segment[x][y].l2==0))
	Segment[x][y].chr=lcd_dbar_char(Segment[x][y].l1, Segment[x][y].l2);
    }
  }
  for (y=0; y<=ROWS; y++) {
    for (x=0; x<=COLS; x++) {
      if (Segment[x][y].l1!=-1 || Segment[x][y].l2!=-1) 
	Segment[x][y].chr=lcd_dbar_char(Segment[x][y].l1, Segment[x][y].l2);
    }
  }

  for (i=0; i<CHAR; i++) {
    if (Character[i].lru==2) {
      char buffer[12];
      char pixel[XRES+1]={0, 16, 24, 28, 30, 31};
      char p0=pixel[Character[i].l1];
      char p1=pixel[Character[i].l2];
      snprintf (buffer, sizeof(buffer), "\376N%c%c%c%c%c%c%c%c%c", i, p0, p0, p0, p0, p1, p1, p1, p1);
      lcd_write (buffer, 11);
    }
    if (Character[i].lru>0)
      Character[i].lru--;
  }

  for (y=0; y<=ROWS; y++) {
    for (x=0; x<=COLS; x++) {
      if (Segment[x][y].l1!=-1 || Segment[x][y].l2!=-1) {
	char buffer[6];
	snprintf (buffer, sizeof(buffer), "\376G%c%c%c", x, y, Segment[x][y].chr);
	lcd_write (buffer, 5);
      }
    }
  }
}


int lcd_init (char *device)
{
  if (lcd_device) free (lcd_device);
  lcd_device=strdup(device);
  lcd=lcd_open();
  if (lcd==-1) return -1;
  
  lcd_clear();
  lcd_write ("\376B", 3); // backlight on
  lcd_write ("\376K", 2);   // cursor off
  lcd_write ("\376T", 2);   // blink off
  lcd_write ("\376D", 2);   // line wrapping off
  lcd_write ("\376R", 2);   // auto scroll off
  lcd_write ("\376V", 2);   // GPO off

  return 0;
}

void lcd_contrast (int contrast)
{
  char buffer[4];
  snprintf (buffer, 4, "\376P%c", contrast);
  lcd_write (buffer, 3);
}

