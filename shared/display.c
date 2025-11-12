/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android, iOS / iPadOS, macOS, Linux, Windows by Matthew Zavislak
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>

/* SDL 3 changed cursor API */
static inline void SDL_ShowCursor_compat(int toggle) {
    if (toggle) {
        SDL_ShowCursor();
    } else {
        SDL_HideCursor();
    }
}
#define SDL_ShowCursor SDL_ShowCursor_compat
/* SDL 3 compatibility - only need what's not provided by oldnames.h */

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#include <OpenGLES/ES2/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
#elif _WIN32
#include <GL/glew.h>
#else
#include <GLES2/gl2.h>
#endif

#include "handy.h"
#include "acidwarp.h"
#include "display.h"
#include "remote_overlay.h"

static SDL_Window *window = NULL;

SDL_GLContext context;
GLuint indtex, paltex, glprogram;

const GLchar vertex[] =
    "#version 100\n"
    "precision mediump float;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexPos;\n"
    "varying vec2 TexCoord0;\n"

    "void main()\n"
    "{\n"
        "gl_Position = Position;\n"
        "TexCoord0 = TexPos;\n"
    "}\0";

const GLchar fragment[] =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform sampler2D Palette;\n"
    "uniform sampler2D IndexTexture;\n"
    // Texture coordinates are passed from vertex shader
    "varying vec2 TexCoord0;\n"

    "void main()\n"
    "{\n"
      // Look up pixel in 8 bpp indexed colour image texture
      "vec4 myindex = texture2D(IndexTexture, TexCoord0);\n"
      // Read RGBA value for that pixel from palette texture
      "gl_FragColor = texture2D(Palette, vec2(myindex.r, 0.0));\n"
    "}\0";
static UCHAR *draw_buf = NULL;
static int fullscreen = 0;
static int width, height;
static int is_software_renderer = 0;

/* Single click debouncing - delay processing until double-click window expires */
static Uint64 pending_click_time = 0;
static int pending_click_x = 0;
static int pending_click_y = 0;
static const Uint64 DOUBLE_CLICK_DELAY_MS = 300; /* Wait 300ms to see if double-click occurs */

static int getInternalFormat(void) {
#ifdef __APPLE__
#if TARGET_OS_IOS
return GL_LUMINANCE;
#else
return GL_R8;
#endif
#elif _WIN32
return GL_R8;
#else
return GL_LUMINANCE;
#endif
}

static int getFormat(void) {
#ifdef __APPLE__
#if TARGET_OS_IOS
return GL_LUMINANCE;
#else
return GL_RED;
#endif
#elif _WIN32
return GL_RED;
#else
return GL_LUMINANCE;
#endif
}

void disp_setPalette(unsigned char *palette)
{
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glViewport(0, 0, width, height);
  glGetIntegerv(GL_VIEWPORT, viewport);

  static GLubyte glcolors[256 * 4];
  int i;

  for (i = 0; i < 256; i++) {
      glcolors[i*4+0] = palette[i*3+0] << 2;
      glcolors[i*4+1] = palette[i*3+1] << 2;
      glcolors[i*4+2] = palette[i*3+2] << 2;
      glcolors[i*4+3] = 255;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, paltex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA,
                  GL_UNSIGNED_BYTE, glcolors);

  glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  /* Render remote overlay on top if visible */
  remote_overlay_render_if_visible(width, height);

  /* Render dim feedback if active */
  remote_overlay_render_dim(width, height);

  if (is_software_renderer) {
    glFinish();  /* For software renderers, wait for all commands to complete */
  } else {
    SDL_GL_SwapWindow(window);
  }
}

void disp_beginUpdate(UCHAR **p, unsigned int *pitch,
                      unsigned int *w, unsigned int *h)
{
  *p = draw_buf;
  *pitch = width;
  *w = width;
  *h = height;
}

void disp_finishUpdate(void)
{
}

void disp_swapBuffers(void)
{
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, indtex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, getFormat(),
                  GL_UNSIGNED_BYTE, draw_buf);
}

static void disp_toggleFullscreen(void)
{
  if (fullscreen) {
    SDL_SetWindowFullscreen(window, 0);
    fullscreen = FALSE;
  } else {
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    fullscreen = TRUE;
  }
  SDL_ShowCursor(!fullscreen);
}

static void disp_processKey(
                            SDL_Keycode key
#define keymod SDL_GetModState()
)
{
  switch (key) {
    case SDLK_UP: handleinput(CMD_PAL_FASTER); break;
    case SDLK_DOWN: handleinput(CMD_PAL_SLOWER); break;
    case SDLK_P: handleinput(CMD_PAUSE); break;
    case SDLK_N:
      handleinput(CMD_SKIP); break;
    case SDLK_C:
    case SDLK_PAUSE:
      if ((keymod & SDL_KMOD_CTRL) == 0) break; /* else like SDLK_Q */
    case SDLK_AC_BACK:
    case SDLK_Q:
      handleinput(CMD_QUIT); break;
    case SDLK_RIGHT:
    case SDLK_K:
      handleinput(CMD_NEWPAL); break;
    case SDLK_LEFT:
    case SDLK_L:
      handleinput(CMD_LOCK); break;
    case SDLK_ESCAPE:
      if (fullscreen) {
        disp_toggleFullscreen();
      } else {
        handleinput(CMD_QUIT);
      }
      break;
    case SDLK_RETURN:
      if (keymod & SDL_KMOD_ALT) {
        disp_toggleFullscreen();
      } else {
        handleinput(CMD_SKIP);
      }
      break;
    default: break;
  }
#undef keymod
}

static void display_redraw(void)
{
#ifdef _WIN32
  printf("[REDRAW] display_redraw() started\n");
  fflush(stdout);
#endif
  glClear(GL_COLOR_BUFFER_BIT);
#ifdef _WIN32
  printf("[REDRAW] glClear() completed\n");
  fflush(stdout);
#endif
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#ifdef _WIN32
  printf("[REDRAW] glDrawArrays() completed\n");
  fflush(stdout);
#endif
  remote_overlay_render_if_visible(width, height);
#ifdef _WIN32
  printf("[REDRAW] remote_overlay_render_if_visible() completed\n");
  fflush(stdout);
#endif
  remote_overlay_render_dim(width, height);
#ifdef _WIN32
  printf("[REDRAW] remote_overlay_render_dim() completed\n");
  fflush(stdout);
  if (is_software_renderer) {
    printf("[REDRAW] About to call glFinish() for software renderer...\n");
    fflush(stdout);
  } else {
    printf("[REDRAW] About to call SDL_GL_SwapWindow()...\n");
    fflush(stdout);
  }
#endif
  if (is_software_renderer) {
    glFinish();  /* For software renderers, wait for all commands to complete */
  } else {
    SDL_GL_SwapWindow(window);
  }
#ifdef _WIN32
  if (is_software_renderer) {
    printf("[REDRAW] glFinish() completed\n");
  } else {
    printf("[REDRAW] SDL_GL_SwapWindow() completed\n");
  }
  fflush(stdout);
#endif
}


void disp_processInput(void) {
  SDL_Event event;

#ifdef _WIN32
  printf("[INPUT] disp_processInput() entered\n");
  fflush(stdout);
#endif

  /* Check if pending single click should be processed */
  if (pending_click_time != 0) {
#ifdef _WIN32
    printf("[INPUT] Checking pending click (time=%llu)\n", (unsigned long long)pending_click_time);
    fflush(stdout);
    printf("[INPUT] About to call SDL_GetTicks() for pending click check\n");
    fflush(stdout);
#endif
    Uint64 current_time = SDL_GetTicks();
#ifdef _WIN32
    printf("[INPUT] SDL_GetTicks() returned: %llu\n", (unsigned long long)current_time);
    fflush(stdout);
#endif
    if (current_time - pending_click_time >= DOUBLE_CLICK_DELAY_MS) {
#ifdef _WIN32
      printf("[INPUT] Processing pending single click\n");
      fflush(stdout);
#endif
      /* Enough time has passed - process as single click */
      if (remote_overlay_is_visible()) {
#ifdef _WIN32
        printf("[INPUT] Remote overlay is visible, handling click\n");
        fflush(stdout);
#endif
        int result = remote_overlay_handle_click(pending_click_x, pending_click_y, width, height);
        if (result == -1) {
          /* Click was outside remote bounds - hide it */
          remote_overlay_hide();
        }
        /* result == 0: click on remote but not button, keep showing */
        /* result == 1: button clicked, keep showing */
      } else {
#ifdef _WIN32
        printf("[INPUT] Remote overlay not visible, showing it\n");
        fflush(stdout);
#endif
        /* Overlay not visible - show it on click */
        remote_overlay_show();
      }
      pending_click_time = 0; /* Clear pending click */
#ifdef _WIN32
      printf("[INPUT] Pending click processed\n");
      fflush(stdout);
#endif
    }
  } else {
#ifdef _WIN32
    printf("[INPUT] No pending click to process\n");
    fflush(stdout);
#endif
  }

#ifdef _WIN32
  printf("[INPUT] Starting SDL_PollEvent() loop\n");
  fflush(stdout);
#endif
  int event_count = 0;
  while ( SDL_PollEvent(&event) > 0 ) {
    event_count++;
#ifdef _WIN32
    printf("[INPUT] SDL_PollEvent() returned event #%d, type=%u\n", event_count, (unsigned int)event.type);
    fflush(stdout);
#endif
    switch (event.type) {
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_MOUSE_BUTTON_DOWN\n");
        fflush(stdout);
#endif
        if (event.button.button == SDL_BUTTON_LEFT) {
#ifdef _WIN32
          printf("[INPUT] Left mouse button, clicks=%d\n", event.button.clicks);
          fflush(stdout);
#endif
          if (event.button.clicks == 2) {
#ifdef _WIN32
            printf("[INPUT] Double click detected\n");
            fflush(stdout);
#endif
            /* Double click detected - cancel pending single click and toggle fullscreen */
            pending_click_time = 0; /* Cancel pending single click */
            /* Prevent fullscreen toggle if clicking within remote overlay bounds */
            if (!remote_overlay_is_visible() ||
                !remote_overlay_is_point_inside(event.button.x, event.button.y, width, height)) {
#ifdef _WIN32
              printf("[INPUT] Toggling fullscreen\n");
              fflush(stdout);
#endif
              disp_toggleFullscreen();
            }
          } else if (event.button.clicks == 1) {
#ifdef _WIN32
            printf("[INPUT] Single click, storing pending state\n");
            fflush(stdout);
            /* Single click - store in pending state and wait for potential double-click */
            printf("[INPUT] About to call SDL_GetTicks() for single click\n");
            fflush(stdout);
#endif
            pending_click_time = SDL_GetTicks();
#ifdef _WIN32
            printf("[INPUT] SDL_GetTicks() returned: %llu\n", (unsigned long long)pending_click_time);
            fflush(stdout);
#endif
            pending_click_x = event.button.x;
            pending_click_y = event.button.y;
          }
        }
        break;
      case SDL_EVENT_KEY_DOWN:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_KEY_DOWN, key=%d\n", (int)event.key.key);
        fflush(stdout);
#endif
        disp_processKey(event.key.key);
#ifdef _WIN32
        printf("[INPUT] disp_processKey() completed\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_DISPLAY_ORIENTATION:
      case SDL_EVENT_WINDOW_RESIZED:
#ifdef _WIN32
        printf("[INPUT] Processing window resize event\n");
        fflush(stdout);
#endif
        disp_init(event.window.data1,event.window.data2,fullscreen);
        handleinput(CMD_RESIZE);
#ifdef _WIN32
        printf("[INPUT] Window resize handled\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_WINDOW_EXPOSED\n");
        fflush(stdout);
#endif
        display_redraw();
#ifdef _WIN32
        printf("[INPUT] display_redraw() completed\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_WINDOW_HIDDEN:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_WINDOW_HIDDEN\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_WINDOW_SHOWN:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_WINDOW_SHOWN\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_WINDOW_MINIMIZED:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_WINDOW_MINIMIZED\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_WINDOW_DESTROYED:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_WINDOW_DESTROYED\n");
        fflush(stdout);
#endif
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_WINDOW_CLOSE_REQUESTED\n");
        fflush(stdout);
#endif
        handleinput(CMD_QUIT);
        break;
      case SDL_EVENT_QUIT:
#ifdef _WIN32
        printf("[INPUT] Processing SDL_EVENT_QUIT\n");
        fflush(stdout);
#endif
        handleinput(CMD_QUIT);
#ifdef _WIN32
        printf("[INPUT] CMD_QUIT handled\n");
        fflush(stdout);
#endif
        break;
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS)
      /* Only quit on background/termination events on mobile platforms */
      case SDL_EVENT_TERMINATING:
      case SDL_EVENT_WILL_ENTER_BACKGROUND:
      case SDL_EVENT_DID_ENTER_BACKGROUND:
#ifdef _WIN32
        printf("[INPUT] Processing mobile platform termination event\n");
        fflush(stdout);
#endif
        handleinput(CMD_QUIT);
        break;
#endif

      default:
#ifdef _WIN32
        printf("[INPUT] Unknown event type: %u\n", (unsigned int)event.type);
        fflush(stdout);
#endif
        break;
    }
  }
#ifdef _WIN32
  printf("[INPUT] SDL_PollEvent() loop completed, processed %d events\n", event_count);
  fflush(stdout);
  printf("[INPUT] disp_processInput() exiting\n");
  fflush(stdout);
#endif
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
      quit(-1);
  }
}

static void disp_allocateOffscreen(void)
{
  /* Drawing must not be happening in the background
   * while the memory being drawn to gets reallocated!
   */
  draw_abort();
  disp_reallocBuffer(&draw_buf);
}

static void disp_glerror(char *s)
{
  GLenum err;
  fprintf(stderr, "OpenGL error at %s. Error log follows:", s);
  while((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%x\n", err);
  }
  exit(-1);
}

static GLuint disp_newtex(void)
{
    GLuint texname;

    glGenTextures(1, &texname);

    glBindTexture(GL_TEXTURE_2D, texname);

    /* Needed for GL_NEAREST sampling on non power of 2 (NPOT) textures
     * in OpenGL ES 2.0 and WebGL.
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    /* Setting GL_TEXTURE_MAG_FILTER to nearest prevents image defects */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texname;
}

static GLuint loadShader(GLuint program, GLenum type,
                         const GLchar *shaderSrc) {
    GLint compile_status;
    GLuint shader;
    shader = glCreateShader(type);
    if (shader == 0) disp_glerror("glCreateShader");
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
      GLsizei loglen = 0;
      GLchar infolog[1024];
      printf("OpenGL error: %s shader failed to compile. Info log follows:\n",
             (type == GL_VERTEX_SHADER) ? "vertex" : "fragment");
      glGetShaderInfoLog(shader, sizeof(infolog), &loglen, infolog);
      fwrite(infolog, loglen, 1, stderr);
      quit(-1);
    }
    glAttachShader(program, shader);
    return 0;
}

static void setAttributesForGLES(void) {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
}

static void setAttributesForGL(void) {
  // OpenGL 4.1 includes full compatibility with the OpenGL ES 2.0 API
  // through the GL_ARB_ES2_compatibility extension

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

static void disp_glinit(int width, int height, Uint32 videoflags)
{
  GLuint buffer;
#if !TARGET_OS_IOS
  GLuint vao;
#endif
  GLint status;

#ifdef _WIN32
  printf("[GL] Initializing OpenGL...\n");
  fflush(stdout);
#endif

  /* Vertices consist of point x, y, z, w followed by texture x and y */
  static const GLfloat vertices[] = {
      -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
  };

#ifdef __APPLE__
  if (TARGET_OS_IOS) {
#ifdef _WIN32
    printf("[GL] Setting attributes for OpenGL ES\n");
    fflush(stdout);
#endif
    setAttributesForGLES();
  } else {
#ifdef _WIN32
    printf("[GL] Setting attributes for OpenGL\n");
    fflush(stdout);
#endif
    setAttributesForGL();
  }
#elif _WIN32
  printf("[GL] Setting attributes for OpenGL (Windows)\n");
  fflush(stdout);
  setAttributesForGL();
#else
#ifdef _WIN32
  printf("[GL] Setting attributes for OpenGL ES\n");
  fflush(stdout);
#endif
  setAttributesForGLES();
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

#ifdef _WIN32
  printf("[GL] Creating SDL window...\n");
  fflush(stdout);
#endif
  window = SDL_CreateWindow("Acid Warp", width, height, videoflags | SDL_WINDOW_OPENGL);
  if (window == NULL) fatalSDLError("creating SDL OpenGL window");
#ifdef _WIN32
  printf("[GL] SDL window created successfully\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[GL] Creating OpenGL context...\n");
  fflush(stdout);
#endif
  context = SDL_GL_CreateContext(window);
  if (context == NULL) fatalSDLError("creating OpenGL profile");
#ifdef _WIN32
  printf("[GL] OpenGL context created successfully\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[GL] Making OpenGL context current...\n");
  fflush(stdout);
#endif
  if (SDL_GL_MakeCurrent(window, context) < 0) {
    fatalSDLError("making OpenGL context current");
  }
#ifdef _WIN32
  printf("[GL] OpenGL context is now current\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[GL] Initializing GLEW...\n");
  fflush(stdout);
  GLenum glew_err = glewInit();
  if (glew_err != GLEW_OK) {
    fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(glew_err));
    quit(-1);
  }
  /* glewInit() may generate a GL_INVALID_ENUM error in core profile - clear it */
  glGetError();
  printf("[GL] GLEW initialized successfully\n");
  fflush(stdout);

  /* Verify OpenGL version and function pointers */
  printf("[GL] Verifying OpenGL context...\n");
  fflush(stdout);
  const GLubyte* version = glGetString(GL_VERSION);
  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* vendor = glGetString(GL_VENDOR);
  if (version) printf("[GL]   Version: %s\n", version);
  if (renderer) printf("[GL]   Renderer: %s\n", renderer);
  if (vendor) printf("[GL]   Vendor: %s\n", vendor);
  fflush(stdout);

  /* Check if critical function pointers are available */
  if (!glCreateProgram) {
    fprintf(stderr, "[GL] ERROR: glCreateProgram function not available!\n");
    quit(-1);
  }
  if (!glCreateShader) {
    fprintf(stderr, "[GL] ERROR: glCreateShader function not available!\n");
    quit(-1);
  }
  printf("[GL] OpenGL context verification complete\n");
  fflush(stdout);

  /* Detect software renderers to work around swap buffer issues in headless/CI environments */
  if (renderer && (strstr((const char*)renderer, "Microsoft Basic Render Driver") ||
                   strstr((const char*)renderer, "llvmpipe") ||
                   strstr((const char*)renderer, "softpipe") ||
                   strstr((const char*)renderer, "Software Rasterizer"))) {
    is_software_renderer = 1;
    printf("[GL] Detected software renderer - will use glFinish() instead of buffer swap...\n");
    fflush(stdout);
    if (SDL_GL_SetSwapInterval(0) < 0) {
      printf("[GL] Warning: Failed to disable VSync: %s\n", SDL_GetError());
    } else {
      printf("[GL] VSync disabled successfully\n");
    }
    fflush(stdout);
  }
#endif

#ifdef _WIN32
  printf("[GL] Creating shader program...\n");
  fflush(stdout);
#endif
  glprogram = glCreateProgram();
  if (glprogram == 0) disp_glerror("glCreateProgram");

#ifdef _WIN32
  printf("[GL] Loading vertex shader...\n");
  fflush(stdout);
#endif
  loadShader(glprogram, GL_VERTEX_SHADER, vertex);

#ifdef _WIN32
  printf("[GL] Loading fragment shader...\n");
  fflush(stdout);
#endif
  loadShader(glprogram, GL_FRAGMENT_SHADER, fragment);

#ifdef _WIN32
  printf("[GL] Binding attributes...\n");
  fflush(stdout);
#endif
  glBindAttribLocation(glprogram, 0, "Position");
  glBindAttribLocation(glprogram, 1, "TexPos");

#ifdef _WIN32
  printf("[GL] Linking shader program...\n");
  fflush(stdout);
#endif
  glLinkProgram(glprogram);
  glGetProgramiv(glprogram, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) disp_glerror("glLinkProgram");

#ifdef _WIN32
  printf("[GL] Using shader program...\n");
  fflush(stdout);
#endif
  glUseProgram(glprogram);

#if TARGET_OS_IOS
  // Must set up branch this way for hybrid iOS/iPadOS/macOS build
#elif TARGET_OS_MAC
  /* Core Profile requires VAO */
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
#elif _WIN32
  /* Core Profile requires VAO */
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
#endif

#ifdef _WIN32
  printf("[GL] Creating vertex buffers...\n");
  fflush(stdout);
#endif
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
                        (char *)&vertices[6] - (char *)&vertices[0], 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                        (char *)&vertices[6] - (char *)&vertices[0],
                        /* Why is this parameter declared as a pointer? */
                        (void *)((char *)&vertices[4] - (char *)&vertices[0]));
#ifdef _WIN32
  printf("[GL] Vertex buffers configured\n");
  fflush(stdout);
#endif

  /* https://www.opengl.org/discussion_boards/showthread.php/163092-Passing-Multiple-Textures-from-OpenGL-to-GLSL-shader */

  /* 256 x 1 texture used as palette lookup table */
#ifdef _WIN32
  printf("[GL] Creating palette texture...\n");
  fflush(stdout);
#endif
  glUniform1i(glGetUniformLocation(glprogram, "Palette"), 0);
  glActiveTexture(GL_TEXTURE0);
  paltex = disp_newtex();
  glBindTexture(GL_TEXTURE_2D, paltex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#ifdef _WIN32
  printf("[GL] Palette texture created\n");
  fflush(stdout);
#endif

  /* 8 bpp indexed colour texture used for image */
#ifdef _WIN32
  printf("[GL] Creating index texture...\n");
  fflush(stdout);
#endif
  glUniform1i(glGetUniformLocation(glprogram, "IndexTexture"), 1);
  glActiveTexture(GL_TEXTURE1);
  indtex = disp_newtex();
#ifdef _WIN32
  printf("[GL] Index texture created\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[GL] Setting pixel store parameters...\n");
  fflush(stdout);
#endif
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#ifdef _WIN32
  printf("[GL] Setting clear color...\n");
  fflush(stdout);
#endif
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef _WIN32
  printf("[GL] OpenGL initialization complete\n");
  fflush(stdout);
#endif
}

void disp_init(int newwidth, int newheight, int flags)
{
  Uint32 videoflags;
  static int inited = 0;

#ifdef _WIN32
  printf("[DISP] disp_init called (%dx%d, flags=%d)\n", newwidth, newheight, flags);
  fflush(stdout);
#endif

  width = newwidth;
  height = newheight;
  fullscreen = (flags & DISP_FULLSCREEN) ? 1 : 0;

  if (!inited) {
#ifdef _WIN32
    printf("[DISP] First time initialization\n");
    fflush(stdout);
#endif
    videoflags = (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);
    SDL_ShowCursor(!fullscreen);
    disp_glinit(width, height, videoflags);
#ifdef _WIN32
    printf("[DISP] Initializing remote overlay...\n");
    fflush(stdout);
#endif
    remote_overlay_init();
#ifdef _WIN32
    printf("[DISP] Remote overlay initialized\n");
    fflush(stdout);
#endif
    inited = 1;
  } /* !inited */

  /* Raspberry Pi console will set window to size of full screen,
   * and not give a resize event. */
#ifdef _WIN32
  printf("[DISP] Getting window size...\n");
  fflush(stdout);
#endif
  SDL_GetWindowSize(window, &width, &height);
#ifdef _WIN32
  printf("[DISP] Window size: %dx%d\n", width, height);
  fflush(stdout);
#endif

  /* Create or recreate texture and set viewport, eg. when resizing */
#ifdef _WIN32
  printf("[DISP] Setting up textures and viewport...\n");
  fflush(stdout);
#endif
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, indtex);
  glTexImage2D(GL_TEXTURE_2D, 0, getInternalFormat(), width, height, 0,
               getFormat(), GL_UNSIGNED_BYTE, NULL);
  glViewport(0, 0, width, height);
#ifdef _WIN32
  printf("[DISP] Textures and viewport configured\n");
  fflush(stdout);
#endif

#ifdef _WIN32
  printf("[DISP] Allocating offscreen buffer...\n");
  fflush(stdout);
#endif
  disp_allocateOffscreen();
#ifdef _WIN32
  printf("[DISP] Offscreen buffer allocated\n");
  fflush(stdout);
#endif
}
