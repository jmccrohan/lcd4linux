#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "display.h"

static DISPLAY Display;
static char *Device=NULL;

int MO_init (DISPLAY *Self)
{
  char *port;
  
  printf ("initializing MatrixOrbital...\n");

  if (Device) {
    free (Device);
    Device=NULL;
  }

  port=cfg_get ("port");
  if (port==NULL || *port=='\0') {
    fprintf (stderr, "MatrixOrbital: no 'port' entry in %s\n", cfg_file());
    return -1;
  }
  Device=strdup(port);
  
  lcd=lcd_open();
  if (lcd==-1) return -1;
  
  lcd_clear();
  lcd_write ("\376B", 3);  // backlight on
  lcd_write ("\376K", 2);  // cursor off
  lcd_write ("\376T", 2);  // blink off
  lcd_write ("\376D", 2);  // line wrapping off
  lcd_write ("\376R", 2);  // auto scroll off
  lcd_write ("\376V", 2);  // GPO off

  return 0;
}

int MO_clear (void)
{
  return 0;
}

int MO_put (int x, int y, char *text)
{
  return 0;
}

int MO_bar (int type, int x, int y, int max, int len1, int len2)
{
  return 0;
}

int MO_flush (void)
{
  return 0;
}


#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_S )

DISPLAY MatrixOrbital[] = {
  { "LCD0821",  8, 2, 5, 8, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD1621", 16, 2, 5, 8, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD2021", 20, 2, 5, 8, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD2041", 20, 4, 5, 8, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush },
  { "LCD4021", 40, 2, 5, 8, BARS, MO_init, MO_clear, MO_put, MO_bar, MO_flush }, 
  { "" }
};

