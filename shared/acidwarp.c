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
#ifdef _WIN32
    printf("[EVENT_FILTER] HandleAppEvents() called, event type=%u\n", (unsigned int)event->type);
    fflush(stdout);
#endif
    switch (event->type)
    {
    case SDL_EVENT_TERMINATING:
#ifdef _WIN32
        printf("[EVENT_FILTER] SDL_EVENT_TERMINATING, calling quit(0)\n");
        fflush(stdout);
#endif
        quit(0);
        return false;
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
#ifdef _WIN32
        printf("[EVENT_FILTER] SDL_EVENT_WILL_ENTER_BACKGROUND, pausing\n");
        fflush(stdout);
#endif
        handleinput(CMD_PAUSE);
        return false;
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
#ifdef _WIN32
        printf("[EVENT_FILTER] SDL_EVENT_WILL_ENTER_FOREGROUND, pausing\n");
        fflush(stdout);
#endif
        handleinput(CMD_PAUSE);
        return false;
    default:
        /* No special processing, add it to the event queue */
#ifdef _WIN32
        printf("[EVENT_FILTER] Event type %u passed through to event queue\n", (unsigned int)event->type);
        fflush(stdout);
#endif
        return true;
    }
}

void quit(int retcode)
{
#ifdef _WIN32
  printf("[QUIT] quit() called with retcode=%d\n", retcode);
  fflush(stdout);
  printf("[QUIT] Setting QUIT_MAIN_LOOP=TRUE\n");
  fflush(stdout);
#endif
  QUIT_MAIN_LOOP = TRUE;
#ifdef _WIN32
  printf("[QUIT] QUIT_MAIN_LOOP set, quit() returning\n");
  fflush(stdout);
#endif
}

void fatalSDLError(const char *msg)
{
  fprintf(stderr, "[FATAL] SDL error while %s: %s\n", msg, SDL_GetError());
  fflush(stderr);
#ifdef _WIN32
  printf("[FATAL] Calling quit(-1) due to SDL error\n");
  fflush(stdout);
#endif
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
#ifdef _WIN32
  printf("[TIMER] timer_lock() called\n");
  fflush(stdout);
#endif
  if (SDL_LockMutex(timer_data.mutex) != 0) {
#ifdef _WIN32
    printf("[TIMER] ERROR: Failed to lock mutex\n");
    fflush(stderr);
#endif
    fatalSDLError("locking timer mutex");
  }
#ifdef _WIN32
  printf("[TIMER] timer_lock() succeeded\n");
  fflush(stdout);
#endif
}

static void timer_unlock(void)
{
#ifdef _WIN32
  printf("[TIMER] timer_unlock() called\n");
  fflush(stdout);
#endif
  if (SDL_UnlockMutex(timer_data.mutex) != 0) {
#ifdef _WIN32
    printf("[TIMER] ERROR: Failed to unlock mutex\n");
    fflush(stderr);
#endif
    fatalSDLError("unlocking timer mutex");
  }
#ifdef _WIN32
  printf("[TIMER] timer_unlock() succeeded\n");
  fflush(stdout);
#endif
}

static Uint32 timer_proc(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
  static int call_count = 0;
  unsigned int tmint = TIMER_INTERVAL;

  /* Only log first few calls to avoid spam */
#ifdef _WIN32
  if (call_count < 5) {
    printf("[TIMER] timer_proc() called (count=%d)\n", call_count);
    fflush(stdout);
    call_count++;
  }
#endif

  timer_lock();
  timer_data.flag = true;
  SDL_SignalCondition(timer_data.cond);
  timer_unlock();
  return tmint ? tmint : 1;
}

static void timer_init(void)
{
#ifdef _WIN32
  printf("[TIMER] timer_init() called\n");
  fflush(stdout);
  printf("[TIMER] Creating mutex...\n");
  fflush(stdout);
#endif
  timer_data.mutex = SDL_CreateMutex();
  if (timer_data.mutex == NULL) {
#ifdef _WIN32
    printf("[TIMER] ERROR: Failed to create mutex\n");
    fflush(stderr);
#endif
    fatalSDLError("creating timer mutex");
  }
#ifdef _WIN32
  printf("[TIMER] Mutex created successfully\n");
  fflush(stdout);
  printf("[TIMER] Creating condition variable...\n");
  fflush(stdout);
#endif
  timer_data.cond = SDL_CreateCondition();
  if (timer_data.cond == NULL) {
#ifdef _WIN32
    printf("[TIMER] ERROR: Failed to create condition variable\n");
    fflush(stderr);
#endif
    fatalSDLError("creating timer condition variable");
  }
#ifdef _WIN32
  printf("[TIMER] Condition variable created successfully\n");
  fflush(stdout);
  printf("[TIMER] Adding timer (interval=%u)...\n", TIMER_INTERVAL);
  fflush(stdout);
#endif
  timer_data.timer_id = SDL_AddTimer(TIMER_INTERVAL, timer_proc,
                                     timer_data.cond);
  if (timer_data.timer_id == 0) {
#ifdef _WIN32
    printf("[TIMER] ERROR: Failed to add timer\n");
    fflush(stderr);
#endif
    fatalSDLError("adding timer");
  }
#ifdef _WIN32
  printf("[TIMER] Timer added successfully (timer_id=%u)\n", timer_data.timer_id);
  fflush(stdout);
#endif
}

static void timer_wait(void)
{
#ifdef _WIN32
  printf("[TIMER] timer_wait() entered\n");
  fflush(stdout);
  printf("[TIMER] timer_data.flag=%d before lock\n", timer_data.flag);
  fflush(stdout);
#endif
  timer_lock();
#ifdef _WIN32
  printf("[TIMER] timer_lock() acquired\n");
  fflush(stdout);
  printf("[TIMER] Checking flag: timer_data.flag=%d\n", timer_data.flag);
  fflush(stdout);
#endif
  while (!timer_data.flag) {
#ifdef _WIN32
    printf("[TIMER] Flag is false, waiting on condition variable with timeout...\n");
    fflush(stdout);
    printf("[TIMER] About to call SDL_WaitConditionTimeout()\n");
    fflush(stdout);
#endif

    /* Use timeout-based waiting (16ms ~= 60fps) to allow event processing */
    const Uint32 timeout_ms = 16;
    int wait_result = SDL_WaitConditionTimeout(timer_data.cond, timer_data.mutex, timeout_ms);

#ifdef _WIN32
    printf("[TIMER] SDL_WaitConditionTimeout() returned: %d\n", wait_result);
    fflush(stdout);
#endif

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

#ifdef _WIN32
    printf("[TIMER] timer_data.flag=%d after wait\n", timer_data.flag);
    fflush(stdout);
#endif
  }
#ifdef _WIN32
  printf("[TIMER] Flag is true, clearing it\n");
  fflush(stdout);
#endif
  timer_data.flag = false;
#ifdef _WIN32
  printf("[TIMER] Unlocking mutex\n");
  fflush(stdout);
#endif
  timer_unlock();
#ifdef _WIN32
  printf("[TIMER] timer_wait() exiting\n");
  fflush(stdout);
#endif
}

int main (int argc, char *argv[])
{
#ifdef _WIN32
  printf("[INIT] Starting Acid Warp...\n");
  fflush(stdout);
#endif

  /* Initialize SDL */
#ifdef _WIN32
  printf("[INIT] Initializing SDL...\n");
  fflush(stdout);
#endif
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0 ) {
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    /* SDL 2 docs say this is safe, but SDL 1 docs don't. */
    SDL_Quit();
    return -1;
  }
#ifdef _WIN32
  printf("[INIT] SDL initialized successfully\n");
  fflush(stdout);
#endif

#ifdef __ANDROID__
  // Trap the Android back button, only works on API 30 (Android 11) and earlier
  SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
#endif

#if defined(TARGET_OS_MAC) || defined(TARGET_OS_WINDOWS)
  setupCustomAboutMenu();
#endif

  SDL_SetEventFilter(HandleAppEvents, NULL);

#ifdef _WIN32
  printf("[INIT] Initializing random seed...\n");
  fflush(stdout);
#endif
  RANDOMIZE();

#ifdef _WIN32
  printf("[INIT] Initializing display (%dx%d)...\n", width, height);
  fflush(stdout);
#endif
  disp_init(width, height, disp_flags);
#ifdef _WIN32
  printf("[INIT] Display initialized successfully\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[INIT] Initializing timer...\n");
  fflush(stdout);
#endif
  timer_init();
#ifdef _WIN32
  printf("[INIT] Timer initialized successfully\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[INIT] Entering main loop...\n");
  fflush(stdout);
#endif
  // ReSharper disable once CppDFALoopConditionNotUpdated
  #pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
  int loop_iteration = 0;
  while(!QUIT_MAIN_LOOP) {
    loop_iteration++;
#ifdef _WIN32
    printf("[MAIN] === Main loop iteration %d ===\n", loop_iteration);
    fflush(stdout);
    printf("[MAIN] QUIT_MAIN_LOOP=%d\n", QUIT_MAIN_LOOP);
    fflush(stdout);
#endif
    mainLoop();
#ifdef _WIN32
    printf("[MAIN] mainLoop() returned\n");
    fflush(stdout);
    printf("[MAIN] About to call timer_wait()\n");
    fflush(stdout);
#endif
    timer_wait();
#ifdef _WIN32
    printf("[MAIN] timer_wait() completed\n");
    fflush(stdout);
#endif
  }
#ifdef _WIN32
  printf("[MAIN] Exited main loop after %d iterations\n", loop_iteration);
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[EXIT] Exiting main loop\n");
  fflush(stdout);
#endif
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
#ifdef _WIN32
    printf("[MAIN] First mainLoop iteration\n");
    fflush(stdout);
#endif
    first_iteration = 0;
  }

#ifdef _WIN32
  printf("[MAIN] Current state: %d\n", state);
  fflush(stdout);
  printf("[MAIN] About to call disp_processInput()\n");
  fflush(stdout);
#endif
  disp_processInput();
#ifdef _WIN32
  printf("[MAIN] disp_processInput() completed\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[MAIN] Checking flags: RESIZE=%d, SKIP=%d, NP=%d, GO=%d, LOCK=%d\n",
         RESIZE, SKIP, NP, GO, LOCK);
  fflush(stdout);
#endif

  if (RESIZE) {
#ifdef _WIN32
    printf("[MAIN] Handling RESIZE flag\n");
    fflush(stdout);
#endif
    RESIZE = FALSE;
    if (state != STATE_INITIAL) {
#ifdef _WIN32
      printf("[MAIN] Calling draw_same() and applyPalette()\n");
      fflush(stdout);
#endif
      draw_same();
      applyPalette();
#ifdef _WIN32
      printf("[MAIN] draw_same() and applyPalette() completed\n");
      fflush(stdout);
#endif
    }
  }

  if (SKIP) {
#ifdef _WIN32
    printf("[MAIN] Checking SKIP flag (state=%d)\n", state);
    fflush(stdout);
#endif
    if (state != STATE_DISPLAY) {
#ifdef _WIN32
      printf("[MAIN] Clearing SKIP flag\n");
      fflush(stdout);
#endif
      SKIP = FALSE;
    }
  }

  if(NP) {
#ifdef _WIN32
    printf("[MAIN] Handling NP flag (show_logo=%d)\n", show_logo);
    fflush(stdout);
#endif
    if (!show_logo) {
#ifdef _WIN32
      printf("[MAIN] Calling newPalette()\n");
      fflush(stdout);
#endif
      newPalette();
#ifdef _WIN32
      printf("[MAIN] newPalette() completed\n");
      fflush(stdout);
#endif
    }
    NP = FALSE;
  }

#ifdef _WIN32
  printf("[MAIN] Entering state machine switch (state=%d)\n", state);
  fflush(stdout);
#endif
  switch (state) {
  case STATE_INITIAL:
#ifdef _WIN32
    printf("[MAIN] STATE_INITIAL: Initializing drawing system\n");
    fflush(stdout);
    printf("[MAIN] About to call draw_init()\n");
    fflush(stdout);
#endif
    draw_init(draw_flags | (show_logo ? DRAW_LOGO : 0));
#ifdef _WIN32
    printf("[MAIN] Drawing system initialized\n");
    fflush(stdout);
    printf("[MAIN] About to call initRolNFade()\n");
    fflush(stdout);
#endif
    initRolNFade(show_logo);
#ifdef _WIN32
    printf("[MAIN] initRolNFade() completed\n");
    fflush(stdout);
#endif

    /* Fall through */
  case STATE_NEXT:
    /* install a new image */
#ifdef _WIN32
    printf("[MAIN] STATE_NEXT: Generating next pattern\n");
    fflush(stdout);
    printf("[MAIN] About to call draw_next()\n");
    fflush(stdout);
#endif
    draw_next();
#ifdef _WIN32
    printf("[MAIN] Pattern generated\n");
    fflush(stdout);
#endif

    if (!show_logo) {
#ifdef _WIN32
      printf("[MAIN] Calling newPalette() in STATE_NEXT\n");
      fflush(stdout);
#endif
      newPalette();
#ifdef _WIN32
      printf("[MAIN] newPalette() completed\n");
      fflush(stdout);
#endif
    }

    ltime = time(NULL);
    mtime = ltime + image_time;
#ifdef _WIN32
    printf("[MAIN] Setting display time: ltime=%ld, mtime=%ld, image_time=%d\n",
           (long)ltime, (long)mtime, image_time);
    fflush(stdout);
#endif
    state = STATE_DISPLAY;
#ifdef _WIN32
    printf("[MAIN] Transitioned to STATE_DISPLAY\n");
    fflush(stdout);
#endif
    /* Fall through */
  case STATE_DISPLAY:
    /* rotate the palette for a while */
#ifdef _WIN32
    printf("[MAIN] STATE_DISPLAY: GO=%d\n", GO);
    fflush(stdout);
#endif
    if(GO) {
#ifdef _WIN32
      printf("[MAIN] Calling fadeInAndRotate()\n");
      fflush(stdout);
#endif
      fadeInAndRotate();
#ifdef _WIN32
      printf("[MAIN] fadeInAndRotate() completed\n");
      fflush(stdout);
#endif
    }

    time_t current_time = time(NULL);
    int should_skip = SKIP || (current_time > mtime && !LOCK);
#ifdef _WIN32
    printf("[MAIN] Checking transition: SKIP=%d, current_time=%ld, mtime=%ld, LOCK=%d, should_skip=%d\n",
           SKIP, (long)current_time, (long)mtime, LOCK, should_skip);
    fflush(stdout);
#endif
    if(should_skip) {
#ifdef _WIN32
      printf("[MAIN] Transitioning to STATE_FADEOUT\n");
      fflush(stdout);
#endif
      /* Transition from logo only fades to black,
       * like the first transition in Acidwarp 4.10.
       */
      beginFadeOut(show_logo);
      state = STATE_FADEOUT;
#ifdef _WIN32
      printf("[MAIN] Transitioned to STATE_FADEOUT\n");
      fflush(stdout);
#endif
    }
    break;

  case STATE_FADEOUT:
    /* fade out */
#ifdef _WIN32
    printf("[MAIN] STATE_FADEOUT: GO=%d\n", GO);
    fflush(stdout);
#endif
    if(GO) {
#ifdef _WIN32
      printf("[MAIN] Calling fadeOut()\n");
      fflush(stdout);
#endif
      if (fadeOut()) {
#ifdef _WIN32
        printf("[MAIN] fadeOut() returned true, transitioning to STATE_NEXT\n");
        fflush(stdout);
#endif
        show_logo = 0;
        image_time = PATTERN_TIME;
        state = STATE_NEXT;
      } else {
#ifdef _WIN32
        printf("[MAIN] fadeOut() returned false, staying in STATE_FADEOUT\n");
        fflush(stdout);
#endif
      }
    }
    break;
  }

#ifdef _WIN32
  printf("[MAIN] State machine switch completed, new state=%d\n", state);
  fflush(stdout);
  printf("[MAIN] mainLoop() completing\n");
  fflush(stdout);
#endif
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
