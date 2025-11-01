/*
 * Amiga backend stuff for 'acidwarp'
 * by megacz@usa.com
*/

#include <proto/exec.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/ports.h>

#include <proto/dos.h>
#include <dos/dostags.h>

#include <proto/graphics.h>
#include <graphics/gfxbase.h>

#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <string.h>

#define AO_ECS  1
#define AO_SYNC 2
#define AO_BMAP 4

#define OSD_SIZE_X 104
#define OSD_SIZE_Y 14
#define OSD_POS_X 16
#define OSD_POS_Y (OSD_SIZE_Y + OSD_POS_X)
#define OSD_DRAW_X 2
#define OSD_DRAW_Y (OSD_SIZE_Y / 1.5)
#define OSD_A_PEN 1
#define OSD_B_PEN 52
#define OSD_TIME 2

#define PTBUF 32

#define ITOAHEXBUF 10

#define ITERATE_LIST(list, type, node)                           \
for (node = (type)((struct List *)(list))->lh_Head;              \
((struct Node *)node)->ln_Succ;                                  \
node = (type)((struct Node *)node)->ln_Succ)



struct amiga_screen
{
  struct RastPort as_temprp;
  struct Screen *as_screen;
  struct Window *as_window;
  struct Window *as_osd;
  struct TextFont *as_tf;
  struct TextAttr as_ta;
  BYTE as_signal;
  LONG as_depth;
  ULONG as_modeid;
  UBYTE *as_scrname;
  UBYTE *as_pubname;
  UWORD as_rgb4[256];
  ULONG *as_rgb32;
  LONG as_ecs;
};

struct procdata
{
  struct Task *task;
  int imageFuncNum;
  UBYTE *buf_graf;
  int xcenter;
  int ycenter;
  int xmax;
  int ymax;
  int colormax;
};



extern int generate_image(int, UBYTE *, int, int, 
                          int, int, int);

BOOL amiga_openscreen(struct amiga_screen *);
void amiga_closescreen(struct amiga_screen *);
BOOL amiga_openosd(struct amiga_screen *);
void amiga_closeosd(struct amiga_screen *);
void amiga_putosdtext(struct amiga_screen *, UBYTE *);
BOOL amiga_chkforvis(struct Screen *);
WORD amiga_getviscnt(struct Screen *);
BOOL amiga_openlibs(void);
void amiga_closelibs(void);
int amiga_getkey(struct amiga_screen *);
ULONG amiga_getwbmodeid(void);
ULONG *amiga_allocrgb32(ULONG);
ULONG amiga_detecttask(ULONG);
ULONG amiga_forceclut(ULONG);

ULONG mst_AtoULONG(UBYTE *);
char *mst_modULONGtoAHEX(char *, ULONG, UBYTE);

int amikey_keypressed(int);
BOOL amibreaksig_check(void);
void unix_control_c_handler(int);
void gl_setpalettecolors(int, int, void *);
void gl_putbox(int, int, int, int, void *);
BOOL amigl_allocatecontext(ULONG);
void amigl_destroycontext(void);
void amihelp_convbuftoecs(UBYTE *, ULONG, UBYTE);
void amihelp_renderbuffer(int, int, UBYTE *, int, 
                          int, int, int, int);
int amihelp_putosdtext(UBYTE *);
