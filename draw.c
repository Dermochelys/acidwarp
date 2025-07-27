/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android by Matthew Zavislak
 */

#include <SDL3/SDL.h>

/* SDL 3 compatibility fixes */

#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
/* Similar to acidwarp.c, need compatibility for mutex and condition variables */
static inline int SDL_LockMutex_compat(SDL_Mutex *mutex) {
    SDL_LockMutex(mutex);
    return 0;
}
static inline int SDL_UnlockMutex_compat(SDL_Mutex *mutex) {
    SDL_UnlockMutex(mutex);
    return 0;
}
#define SDL_LockMutex SDL_LockMutex_compat
#define SDL_UnlockMutex SDL_UnlockMutex_compat

#include "handy.h"
#include "acidwarp.h"
#include "bit_map.h"
#include "display.h"

static int imageFuncList[NUM_IMAGE_FUNCTIONS];
static int imageFuncListIndex=0;
static int flags = 0;
static bool drawnext = false;
static SDL_Condition *drawnext_cond = NULL;
static bool drawdone = false;
static SDL_Condition *drawdone_cond = NULL;
static SDL_Mutex *draw_mtx = NULL;
static int drawing_main(void *param);
static SDL_Thread *drawing_thread = NULL;
int abort_draw = 0;
int quit_draw = 0;
static int redraw_same = 0;

static void draw(int which) {
  UCHAR *buf_graf;
  unsigned int buf_graf_stride, width, height;
  disp_beginUpdate(&buf_graf, &buf_graf_stride, &width, &height);
  if (which < 0) {
    writeBitmapImageToArray(buf_graf, width, height,buf_graf_stride);
  } else {
    if (flags & DRAW_FLOAT) {
      generate_image_float(which,
                           buf_graf, width/2, height/2, width, height,
                           256, buf_graf_stride, flags & DRAW_SCALED);
    }
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

/* Drawing runs on separate thread, so as soon as one image is drawn to the
 * screen, the next can be computed in the background. This way, typical
 * image changes won't need to wait for drawing computations, which can be
 * slow when generating large images using floating point.
 */
static int drawing_main(void *param) {
  int displayed_img;
  int draw_img = (flags & DRAW_LOGO) ? -1 : imageFuncList[imageFuncListIndex];
  displayed_img = draw_img;
  while (1) {
    /* Draw next image to back buffer */
    draw(draw_img);

    /* Tell main thread that image is drawn */
    SDL_LockMutex(draw_mtx);
    drawdone = true;
    SDL_SignalCondition(drawdone_cond);

    /* Wait for next image */
    while (!drawnext) {
      SDL_WaitCondition(drawnext_cond, draw_mtx);
    }
    drawnext = false;
    SDL_UnlockMutex(draw_mtx);

    if (quit_draw) break;

    if (redraw_same) {
      draw_img = displayed_img;
      redraw_same = 0;
    } else {
      /* move to the next image */
      draw_advance();
      displayed_img = draw_img;
      draw_img = imageFuncList[imageFuncListIndex];
    }
  }
  return 0;
}

void draw_abort(void) {
  if (drawing_thread != NULL) {
    SDL_LockMutex(draw_mtx);
    while (!drawdone) {
      abort_draw = 1;
      SDL_WaitCondition(drawdone_cond, draw_mtx);
      abort_draw = 0;
    }
    SDL_UnlockMutex(draw_mtx);
  }
}

void draw_next(void) {
  /* Wait for image to finish drawing image */
  SDL_LockMutex(draw_mtx);
  while (!drawdone) {
    SDL_WaitCondition(drawdone_cond, draw_mtx);
  }

  /* This should actually display what the thread drew */
  disp_swapBuffers();

  /* Tell drawing thread it can continue now that buffers are swapped */
  drawnext = true;
  drawdone = false;
  SDL_SignalCondition(drawnext_cond);
  SDL_UnlockMutex(draw_mtx);
}

static void draw_continue(void) {
  SDL_LockMutex(draw_mtx);
  drawnext = true;
  drawdone = false;
  SDL_SignalCondition(drawnext_cond);
  SDL_UnlockMutex(draw_mtx);
}

void draw_same(void) {
  redraw_same = 1;
  draw_continue();
  draw_next();
}

void draw_init(int draw_flags) {
  flags = draw_flags;
  makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
  abort_draw = 0;
  quit_draw = 0;
  if (!(draw_mtx = SDL_CreateMutex()) ||
      !(drawdone_cond = SDL_CreateCondition()) ||
      !(drawnext_cond = SDL_CreateCondition())
      )
    fatalSDLError("creating drawing synchronization primitives");
  drawing_thread = SDL_CreateThread(drawing_main,
                                    "DrawingThread",
                                    NULL);
  if (drawing_thread == NULL)
    fatalSDLError("creating drawing thread");
  /* TODO check SDL errors */
}
