/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android by Matthew Zavislak
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "handy.h"
#include "acidwarp.h"
#include "display.h"

static SDL_Window *window = NULL;
static SDL_Surface *screen = NULL, *out_surf = NULL;
#define draw_surf out_surf
static int disp_DrawingOnSurface;
static UCHAR *draw_buf = NULL;

static int fullscreen = 0;
static int winwidth = 0;
static int winheight;

static int scaling = 1;
static int width, height;

void disp_setPalette(unsigned char *palette)
{
  static SDL_Color sdlColors[256];

  int i;
  for(i=0;i<256;i++) {
    sdlColors[i].r = palette[i*3+0] << 2;
    sdlColors[i].g = palette[i*3+1] << 2;
    sdlColors[i].b = palette[i*3+2] << 2;
  }

    /* Update colours in software surface, blit it to the screen
     * with updated colours, and then show it on the screen.
     */
    /* Is this really necessary,
     * or could code above write directly into sdlPalette->Colors?
     */
    SDL_SetSurfacePalette(out_surf, SDL_CreatePalette(256));
    SDL_SetPaletteColors(SDL_GetSurfacePalette(out_surf), sdlColors, 0, 256);
    if (out_surf != screen) {
      SDL_BlitSurface(out_surf, NULL, screen, NULL);
    }
    SDL_UpdateWindowSurface(window);

}

void disp_beginUpdate(UCHAR **p, unsigned int *pitch,
                      unsigned int *w, unsigned int *h)
{
  /* Locking only needed at this point if drawing routines directly draw
   * on a surface, and that surface needs locking.
   */
  if (disp_DrawingOnSurface) {
    if (SDL_MUSTLOCK(draw_surf)) {
      if (SDL_LockSurface(draw_surf) != 0) {
        fatalSDLError("locking surface when starting update");
      }
    }
    *p = draw_surf->pixels;
    *pitch = draw_surf->pitch;
  } else {
    *p = draw_buf;
    *pitch = width;
  }
  *w = width;
  *h = height;
}

void disp_finishUpdate(void)
{
  /* Locking only needed at this point if drawing routines directly draw
   * on a surface, and that surface needs locking.
   */
  if (disp_DrawingOnSurface) {
    if (SDL_MUSTLOCK(draw_surf)) SDL_UnlockSurface(draw_surf);
    draw_buf = NULL;
  }
}

static void disp_toSurface(void)
{
  int row;
  unsigned char *outp, *inp = draw_buf;
  /* This means drawing was on a separate buffer and it needs to be
   * copied to the surface. It also means the surface hasn't been locked.
   */
  if (SDL_MUSTLOCK(out_surf)) {
    if (SDL_LockSurface(out_surf) != 0) {
      fatalSDLError("locking surface when ending update");
    }
  }
  outp = out_surf->pixels;

  if (scaling == 1) {
    for (row = 0; row < height; row++) {
      memcpy(outp, inp, width);
      outp += out_surf->pitch;
      inp += width;
    }
  } else if (scaling == 2) {
    unsigned char *outp2 = outp + out_surf->pitch;
    int skip = (out_surf->pitch - width) << 1;
    int col;
    unsigned char c;
    for (row = 0; row < height; row++) {
      for (col = 0; col < width; col++) {
        c = *(inp++);
        *(outp++) = c;
        *(outp++) = c;
        *(outp2++) = c;
        *(outp2++) = c;
      }
      outp += skip;
      outp2 += skip;
    }
  }
  if (SDL_MUSTLOCK(out_surf)) {
    SDL_UnlockSurface(out_surf);
  }
}

void disp_swapBuffers(void)
{
  if (!disp_DrawingOnSurface) {
    disp_toSurface();
  }

  if (out_surf != screen) {
    SDL_BlitSurface(out_surf, NULL, screen, NULL);
  }
  SDL_UpdateWindowSurface(window);
}

static void disp_toggleFullscreen(void)
{
  if (fullscreen) {
    SDL_SetWindowFullscreen(window, false);
    fullscreen = FALSE;
  } else {
    winwidth = width;
    winheight = height;
    SDL_SetWindowFullscreen(window, true);
    fullscreen = TRUE;
  }
  if (fullscreen) {
    SDL_HideCursor();
  } else {
    SDL_ShowCursor();
  }
}

static void disp_processKey(SDL_Keycode key
#define keymod SDL_GetModState()
)
{
  switch (key) {
    case SDLK_UP:
      handleInput(CMD_PAL_FASTER); break;
    case SDLK_DOWN:
      handleInput(CMD_PAL_SLOWER); break;
    case SDLK_N:
      handleInput(CMD_SKIP); break;
    case SDLK_C:
    case SDLK_PAUSE:
      if ((keymod & SDL_KMOD_CTRL) == 0) break; /* else like SDLK_Q */
    case SDLK_Q:
      handleInput(CMD_QUIT); break;
    case SDLK_ESCAPE:
      if (fullscreen) disp_toggleFullscreen();
      break;
    case SDLK_RETURN:
      if (keymod & SDL_KMOD_ALT) disp_toggleFullscreen();
      break;
    default: break;
  }
#undef keymod
}

void disp_processInput(void) {
  SDL_Event event;

  while ( SDL_PollEvent(&event) ) {
    switch (event.type) {
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
          if (event.button.clicks == 2) {
            disp_toggleFullscreen();
          }
        }
        break;
      case SDL_EVENT_KEY_DOWN:
        disp_processKey(event.key.key);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        /* If full screen resolution is at least twice as large as
         * previous window, then use 2x scaling, else no scaling.
         */
        scaling = fullscreen ?
                  ((winwidth != 0 &&
                    event.window.data1 >= 2 * winwidth) ? 2 : 1) : 1;
        if (width != (event.window.data1 / scaling) ||
            height != (event.window.data2 / scaling)) {
          disp_init(event.window.data1 / scaling,
                    event.window.data2 / scaling,
                    fullscreen
                  );
        }
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
        break;
      case SDL_EVENT_QUIT:
        handleInput(CMD_QUIT);
        break;

      default:
        break;
    }
  }
}

static void disp_freeBuffer(UCHAR **buf)
{
  if (*buf != NULL) {
    free(*buf);
    *buf = NULL;
  }
}

static void disp_reallocBuffer(UCHAR **buf)
{
  disp_freeBuffer(buf);
  *buf = calloc(width * height, 1);
  if (*buf == NULL) {
      printf("Couldn't allocate graphics buffer.\n");
      quit();
  }
}

static void disp_freeSurface(SDL_Surface **surf)
{
  if (*surf != NULL) {
    SDL_DestroySurface(*surf);
    *surf = NULL;
  }
}

static void disp_allocSurface(SDL_Surface **surf)
{
  *surf = SDL_CreateSurface(width*scaling, height*scaling, SDL_PIXELFORMAT_INDEX8);
  if (!(*surf)) fatalSDLError("creating secondary surface");
}

static void disp_allocateOffscreen(void)
{
  /* Free secondary surface */
  if (out_surf != screen) disp_freeSurface(&out_surf);

  {
    /* Create 8 bit surface to draw into. This is needed if pixel
     * formats differ or to respond to SDL_VIDEOEXPOSE events.
     */
    disp_allocSurface(&out_surf);
  }

  if (scaling == 1
      /* Normally need to have offscreen data for expose events,
       */

      ) {
    if (!disp_DrawingOnSurface) {
      disp_freeBuffer(&draw_buf);
    }
    disp_DrawingOnSurface = 1;
  } else {
    disp_DrawingOnSurface = 0;
    disp_reallocBuffer(&draw_buf);
  }
}

void disp_init(int newwidth, int newheight, int flags)
{
  Uint32 videoflags;
  static int inited = 0;

  width = newwidth;
  height = newheight;
  fullscreen = flags ? 1 : 0;

  if (!inited) {
    videoflags = (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);

    if (fullscreen) {
      SDL_HideCursor();
    } else {
      SDL_ShowCursor();
    }

    scaling = 1;

    /* The screen is a destination for SDL_BlitSurface() copies.
     * Nothing is ever directly drawn here.
     */
    window = SDL_CreateWindow("Acidwarp",
                              width*scaling, height*scaling, videoflags);
    if (window == NULL) fatalSDLError("creating SDL window");

    inited = 1;
  } /* !inited */

  /* Raspberry Pi console will set window to size of full screen,
   * and not give a resize event. */
  SDL_GetWindowSize(window, &width, &height);

  /* Needs to be called again after resizing */
  screen = SDL_GetWindowSurface(window);
  if (!screen) fatalSDLError("getting window surface");

  disp_allocateOffscreen();

  /* This may be unnecessary if switching between windowed
   * and full screen mode with the same dimensions. */
  handleInput(CMD_RESIZE);
}
