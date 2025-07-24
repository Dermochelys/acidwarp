/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android by Matthew Zavislak
 */

#include <SDL3/SDL.h>

#include "handy.h"
#include "acidwarp.h"
#include "bit_map.h"
#include "display.h"

static int imageFuncList[NUM_IMAGE_FUNCTIONS];
static int imageFuncListIndex=0;
static int flags = 0;
static int draw_first = 0;

static void draw(int which) {
  UCHAR *buf_graf;
  unsigned int buf_graf_stride, width, height;
  disp_beginUpdate(&buf_graf, &buf_graf_stride, &width, &height);
  if (which < 0) {
    writeBitmapImageToArray(buf_graf, NOAHS_FACE, width, height,
                            buf_graf_stride);
  } else {
    generate_image_float(which,
                         buf_graf, width/2, height/2, width, height,
                         256, buf_graf_stride, flags & DRAW_SCALED);
  }
  disp_finishUpdate();
}

static void draw_advance(void)
{
  flags &= ~DRAW_LOGO;
  if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS) {
    imageFuncListIndex = 0;
    makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
  }
}

void draw_same(void) {
  draw((flags & DRAW_LOGO) ? -1 : imageFuncList[imageFuncListIndex]);
  disp_swapBuffers();
}

void draw_next(void) {
  if (draw_first) {
    draw_first = 0;
  } else {
    draw_advance();
  }
  draw_same();
}

void draw_init(int draw_flags) {
  flags = draw_flags;
  makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
  draw_first = 1;
}
