/* Remote Control Overlay for Acid Warp
 * Ported to Android, iOS / iPadOS, macOS, Linux, Windows by Matthew Zavislak
 */

#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#include <OpenGLES/ES2/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
#elif _WIN32
#include <windows.h>
#include <GL/glew.h>
#else
#include <GLES2/gl2.h>
#endif

#include "handy.h"
#include "remote_overlay.h"
#include "acidwarp.h"

/* Overlay state */
static int overlay_visible = 0;
static int remote_width = 0;
static int remote_height = 0;
static float remote_x = 0.0f;  /* Center X in window coordinates */
static float remote_y = 0.0f;  /* Center Y in window coordinates */

/* Button press feedback - dim entire screen */
static int dim_active = 0;
static Uint64 dim_start_time = 0;
static const Uint64 DIM_DURATION_MS = 150; /* Dim duration in milliseconds */

/* OpenGL resources */
static GLuint overlay_texture = 0;
static GLuint overlay_program = 0;
static GLuint overlay_vbo = 0;
static GLuint dim_program = 0;  /* Simple shader for dim effect */
static GLuint dim_vbo = 0;

/* Shader source for RGBA texture rendering */
static const GLchar overlay_vertex[] =
    "#version 100\n"
    "precision mediump float;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexPos;\n"
    "varying vec2 TexCoord0;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = Position;\n"
    "    TexCoord0 = TexPos;\n"
    "}\0";

static const GLchar overlay_fragment[] =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform sampler2D Texture;\n"
    "varying vec2 TexCoord0;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(Texture, TexCoord0);\n"
    "}\0";

/* Simple shader for dim effect (full screen dark overlay) */
static const GLchar dim_vertex[] =
    "#version 100\n"
    "precision mediump float;\n"
    "attribute vec4 Position;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = Position;\n"
    "}\0";

static const GLchar dim_fragment[] =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform vec4 DimColor;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = DimColor;\n"
    "}\0";

/* Button regions (normalized coordinates relative to remote image) */
typedef struct {
    const char *name;
    float x_min, y_min, x_max, y_max;  /* 0.0 to 1.0 */
    SDL_Keycode key;
} ButtonRegion;

static const ButtonRegion buttons[] = {
    /* Row 1: + Increase animation speed */
    {"Speed +", 0.02f, 0.08f, 0.14f, 0.30f, SDLK_UP},

    /* Row 1: - Decrease animation speed */
    {"Speed -", 0.86f, 0.08f, 0.98f, 0.30f, SDLK_DOWN},

    /* Row 2: Next Palette button */
    {"Next Palette", 0.16f, 0.42f, 0.86f, 0.64f, SDLK_RIGHT},

    /* Row 3: Next Pattern button - full width with no gaps */
    {"Next Pattern", 0.26f, 0.72f, 0.76f, 0.94f, SDLK_N},
};

static const int num_buttons = sizeof(buttons) / sizeof(ButtonRegion);

static void remote_glerror(const char *s)
{
    GLenum err;
    fprintf(stderr, "OpenGL error at %s. Error log follows:\n", s);
    while((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "%x\n", err);
    }
}

static GLuint load_overlay_shader(GLuint program, GLenum type, const GLchar *shaderSrc)
{
    GLint compile_status;
    GLuint shader;
    shader = glCreateShader(type);
    if (shader == 0) {
        remote_glerror("glCreateShader for overlay");
        return 0;
    }
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        GLsizei loglen = 0;
        GLchar infolog[1024];
        printf("OpenGL error: %s shader for overlay failed to compile. Info log follows:\n",
               (type == GL_VERTEX_SHADER) ? "vertex" : "fragment");
        glGetShaderInfoLog(shader, sizeof(infolog), &loglen, infolog);
        fwrite(infolog, loglen, 1, stderr);
        return 0;
    }
    glAttachShader(program, shader);
    return shader;
}

/* Symbols for embedded remote.png on Linux (created by objcopy) */
#if defined(__linux__) && !defined(__ANDROID__)
extern unsigned char _binary_remote_png_start[];
extern unsigned char _binary_remote_png_end[];
#endif

void remote_overlay_init(void)
{
    SDL_Surface *surface = NULL;
    GLint status;
    char path[1024];

    /* Load the remote PNG - platform-specific path handling */
#ifdef __APPLE__
    const char *basePath = SDL_GetBasePath();
    if (basePath) {
#if TARGET_OS_IOS
        /* iOS: Resources are in the app bundle root */
        snprintf(path, sizeof(path), "%sremote.png", basePath);
        surface = IMG_Load(path);
#else
        /* macOS: Resources are in Contents/Resources/, executable is in Contents/MacOS/ */
        snprintf(path, sizeof(path), "%s../Resources/remote.png", basePath);
        surface = IMG_Load(path);
#endif
    } else {
        surface = IMG_Load("remote.png");
    }
#elif defined(__ANDROID__)
    /* Android: Load from assets folder using SDL_IOFromFile */
    /* SDL3 can load from Android assets using a path like "remote.png" */
    SDL_IOStream *io = SDL_IOFromFile("remote.png", "rb");
    if (io != NULL) {
        surface = IMG_Load_IO(io, 1); /* 1 = SDL will close the IO stream automatically */
        if (surface == NULL) {
            fprintf(stderr, "Failed to load remote.png from Android assets: %s\n", SDL_GetError());
            return;
        }
    } else {
        fprintf(stderr, "Failed to open remote.png from Android assets: %s\n", SDL_GetError());
        return;
    }
#elif _WIN32
    /* Windows: Load from embedded resource */
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(101), RT_RCDATA);
    if (hRes == NULL) {
        surface = NULL;
    } else {
        HGLOBAL hData = LoadResource(NULL, hRes);
        if (hData == NULL) {
            surface = NULL;
        } else {
            DWORD size = SizeofResource(NULL, hRes);
            const void *data = LockResource(hData);
            if (data == NULL) {
                surface = NULL;
            } else {
                /* Create SDL IO stream from memory */
                SDL_IOStream *io = SDL_IOFromConstMem(data, (int)size);
                if (io != NULL) {
                    surface = IMG_Load_IO(io, 1); /* 1 = SDL will close the IO stream automatically */
                } else {
                    surface = NULL;
                }
            }
        }
    }
#elif defined(__linux__)
    /* Linux: Load from embedded resource */
    size_t png_size = (size_t)(_binary_remote_png_end - _binary_remote_png_start);
    SDL_IOStream *io = SDL_IOFromConstMem(_binary_remote_png_start, (int)png_size);
    if (io != NULL) {
        surface = IMG_Load_IO(io, 1); /* 1 = SDL will close the IO stream automatically */
        if (surface == NULL) {
            fprintf(stderr, "Failed to load remote.png from embedded resource: %s\n", SDL_GetError());
            return;
        }
    } else {
        fprintf(stderr, "Failed to create IO stream from embedded remote.png: %s\n", SDL_GetError());
        return;
    }
#else
    surface = IMG_Load("remote.png");
#endif

    if (surface == NULL) {
        return;
    }

    remote_width = surface->w;
    remote_height = surface->h;

    /* Create OpenGL texture */
    glGenTextures(1, &overlay_texture);
    glBindTexture(GL_TEXTURE_2D, overlay_texture);

    /* Set texture parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Convert surface to RGBA if necessary */
    SDL_Surface *rgba_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surface);

    if (rgba_surface == NULL) {
        fprintf(stderr, "Failed to convert surface to RGBA: %s\n", SDL_GetError());
        return;
    }

    /* Upload texture data */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surface->w, rgba_surface->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels);

    /* Destroy converted surface */
    SDL_DestroySurface(rgba_surface);

    /* Create shader program for overlay */
    overlay_program = glCreateProgram();
    if (overlay_program == 0) {
        remote_glerror("glCreateProgram for overlay");
        return;
    }

    load_overlay_shader(overlay_program, GL_VERTEX_SHADER, overlay_vertex);
    load_overlay_shader(overlay_program, GL_FRAGMENT_SHADER, overlay_fragment);

    glBindAttribLocation(overlay_program, 0, "Position");
    glBindAttribLocation(overlay_program, 1, "TexPos");

    glLinkProgram(overlay_program);
    glGetProgramiv(overlay_program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        remote_glerror("glLinkProgram for overlay");
        return;
    }

    /* Create VBO for overlay quad */
    glGenBuffers(1, &overlay_vbo);

    /* Create shader program for dim effect */
    dim_program = glCreateProgram();
    if (dim_program == 0) {
        remote_glerror("glCreateProgram for dim");
        return;
    }

    load_overlay_shader(dim_program, GL_VERTEX_SHADER, dim_vertex);
    load_overlay_shader(dim_program, GL_FRAGMENT_SHADER, dim_fragment);

    glBindAttribLocation(dim_program, 0, "Position");

    glLinkProgram(dim_program);
    glGetProgramiv(dim_program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        remote_glerror("glLinkProgram for dim");
        return;
    }

    /* Create VBO for dim quad */
    glGenBuffers(1, &dim_vbo);

}

void remote_overlay_cleanup(void)
{
    if (overlay_texture != 0) {
        glDeleteTextures(1, &overlay_texture);
        overlay_texture = 0;
    }
    if (overlay_program != 0) {
        glDeleteProgram(overlay_program);
        overlay_program = 0;
    }
    if (overlay_vbo != 0) {
        glDeleteBuffers(1, &overlay_vbo);
        overlay_vbo = 0;
    }
    if (dim_program != 0) {
        glDeleteProgram(dim_program);
        dim_program = 0;
    }
    if (dim_vbo != 0) {
        glDeleteBuffers(1, &dim_vbo);
        dim_vbo = 0;
    }
}

/* Helper function to calculate scale and scaled dimensions */
static void calculate_overlay_size(int window_width, int window_height,
                                   float *out_scale, float *out_scaled_width, float *out_scaled_height)
{
    /* Scale remote to fit nicely in window, ensuring it fits within both width and height */
    /* Use 90% of window size as maximum to leave some padding */
    float max_width = window_width * 0.9f;
    float max_height = window_height * 0.9f;
    
    /* Calculate scale based on height (preferred: 1/3 of window height) */
    float preferred_height = window_height / 3.0f;
    float scale_height = preferred_height / remote_height;
    
    /* Calculate scale based on width constraint */
    float scale_width = max_width / remote_width;
    
    /* Use the smaller scale to ensure it fits in both dimensions */
    float scale = scale_height;
    if (scale_width < scale_height) {
        scale = scale_width;
    }
    
    /* Ensure the scaled size doesn't exceed maximum dimensions */
    float scaled_width = remote_width * scale;
    float scaled_height = remote_height * scale;
    
    if (scaled_width > max_width) {
        scale = max_width / remote_width;
        scaled_width = max_width;
        scaled_height = remote_height * scale;
    }
    if (scaled_height > max_height) {
        scale = max_height / remote_height;
        scaled_width = remote_width * scale;
        scaled_height = max_height;
    }
    
    *out_scale = scale;
    *out_scaled_width = scaled_width;
    *out_scaled_height = scaled_height;
}


void remote_overlay_render_if_visible(int window_width, int window_height)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (!overlay_visible || overlay_texture == 0 || overlay_program == 0) {
        return;
    }

    /* Calculate overlay position and size */
    float scale, scaled_width, scaled_height;
    calculate_overlay_size(window_width, window_height, &scale, &scaled_width, &scaled_height);

    /* Center the remote in the window */
    remote_x = window_width / 2.0f;
    remote_y = window_height / 2.0f;

    /* Convert to NDC (Normalized Device Coordinates) */
    float left = (remote_x - scaled_width / 2.0f) / window_width * 2.0f - 1.0f;
    float right = (remote_x + scaled_width / 2.0f) / window_width * 2.0f - 1.0f;
    float bottom = -((remote_y + scaled_height / 2.0f) / window_height * 2.0f - 1.0f);
    float top = -((remote_y - scaled_height / 2.0f) / window_height * 2.0f - 1.0f);

    /* Vertices: position (x, y, z, w) + texcoord (u, v) */
    GLfloat vertices[] = {
        left,  bottom, 0.0f, 1.0f,  0.0f, 1.0f,  /* Bottom-left */
        left,  top,    0.0f, 1.0f,  0.0f, 0.0f,  /* Top-left */
        right, bottom, 0.0f, 1.0f,  1.0f, 1.0f,  /* Bottom-right */
        right, top,    0.0f, 1.0f,  1.0f, 0.0f,  /* Top-right */
    };

    /* Save current OpenGL state */
    GLint current_program;
    GLint current_texture;
    GLint current_active_texture;
    GLint current_array_buffer;
    GLint current_viewport[4];
    GLint blend_enabled;
    GLint attr0_enabled, attr1_enabled;
    GLint attr0_size, attr1_size;
    GLint attr0_type, attr1_type;
    GLint attr0_stride, attr1_stride;
    GLint attr0_normalized, attr1_normalized;
    void *attr0_ptr, *attr1_ptr;

    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &current_active_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_array_buffer);
    glGetIntegerv(GL_VIEWPORT, current_viewport);
    blend_enabled = glIsEnabled(GL_BLEND);
    
    /* Save vertex attribute state */
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr0_enabled);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr1_enabled);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attr0_size);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attr1_size);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attr0_type);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attr1_type);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attr0_stride);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attr1_stride);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attr0_normalized);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attr1_normalized);
    glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attr0_ptr);
    glGetVertexAttribPointerv(1, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attr1_ptr);

    /* Enable blending for transparency */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Use overlay shader program */
    glUseProgram(overlay_program);

    /* Bind texture */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, overlay_texture);
    glUniform1i(glGetUniformLocation(overlay_program, "Texture"), 0);

    /* Upload vertices */
    glBindBuffer(GL_ARRAY_BUFFER, overlay_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    /* Set vertex attributes */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                         (void*)(4 * sizeof(GLfloat)));

    /* Draw the overlay */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    /* Restore OpenGL state */
    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }

    /* Restore buffer binding first */
    glBindBuffer(GL_ARRAY_BUFFER, current_array_buffer);
    
    /* Restore vertex attribute arrays to their previous state */
    if (attr0_enabled) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, attr0_size, attr0_type, attr0_normalized, attr0_stride, attr0_ptr);
    } else {
        glDisableVertexAttribArray(0);
    }
    if (attr1_enabled) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, attr1_size, attr1_type, attr1_normalized, attr1_stride, attr1_ptr);
    } else {
        glDisableVertexAttribArray(1);
    }

    glActiveTexture(current_active_texture);
    glBindTexture(GL_TEXTURE_2D, current_texture);
    glUseProgram(current_program);
    
    /* Restore viewport */
    glViewport(current_viewport[0], current_viewport[1], current_viewport[2], current_viewport[3]);
}

void remote_overlay_render_dim(int window_width, int window_height)
{
    /* Render dim overlay if active */
    if (dim_active && dim_program != 0) {
        Uint64 current_time = SDL_GetTicks();
        Uint64 dim_duration = current_time - dim_start_time;
        
        if (dim_duration < DIM_DURATION_MS) {
            /* Calculate dim opacity (fade out over duration) */
            float dim_alpha = 1.0f - (float)dim_duration / (float)DIM_DURATION_MS;
            
            /* Calculate overlay position and size (same as overlay rendering) */
            float scale, scaled_width, scaled_height;
            calculate_overlay_size(window_width, window_height, &scale, &scaled_width, &scaled_height);
            
            /* Center the remote in the window (same as overlay) */
            float remote_x = window_width / 2.0f;
            float remote_y = window_height / 2.0f;
            
            /* Convert to NDC (Normalized Device Coordinates) */
            float left = (remote_x - scaled_width / 2.0f) / window_width * 2.0f - 1.0f;
            float right = (remote_x + scaled_width / 2.0f) / window_width * 2.0f - 1.0f;
            float bottom = -((remote_y + scaled_height / 2.0f) / window_height * 2.0f - 1.0f);
            float top = -((remote_y - scaled_height / 2.0f) / window_height * 2.0f - 1.0f);
            
            /* Save current OpenGL state */
            GLint current_program;
            GLint current_array_buffer;
            GLint current_viewport_dim[4];
            GLint blend_enabled;
            GLint attr0_enabled, attr1_enabled;
            GLint attr0_size, attr1_size;
            GLint attr0_type, attr1_type;
            GLint attr0_stride, attr1_stride;
            GLint attr0_normalized, attr1_normalized;
            void *attr0_ptr, *attr1_ptr;
            
            glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_array_buffer);
            glGetIntegerv(GL_VIEWPORT, current_viewport_dim);
            blend_enabled = glIsEnabled(GL_BLEND);
            
            /* Save vertex attribute state */
            glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr0_enabled);
            glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr1_enabled);
            glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attr0_size);
            glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attr1_size);
            glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attr0_type);
            glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attr1_type);
            glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attr0_stride);
            glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attr1_stride);
            glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attr0_normalized);
            glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attr1_normalized);
            glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attr0_ptr);
            glGetVertexAttribPointerv(1, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attr1_ptr);
            
            /* Ensure viewport is correct */
            glViewport(0, 0, window_width, window_height);
            
            /* Enable blending */
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            /* Use dim shader program */
            glUseProgram(dim_program);
            
            /* Set dim color (black with alpha for dimming effect) */
            GLfloat dim_color[4] = {0.0f, 0.0f, 0.0f, dim_alpha * 0.4f}; /* 40% opacity max */
            glUniform4fv(glGetUniformLocation(dim_program, "DimColor"), 1, dim_color);
            
            /* Create dim quad vertices matching overlay bounds (NDC coordinates) */
            GLfloat dim_vertices[] = {
                left,  bottom, 0.0f, 1.0f,  /* Bottom-left */
                left,  top,    0.0f, 1.0f,  /* Top-left */
                right, bottom, 0.0f, 1.0f,  /* Bottom-right */
                right, top,    0.0f, 1.0f,  /* Top-right */
            };
            
            /* Upload vertices */
            glBindBuffer(GL_ARRAY_BUFFER, dim_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(dim_vertices), dim_vertices, GL_DYNAMIC_DRAW);
            
            /* Set vertex attributes */
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
            
            /* Draw the dim overlay */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            /* Restore buffer binding first */
            glBindBuffer(GL_ARRAY_BUFFER, current_array_buffer);
            
            /* Restore vertex attribute arrays to their previous state */
            if (attr0_enabled) {
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, attr0_size, attr0_type, attr0_normalized, attr0_stride, attr0_ptr);
            } else {
                glDisableVertexAttribArray(0);
            }
            if (attr1_enabled) {
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, attr1_size, attr1_type, attr1_normalized, attr1_stride, attr1_ptr);
            } else {
                glDisableVertexAttribArray(1);
            }
            
            /* Restore OpenGL state */
            if (!blend_enabled) {
                glDisable(GL_BLEND);
            }
            glUseProgram(current_program);
            glViewport(current_viewport_dim[0], current_viewport_dim[1], 
                      current_viewport_dim[2], current_viewport_dim[3]);
        } else {
            /* Dim expired */
            dim_active = 0;
        }
    }
}

int remote_overlay_is_point_inside(int x, int y, int window_width, int window_height)
{
    float scale, scaled_width, scaled_height;
    float remote_left, remote_top;
    float remote_x, remote_y;

    if (!overlay_visible) {
        return 0;
    }

    /* Calculate overlay bounds using the same scaling logic */
    calculate_overlay_size(window_width, window_height, &scale, &scaled_width, &scaled_height);
    remote_x = window_width / 2.0f;
    remote_y = window_height / 2.0f;
    remote_left = remote_x - scaled_width / 2.0f;
    remote_top = remote_y - scaled_height / 2.0f;

    /* Check if point is inside the remote bounds */
    if (x >= remote_left && x <= remote_left + scaled_width &&
        y >= remote_top && y <= remote_top + scaled_height) {
        return 1;  /* Inside bounds */
    }

    return 0;  /* Outside bounds */
}

int remote_overlay_handle_click(int x, int y, int window_width, int window_height)
{
    int i;
    float click_x, click_y;
    float scale, scaled_width, scaled_height;
    float remote_left, remote_top;
    float rel_x, rel_y;

    if (!overlay_visible) {
        return -1;
    }

    /* Calculate overlay bounds using the same scaling logic */
    calculate_overlay_size(window_width, window_height, &scale, &scaled_width, &scaled_height);
    remote_x = window_width / 2.0f;
    remote_y = window_height / 2.0f;
    remote_left = remote_x - scaled_width / 2.0f;
    remote_top = remote_y - scaled_height / 2.0f;

    /* Check if click is outside the remote bounds */
    if (x < remote_left || x > remote_left + scaled_width ||
        y < remote_top || y > remote_top + scaled_height) {
        return -1;  /* Outside bounds */
    }

    /* Convert click to normalized coordinates within remote image */
    rel_x = (x - remote_left) / scaled_width;
    rel_y = (y - remote_top) / scaled_height;

    /* Check which button was clicked */
    for (i = 0; i < num_buttons; i++) {
        /* Use inclusive bounds - ensure we catch clicks at the edges */
        if (rel_x >= buttons[i].x_min && rel_x <= buttons[i].x_max &&
            rel_y >= buttons[i].y_min && rel_y <= buttons[i].y_max) {

            /* Simulate key press */
            SDL_Event key_event;
            key_event.type = SDL_EVENT_KEY_DOWN;
            key_event.key.key = buttons[i].key;
            SDL_PushEvent(&key_event);

            printf("Remote button '%s' clicked -> key %d\n", buttons[i].name, buttons[i].key);
            
            /* Trigger dim feedback */
            dim_active = 1;
            dim_start_time = SDL_GetTicks();
            
            return 1;  /* Button clicked */
        }
    }

    /* Click was on remote but not on any button */
    return 0;
}

void remote_overlay_show(void)
{
    overlay_visible = 1;
}

void remote_overlay_hide(void)
{
    overlay_visible = 0;
}

int remote_overlay_is_visible(void)
{
    return overlay_visible;
}
