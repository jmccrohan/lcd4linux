#define BAR_L 1
#define BAR_R 2
#define BAR_U 4
#define BAR_D 8
#define BAR_S 256

typedef struct {
  char name[16];
  int rows;
  int cols;
  int xres;
  int yres;
  int bars;
  int (*init) (void);
  int (*clear) (void);
  int (*put) (int x, int y, char *text);
  int (*bar) (int type, int x, int y, int max, int len1, int len2);
  int (*flush) (void);
} DISPLAY;

typedef struct {
  char name[16];
  DISPLAY *Display;
} FAMILY;

int lcd_init (char *display);
int lcd_clear (void);
int lcd_put (int x, int y, char *text);
int lcd_bar (int type, int x, int y, int max, int len1, int len2);
int lcd_flush (void);
