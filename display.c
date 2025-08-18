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
#include <GLES2/gl2.h>

#include "handy.h"
#include "acidwarp.h"
#include "display.h"

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

void disp_setPalette(unsigned char *palette)
{
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
  SDL_GL_SwapWindow(window);
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
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE,
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
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  SDL_GL_SwapWindow(window);
}


void disp_processInput(void) {
  SDL_Event event;

  while ( SDL_PollEvent(&event) > 0 ) {
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
      case SDL_EVENT_DISPLAY_ORIENTATION:
      case SDL_EVENT_WINDOW_RESIZED:
        disp_init(event.window.data1,event.window.data2,fullscreen);
        handleinput(CMD_RESIZE);
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
        display_redraw();
        break;
      case SDL_EVENT_QUIT:
      case SDL_EVENT_TERMINATING:
      case SDL_EVENT_WILL_ENTER_BACKGROUND:
      case SDL_EVENT_DID_ENTER_BACKGROUND:
        handleinput(CMD_QUIT);
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

static void disp_glinit(int width, int height, Uint32 videoflags)
{
  GLuint buffer;
  GLint status;

  /* Vertices consist of point x, y, z, w followed by texture x and y */
  static const GLfloat vertices[] = {
      -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
  };

  /* WebGL 1.0 is based on OpenGL ES 2.0 */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
 
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  window = SDL_CreateWindow("Acidwarp", width, height, videoflags | SDL_WINDOW_OPENGL);
  if (window == NULL) fatalSDLError("creating SDL OpenGL window");
  context = SDL_GL_CreateContext(window);
  if (context == NULL) fatalSDLError("creating OpenGL profile");


  glprogram = glCreateProgram();
  if (glprogram == 0) disp_glerror("glCreateProgram");
  loadShader(glprogram, GL_VERTEX_SHADER, vertex);
  loadShader(glprogram, GL_FRAGMENT_SHADER, fragment);
  glBindAttribLocation(glprogram, 0, "Position");
  glBindAttribLocation(glprogram, 1, "TexPos");
  glLinkProgram(glprogram);
  glGetProgramiv(glprogram, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) disp_glerror("glLinkProgram");
  glUseProgram(glprogram);

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

  /* https://www.opengl.org/discussion_boards/showthread.php/163092-Passing-Multiple-Textures-from-OpenGL-to-GLSL-shader */

  /* 256 x 1 texture used as palette lookup table */
  glUniform1i(glGetUniformLocation(glprogram, "Palette"), 0);
  glActiveTexture(GL_TEXTURE0);
  paltex = disp_newtex();
  glBindTexture(GL_TEXTURE_2D, paltex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  /* 8 bpp indexed colour texture used for image */
  glUniform1i(glGetUniformLocation(glprogram, "IndexTexture"), 1);
  glActiveTexture(GL_TEXTURE1);
  indtex = disp_newtex();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void disp_init(int newwidth, int newheight, int flags)
{
  Uint32 videoflags;
  static int inited = 0;

  width = newwidth;
  height = newheight;
  fullscreen = (flags & DISP_FULLSCREEN) ? 1 : 0;

  if (!inited) {
    videoflags = (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);
    SDL_ShowCursor(!fullscreen);
    disp_glinit(width, height, videoflags);
    inited = 1;
  } /* !inited */

  /* Raspberry Pi console will set window to size of full screen,
   * and not give a resize event. */
  SDL_GetWindowSize(window, &width, &height);

  /* Create or recreate texture and set viewport, eg. when resizing */
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, indtex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0,
               GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
  glViewport(0, 0, width, height);

  disp_allocateOffscreen();
}
