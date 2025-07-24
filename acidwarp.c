/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android by Matthew Zavislak
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// For Android logging
//#include <android/log.h>

#include "handy.h"
#include "acidwarp.h"
#include "rolnfade.h"
#include "display.h"

#define NUM_IMAGE_FUNCTIONS 40
#define NOAHS_FACE           0

#define LOGO_TIME           10
#define IMAGE_TIME          60

/* there are WAY too many global things here... */
static int ROTATION_DELAY = 30000;
static int show_logo = 1;
static int image_time = LOGO_TIME;
static int disp_flags = 1;
static int draw_flags = DRAW_SCALED;
static int width = 320, height = 200;
UCHAR *buf_graf = NULL;
static int GO = TRUE;
static int SKIP = FALSE;
static int NP = FALSE; /* flag indicates new palette */
static int RESIZE = FALSE;
static int QUIT = FALSE;

/* Prototypes for forward referenced functions */
static void mainLoop(void);

static void timer_quit(void);

#pragma ide diagnostic ignored "cert-err33-c"
void fatalSDLError(const char *msg) {
  fprintf(stderr, "SDL error while %s: %s", msg, SDL_GetError());
  quit();
}

/* Used by drawing code */
void makeShuffledList(int *list, int listSize) {
  int entryNum, r;

  for (entryNum = 0; entryNum < listSize; ++entryNum) {
    list[entryNum] = -1;
  }

  for (entryNum = 0; entryNum < listSize; ++entryNum) {
    do {
      r = RANDOM(listSize);
    } while (list[r] != -1);

    list[r] = entryNum;
  }
}

#define TIMER_INTERVAL (ROTATION_DELAY / 1000)
static struct {
  SDL_Condition *cond;
  SDL_Mutex *mutex;
  SDL_TimerID timer_id;
  bool flag;
} timer_data = {NULL, NULL, 0, false};

static void timer_lock(void) {
  SDL_LockMutex(timer_data.mutex);
}

static void timer_unlock(void) {
  SDL_UnlockMutex(timer_data.mutex);
}

static Uint32 timer_proc(void *userdata, SDL_TimerID timerID, Uint32 interval) {
  unsigned int tmint = TIMER_INTERVAL;
  timer_lock();
  timer_data.flag = true;
  SDL_SignalCondition(timer_data.cond);
  timer_unlock();
  return tmint ? tmint : 1;
}

static void timer_init(void) {
  timer_data.mutex = SDL_CreateMutex();
  if (timer_data.mutex == NULL) {
    fatalSDLError("creating timer mutex");
  }
  timer_data.cond = SDL_CreateCondition();
  if (timer_data.cond == NULL) {
    fatalSDLError("creating timer condition variable");
  }
  timer_data.timer_id = SDL_AddTimer(TIMER_INTERVAL, timer_proc,
                                     timer_data.cond);
  if (timer_data.timer_id == 0) {
    fatalSDLError("adding timer");
  }
}

void quit() {
  QUIT = TRUE;
  timer_quit();
}

static void timer_quit(void) {
  if (timer_data.timer_id != 0) {
    SDL_RemoveTimer(timer_data.timer_id);
    timer_data.timer_id = 0;
  }
  if (timer_data.cond != NULL) {
    SDL_DestroyCondition(timer_data.cond);
    timer_data.cond = 0;
  }
  if (timer_data.mutex != NULL) {
    SDL_DestroyMutex(timer_data.mutex);
    timer_data.mutex = NULL;
  }
}

static void timer_wait(void) {
  timer_lock();
  while (!timer_data.flag) {
    SDL_WaitCondition(timer_data.cond, timer_data.mutex);
  }
  timer_data.flag = false;
  timer_unlock();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
bool HandleAppEvents(void *userdata, SDL_Event *event) {
  Uint32 type = event->type;

//  if (type != SDL_EVENT_KEY_UP) {
//    __android_log_print(ANDROID_LOG_INFO, "acidwarp.c", "HandleAppEvent, type=%d", type);
//  }

  switch (type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_TERMINATING:
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
    case SDL_EVENT_DID_ENTER_BACKGROUND:
      quit();
      return false;

    case SDL_EVENT_LOW_MEMORY:
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
    case SDL_EVENT_DID_ENTER_FOREGROUND:
      return false;

    case SDL_EVENT_KEY_UP:
      SDL_Keycode keycode = event->key.key;
      SDL_Scancode scancode = event->key.scancode;

      //__android_log_print(ANDROID_LOG_INFO, "acidwarp.c", "HandleAppEvent, type=%d, keycode=%d, scancode=%d", type, keycode, scancode);

      if (keycode == SDLK_AC_BACK || scancode == SDL_SCANCODE_ESCAPE) {
        handleInput(CMD_QUIT);
      } else if (scancode == SDL_SCANCODE_RETURN) {
        handleInput(CMD_SKIP);
      } else if (scancode == SDL_SCANCODE_UP) {
        handleInput(CMD_PAL_FASTER);
      } else if (scancode == SDL_SCANCODE_DOWN) {
        handleInput(CMD_PAL_SLOWER);
      } else if (scancode == SDL_SCANCODE_RIGHT) {
        handleInput(CMD_NEWPAL);
      }

      return false;
    default:
      /* No special processing, add it to the event queue */
      return true;
  }
}
#pragma clang diagnostic pop

#pragma ide diagnostic ignored "cert-msc51-cpp"

int main(int argc, char *argv[]) {

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    return -1;
  }

  SDL_SetEventFilter(HandleAppEvents, NULL);
  RANDOMIZE();
  disp_init(width, height, disp_flags);
  timer_init();

  printf("starting main loop\n");

  #pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
  while (!QUIT) {
    mainLoop();
    timer_wait();
  }

  return 0;
}

static void mainLoop(void) {
  static time_t ltime, mtime;
  static enum {
    STATE_INITIAL,
    STATE_NEXT,
    STATE_DISPLAY,
    STATE_FADEOUT
  } state = STATE_INITIAL;

  disp_processInput();

  if (RESIZE) {
    RESIZE = FALSE;

    if (state != STATE_INITIAL) {
      draw_same();
      applyPalette();
    }
  }

  if (SKIP) {
    // In order to keep state management simple, we ignore skip requests unless they
    // are during the display phase.  Then, handle the skip request during the
    // STATE_DISPLAY part of the switch statement below.
    if (state != STATE_DISPLAY) {
      SKIP = FALSE;
    }
  }

  if (NP) {
    if (!show_logo) { newPalette(); }
    NP = FALSE;
  }

  if (QUIT) {
    return;
  }

  switch (state) {
    case STATE_INITIAL:
      draw_init(draw_flags | (show_logo ? DRAW_LOGO : 0));
      initRolNFade(show_logo);

      /* Fall through */
    case STATE_NEXT:
      /* install a new image */
      draw_next();

      if (!show_logo) {
        newPalette();
      }

      SKIP = FALSE;

      ltime = time(NULL);
      mtime = ltime + image_time;
      state = STATE_DISPLAY;
      /* Fall through */
    case STATE_DISPLAY:
      /* rotate the palette for a while */
      if (GO) {
        fadeInAndRotate();
      }

      // Treat SKIP as simply a request to override the timer
      if (SKIP || time(NULL) > mtime) {
        SKIP = FALSE;
        beginFadeOut(show_logo);
        state = STATE_FADEOUT;
      }

      break;
    case STATE_FADEOUT:
      /* fade out */
      if (GO) {
        if (fadeOut()) {
          show_logo = 0;
          image_time = IMAGE_TIME;
          state = STATE_NEXT;
        }
      }
      break;
  }
}

/* ------------------------END MAIN----------------------------------------- */

void handleInput(enum acidwarp_command cmd) {
  switch (cmd) {
    case CMD_SKIP:
      SKIP = TRUE;
      break;
    case CMD_QUIT:
      quit();
      break;
    case CMD_PAL_FASTER:
      ROTATION_DELAY = ROTATION_DELAY - 5000;
      if (ROTATION_DELAY < 0) { ROTATION_DELAY = 0; }
      break;
    case CMD_PAL_SLOWER:
      ROTATION_DELAY = ROTATION_DELAY + 5000;
      break;
    case CMD_RESIZE:
      RESIZE = TRUE;
      break;
    case CMD_NEWPAL:
      NP = TRUE;
      break;
  }
}
