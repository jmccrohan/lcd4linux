#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "display.h"


int MO_init (void)
{
  printf ("initializing MatrixOrbital...\n");
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

