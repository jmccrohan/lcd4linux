#include <stdlib.h>
#include <stdio.h>

#include "display.h"

extern DISPLAY MatrixOrbital[];

FAMILY Driver[] = {
  { "MatrixOrbital", MatrixOrbital },
  { "" }
};


static DISPLAY *Display = NULL;

int lcd_init (char *display)
{
  int i, j;
  for (i=0; Driver[i].name[0]; i++) {
    for (j=0; Driver[i].Display[j].name[0]; j++) {
      if (strcmp (Driver[i].Display[j].name, display)==0) {
	Display=&Driver[i].Display[j];
	return Display->init();
      }
    }
  }
  fprintf (stderr, "lcd_init(%s) failed: no such display\n", display);
  return -1;
}

int lcd_clear (void)
{
  return 0;
}

int lcd_put (int x, int y, char *text)
{
  return 0;
}

int lcd_bar (int type, int x, int y, int max, int len1, int len2)
{
  return 0;
}

int lcd_flush (void)
{
  return 0;
}

void main (void) {
  int i, j;
  
  lcd_init ("junk");
  lcd_init ("LCD2041");

}
