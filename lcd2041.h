#define ROWS 4
#define COLS 20
#define XRES 5
#define YRES 8
#define CHAR 8

int lcd_init (char *device);
void lcd_contrast (int contrast);
void lcd_clear (void);
void lcd_put (int x, int y, char *string);
void lcd_hbar (int x, int y, int dir, int max, int len);
void lcd_vbar (int x, int y, int dir, int max, int len);
void lcd_dbar (int x, int y, int dir, int max, int len1, int len2);
void lcd_dbar_flush (void);
