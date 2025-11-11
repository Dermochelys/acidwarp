/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android, iOS / iPadOS, macOS, Linux, Windows by Matthew Zavislak
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

/* SDL 3 compatibility fixes */
#define SDL_INIT_TIMER 0x00000001u
/* SDL 3 changed mutex and condition functions to return void, so we need wrappers */
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
#include "rolnfade.h"
#include "display.h"
#include "AboutMenu.h"

#define LOGO_TIME           10
#define PATTERN_TIME        60

/* there are WAY too many global things here... */
static int ROTATION_DELAY = 30000;
static int show_logo = 1;
static int image_time = LOGO_TIME;
static int disp_flags = 0;
static int draw_flags = DRAW_FLOAT | DRAW_SCALED;
static int width = 1280, height = 800;
static int GO = TRUE;
static int SKIP = FALSE;
static int NP = FALSE; /* flag indicates new palette */
static int LOCK = FALSE; /* flag indicates don't change to next image */
static int RESIZE = FALSE;
static int QUIT_MAIN_LOOP = FALSE;

/* Prototypes for forward referenced functions */
static void mainLoop(void);

bool HandleAppEvents(void *userdata, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_TERMINATING:
        quit(0);
        return false;
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        handleinput(CMD_PAUSE);
        return false;
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
        handleinput(CMD_PAUSE);
        return false;
    default:
        /* No special processing, add it to the event queue */
        return true;
    }
}

void quit(int retcode)
{
  QUIT_MAIN_LOOP = TRUE;
}

void fatalSDLError(const char *msg)
{
  fprintf(stderr, "SDL error while %s: %s", msg, SDL_GetError());
  quit(-1);
}

/* Used by both ordinary and Emscripten worker drawing code */
void makeShuffledList(int *list, int listSize)
{
  int entryNum, r;

  for (entryNum = 0; entryNum < listSize; ++entryNum)
    list[entryNum] = -1;

  for (entryNum = 0; entryNum < listSize; ++entryNum)
    {
      do
        r = RANDOM(listSize);
      while (list[r] != -1);

      list[r] = entryNum;
    }
}

#define TIMER_INTERVAL (ROTATION_DELAY / 1000)
static struct {
  SDL_Condition *cond;
  SDL_Mutex *mutex;
  SDL_TimerID timer_id;
  bool flag;
} timer_data = { NULL, NULL, 0, false };

static void timer_lock(void)
{
  if (SDL_LockMutex(timer_data.mutex) != 0) {
    fatalSDLError("locking timer mutex");
  }
}

static void timer_unlock(void)
{
  if (SDL_UnlockMutex(timer_data.mutex) != 0) {
    fatalSDLError("unlocking timer mutex");
  }
}

static Uint32 timer_proc(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
  static int call_count = 0;
  unsigned int tmint = TIMER_INTERVAL;

  /* Only log first few calls to avoid spam */
  if (call_count < 5) {
    printf("[TIMER] timer_proc() called (count=%d)\n", call_count);
    fflush(stdout);
    call_count++;
  }

  timer_lock();
  timer_data.flag = true;
  SDL_SignalCondition(timer_data.cond);
  timer_unlock();
  return tmint ? tmint : 1;
}

static void timer_init(void)
{
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

static void timer_wait(void)
{
  printf("[TIMER] timer_wait() entered\n");
  fflush(stdout);
  timer_lock();
  printf("[TIMER] timer_lock() acquired\n");
  fflush(stdout);
  while (!timer_data.flag) {
    printf("[TIMER] Waiting on condition variable...\n");
    fflush(stdout);
    SDL_WaitCondition(timer_data.cond, timer_data.mutex);
    printf("[TIMER] Woke up from SDL_WaitCondition\n");
    fflush(stdout);
  }
  timer_data.flag = false;
  printf("[TIMER] Unlocking mutex\n");
  fflush(stdout);
  timer_unlock();
  printf("[TIMER] timer_wait() exiting\n");
  fflush(stdout);
}

int main (int argc, char *argv[])
{
  printf("[INIT] Starting Acid Warp...\n");
  fflush(stdout);

  /* Initialize SDL */
  printf("[INIT] Initializing SDL...\n");
  fflush(stdout);
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0 ) {
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    /* SDL 2 docs say this is safe, but SDL 1 docs don't. */
    SDL_Quit();
    return -1;
  }
  printf("[INIT] SDL initialized successfully\n");
  fflush(stdout);

#ifdef __ANDROID__
  // Trap the Android back button, only works on API 30 (Android 11) and earlier
  SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
#endif

#if defined(TARGET_OS_MAC) || defined(TARGET_OS_WINDOWS)
  setupCustomAboutMenu();
#endif

  SDL_SetEventFilter(HandleAppEvents, NULL);

  printf("[INIT] Initializing random seed...\n");
  fflush(stdout);
  RANDOMIZE();

  printf("[INIT] Initializing display (%dx%d)...\n", width, height);
  fflush(stdout);
  disp_init(width, height, disp_flags);
  printf("[INIT] Display initialized successfully\n");
  fflush(stdout);

  printf("[INIT] Initializing timer...\n");
  fflush(stdout);
  timer_init();
  printf("[INIT] Timer initialized successfully\n");
  fflush(stdout);

  printf("[INIT] Entering main loop...\n");
  fflush(stdout);
  // ReSharper disable once CppDFALoopConditionNotUpdated
  #pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
  while(!QUIT_MAIN_LOOP) {
    mainLoop();
    printf("[MAIN] About to call timer_wait()\n");
    fflush(stdout);
    timer_wait();
    printf("[MAIN] timer_wait() completed\n");
    fflush(stdout);
  }

  printf("[EXIT] Exiting main loop\n");
  fflush(stdout);
  return 0;
}

static void mainLoop(void)
{
  static time_t ltime, mtime;
  static enum {
    STATE_INITIAL,
    STATE_NEXT,
    STATE_DISPLAY,
    STATE_FADEOUT
  } state = STATE_INITIAL;
  static int first_iteration = 1;

  if (first_iteration) {
    printf("[MAIN] First mainLoop iteration\n");
    fflush(stdout);
    first_iteration = 0;
  }

  printf("[MAIN] About to call disp_processInput()\n");
  fflush(stdout);
  disp_processInput();
  printf("[MAIN] disp_processInput() completed\n");
  fflush(stdout);

  if (RESIZE) {
    RESIZE = FALSE;
    if (state != STATE_INITIAL) {
      draw_same();
      applyPalette();
    }
  }

  if (SKIP) {
    if (state != STATE_DISPLAY) {
      SKIP = FALSE;
    }
  }

  if(NP) {
    if (!show_logo) newPalette();
    NP = FALSE;
  }

  switch (state) {
  case STATE_INITIAL:
    printf("[MAIN] STATE_INITIAL: Initializing drawing system\n");
    fflush(stdout);
    draw_init(draw_flags | (show_logo ? DRAW_LOGO : 0));
    printf("[MAIN] Drawing system initialized\n");
    fflush(stdout);
    initRolNFade(show_logo);

    /* Fall through */
  case STATE_NEXT:
    /* install a new image */
    printf("[MAIN] STATE_NEXT: Generating next pattern\n");
    fflush(stdout);
    draw_next();
    printf("[MAIN] Pattern generated\n");
    fflush(stdout);

    if (!show_logo) {
      newPalette();
    }

    ltime = time(NULL);
    mtime = ltime + image_time;
    state = STATE_DISPLAY;
    /* Fall through */
  case STATE_DISPLAY:
    /* rotate the palette for a while */
    if(GO) {
      fadeInAndRotate();
    }

    if(SKIP || (time(NULL) > mtime && !LOCK)) {
      /* Transition from logo only fades to black,
       * like the first transition in Acidwarp 4.10.
       */
      beginFadeOut(show_logo);
      state = STATE_FADEOUT;
    }
    break;

  case STATE_FADEOUT:
    /* fade out */
    if(GO) {
      if (fadeOut()) {
        show_logo = 0;
        image_time = PATTERN_TIME;
        state = STATE_NEXT;
      }
    }
    break;
  }

  printf("[MAIN] mainLoop() completing\n");
  fflush(stdout);
}

/* ------------------------END MAIN----------------------------------------- */

void handleinput(enum acidwarp_command cmd)
{
  switch(cmd)
    {
    case CMD_PAUSE:
      if (GO) GO = FALSE;
      else GO = TRUE;
      break;
    case CMD_SKIP:
      SKIP = TRUE;
      break;
    case CMD_QUIT:
      quit(0);
      break;
    case CMD_NEWPAL:
      NP = TRUE;
      break;
    case CMD_LOCK:
      if (LOCK) LOCK = FALSE;
      else LOCK = TRUE;
      break;
    case CMD_PAL_FASTER:
      ROTATION_DELAY = ROTATION_DELAY - 5000;
      if (ROTATION_DELAY < 0) ROTATION_DELAY = 0;
      break;
    case CMD_PAL_SLOWER:
      ROTATION_DELAY = ROTATION_DELAY + 5000;
      break;
    case CMD_RESIZE:
      RESIZE = TRUE;
      break;
    }
}
