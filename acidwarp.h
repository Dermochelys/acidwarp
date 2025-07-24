/* ACIDWARP.H */

#define VERSION "Acid Warp Version 4.10 (C)Copyright 1992, 1993 by Noah Spurrier and Mark Bilk\nAndroid port by Matthew Zavislak, based on SDL port by Boris Gjenero, based on Linux port by Steven Wills"

#define DIRECTN_CHANGE_PERIOD_IN_TICKS               256

/* Palette types  */
#define RGBW_PAL          	0
#define W_PAL                   1
#define W_HALF_PAL              2
#define PASTEL_PAL              3
#define RGBW_LIGHTNING_PAL      4
#define W_LIGHTNING_PAL         5
#define W_HALF_LIGHTNING_PAL    6
#define PASTEL_LIGHTNING_PAL    7
#define RED 0
#define GREEN 1
#define BLUE 2
#define NUM_PALETTE_TYPES       8

#define NUM_IMAGE_FUNCTIONS 40
#define NOAHS_FACE   0

enum acidwarp_command {
  CMD_SKIP,
  CMD_QUIT,
  CMD_NEWPAL,
  CMD_PAL_FASTER,
  CMD_PAL_SLOWER,
  CMD_RESIZE,
};

void handleInput(enum acidwarp_command cmd);
void generate_image_float(int imageFuncNum, UCHAR *buf_graf,
                          int xcenter, int ycenter,
                          int width, int height,
                          int colors, int pitch, int normalize);
void fatalSDLError(const char *msg);
void quit();
void makeShuffledList(int *list, int listSize);

#define DRAW_LOGO 1
#define DRAW_SCALED 4
void draw_init(int flags);
void draw_same(void);
void draw_next(void);
