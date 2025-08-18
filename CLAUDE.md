# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Acidwarp is a classic eye candy program that displays animated patterns with palette cycling effects. Originally an MS-DOS program by Noah Spurrier, this is a cross-platform SDL port by Boris Gjenero. The program generates psychedelic visual patterns using floating-point calculations and animates them through palette rotation.

## Build System

This project uses both traditional Makefiles and CMake, with CMake being the primary build system:

### CMake Build (Primary)
- **Build**: `cmake . && make` or `make` (if already configured)
- **Clean**: `make clean` 
- **CMake reconfigure**: `cmake .`

### Build Options
- **SDL version**: Set `SDL_VERSION` to 2 or 3 (default: 2)
- **OpenGL support**: Use `GL=ON` (default) or `GL=OFF`
- **Static linking**: Use `STATIC=ON` (Windows default) or `STATIC=OFF`

### Platform-Specific Builds
- **Emscripten/WebGL**: `emmake make` or `emmake make GL=1`
- **Windows from Cygwin**: Default CC is `i686-w64-mingw32-gcc`
- **SDL 2**: `make SDL=2` 
- **With OpenGL**: `make GL=1`

## Architecture

### Core Components

1. **Main Loop** (`acidwarp.c`): Event handling, timing, and program control
2. **Display System** (`display.c/h`): SDL interface, window management, and palette handling
3. **Drawing System** (`draw.c`): Pattern rendering coordination and worker thread management
4. **Image Generation**:
   - `img_float.c`: High-resolution floating-point pattern generation
   - `bit_map.c`: Bitmap manipulation utilities
5. **Palette System** (`palinit.c`, `rolnfade.c`): Color palette generation and animation
6. **Worker System** (`worker.c/h`): Background pattern computation (Emscripten)

### Key Architecture Patterns

- **Worker Threading**: Emscripten builds use web workers for non-blocking pattern generation
- **Modular Display**: Display layer abstracts SDL operations for cross-platform compatibility
- **Command System**: Input handling through `acidwarp_command` enum for clean separation

### Image Function System

The program includes 40 different pattern generation functions (defined by `NUM_IMAGE_FUNCTIONS`). Pattern selection uses shuffled lists to avoid repetition.

## Dependencies

- **SDL 1.2.x or 2.x**: Core graphics and input (detected via `sdl-config` or pkg-config)
- **OpenGL/OpenGL ES 2.0**: Hardware-accelerated palette cycling (optional)
- **ImageMagick convert**: Icon resizing during build
- **xxd**: Icon embedding into source code
- **icotool**: Windows icon generation
- **windres**: Windows resource compilation

## Development Notes

- Pattern functions are in separate files for modularity
- Global state is concentrated in `acidwarp.c` (acknowledged as needing refactoring)
- Emscripten builds have special handling for web constraints (no fullscreen, different timing)
- The codebase maintains compatibility with the original VGA 320x200 mode while supporting modern resolutions