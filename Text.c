/* $Id: Text.c,v 1.2 2001/03/09 15:04:53 reinelt Exp $
 *
 * pure ncurses based text driver
 *
 * Copyright 2001 by Leopold Tötsch (lt@toetsch.at)
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
 * $Log: Text.c,v $
 * Revision 1.2  2001/03/09 15:04:53  reinelt
 *
 * rename 'raster' to 'Text in Text.c
 * added TOTO item
 *
 * Revision 1.1  2001/03/09 13:08:11  ltoetsch
 * Added Text driver
 *
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD Text[]
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>

#define min(x,y) (x)<(y)?x:y

#ifdef STANDALONE

int main(int argc, char *argv[]) 
{
  WINDOW *w;
  int x,y;
  
  w = initscr();
  
  for (x=0; x < 255; x++)
    addch(acs_map[x]);    
  refresh();
  sleep(5);
  endwin();
  return 0;
  
}

#else

#include <string.h>
#include "debug.h"
#include "cfg.h"
#include "display.h"
extern int foreground;

static LCD Lcd;
static WINDOW *w;
WINDOW *err_win;
#define BARS ( BAR_R | BAR_H2 )
int err_rows;

int Text_clear (void)
{
  werase(w);
  box(w, 0, 0);
  return 0;
}

int Text_quit(void) {
  endwin();
  return 0;
}

int Text_init (LCD *Self)
{
  int cols=-1, rows=-1;
  int scr_cols, scr_rows;
  char *s;
  if (!foreground) {
    error("Text: you want me to display on /dev/null: sorry, I can't");
    return -1;
  }
  
  if (sscanf(s=cfg_get("size")?:"20x4", "%dx%d", &cols, &rows)!=2 || rows<1 || cols<1) {
    error ("Text: bad size '%s'", s);
    return -1;
  }
  Self->rows=rows;
  Self->cols=cols;
  Self->xres=1;
  Self->yres=1;
  Lcd=*Self;
  
  w = initscr();
  scr_cols=COLS;
  scr_rows=LINES;
  delwin(w);
  w = newwin(rows+2,cols+2,0,0);
  err_rows = scr_rows-rows-3;
  if (err_rows >= 4) {
    err_win = newwin(err_rows, scr_cols, rows+3, 0);
    err_rows -= 3;
    box(err_win, 0, 0);
    mvwprintw(err_win,0,3, "Stderr:");
    wmove(err_win, 1 , 0);
    wrefresh(err_win);
  }
  Text_clear();
  return w ? 0 : -1;
}

/* ncures scroll SIGSEGVs on my system, so this is a workaroud */

int curs_err(char *buffer) 
{
  static int lines;
  static char *lb[100];
  int start, i;
  
  if (err_win) {
    if (lines >= err_rows) {
      free(lb[0]);
      for (i=1; i<=err_rows; i++) 
	lb[i-1] = lb[i];
      start = 0;
    }
    else
      start = lines;
    lb[lines] = strdup(buffer);
    for (i=start; i<=lines; i++) {
      mvwprintw(err_win,i+1,1, "%s", lb[i]);
      wclrtoeol(err_win);
    }
    box(err_win, 0, 0);
    mvwprintw(err_win,0,3, "Stderr:");
    wrefresh(err_win);
    if (lines < err_rows)
      lines++;
    return 1;
  }
  return 0;
}

int Text_put (int row, int col, char *text)
{
  if (col < Lcd.cols)
    mvwprintw(w, row+1 , col+1, "%.*s", Lcd.cols-col, text);
  return 0;
}

int Text_bar (int type, int row, int col, int max, int len1, int len2)
{
  int len, i;
  if (cfg_get("TextBar")) 
    mvwprintw(w, row+1 , col+1, "%d %d %d", max, len1, len2);
  else {
    len = min(len1, len2);
    len = min(len, Lcd.cols-col-1);
    if (len) {
      wmove(w, row+1 , col+1);
      for (i=0; i<len;i++)
        waddch(w,ACS_BLOCK);
    }
    col += len;
    len1 -= len;
    len2 -= len;
    len1 = min(len1, Lcd.cols-col-1);
    for (i=0; i<len1;i++)
      waddch(w,ACS_S1);
    len2 = min(len2, Lcd.cols-col-1);
    for (i=0; i<len2;i++)
      waddch(w,ACS_S9);
  }
  
  return 0;
}

int Text_gpo (int num, int val)
{
  return 0;
}

int Text_flush (void)
{
  box(w, 0, 0);
  wrefresh(w);  
  return 0;
}


LCD Text[] = {
  { "Text",4,20,1,1,BARS,0,
      Text_init,Text_clear,Text_put,Text_bar,Text_gpo,Text_flush,Text_quit },
  { NULL }
};

#endif
