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

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_MAC || TARGET_OS_IOS
#include <CoreFoundation/CoreFoundation.h>
#endif
#endif

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
  fprintf(stderr, "[FATAL] SDL error while %s: %s\n", msg, SDL_GetError());
  fflush(stderr);
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
  unsigned int tmint = TIMER_INTERVAL;

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
  timer_lock();
  while (!timer_data.flag) {
    /* Use timeout-based waiting (16ms ~= 60fps) to allow event processing */
    const Uint32 timeout_ms = 16;
    SDL_WaitConditionTimeout(timer_data.cond, timer_data.mutex, timeout_ms);

    /* Platform-specific event loop processing:
     * - macOS/iOS: Pump NSRunLoop to allow XCUITest to detect idle state
     * - Windows: Pump SDL events to process Windows messages (prevents hanging)
     * - Other platforms: Pump SDL events for general responsiveness */
    #ifdef __APPLE__
    #if TARGET_OS_MAC || TARGET_OS_IOS
    /* Allow NSRunLoop to process events briefly - this helps XCUITest detect idle state */
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, false);
    #endif
    #else
    /* Pump SDL events for general responsiveness on other platforms */
    SDL_PumpEvents();
    #endif
  }
  timer_data.flag = false;
  timer_unlock();
}

int main (int argc, char *argv[])
{
  fprintf(stderr, "[INIT] Acid Warp starting...\n");
  fflush(stderr);

  /* Initialize SDL */
  fprintf(stderr, "[INIT] Initializing SDL...\n");
  fflush(stderr);
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0 ) {
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    /* SDL 2 docs say this is safe, but SDL 1 docs don't. */
    SDL_Quit();
    return -1;
  }
  fprintf(stderr, "[INIT] SDL initialized successfully\n");
  fflush(stderr);

#ifdef __ANDROID__
  // Trap the Android back button, only works on API 30 (Android 11) and earlier
  SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
#endif

#if defined(TARGET_OS_MAC) || defined(TARGET_OS_WINDOWS)
  setupCustomAboutMenu();
#endif

  SDL_SetEventFilter(HandleAppEvents, NULL);

  RANDOMIZE();

  fprintf(stderr, "[INIT] Initializing display...\n");
  fflush(stderr);
  disp_init(width, height, disp_flags);
  fprintf(stderr, "[INIT] Display initialized\n");
  fflush(stderr);

  fprintf(stderr, "[INIT] Initializing timer...\n");
  fflush(stderr);
  timer_init();
  fprintf(stderr, "[INIT] Timer initialized\n");
  fflush(stderr);

  fprintf(stderr, "[INIT] Entering main loop\n");
  fflush(stderr);

  // ReSharper disable once CppDFALoopConditionNotUpdated
  #pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
  while(!QUIT_MAIN_LOOP) {
    mainLoop();
    timer_wait();
  }

  fprintf(stderr, "[EXIT] Main loop exited\n");
  fflush(stderr);
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

  disp_processInput();

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
    fprintf(stderr, "[INIT] Initializing drawing system\n");
    fflush(stderr);
    draw_init(draw_flags | (show_logo ? DRAW_LOGO : 0));
    initRolNFade(show_logo);
    fprintf(stderr, "[INIT] Drawing system initialized, starting pattern display\n");
    fflush(stderr);

    /* Fall through */
  case STATE_NEXT:
    /* install a new image */
    draw_next();

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
