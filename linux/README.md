# Acid Warp for Linux

<a href="https://snapcraft.io/acidwarp"><img src="https://snapcraft.io/en/dark/install.svg" height="41" alt="Get it from the Snap Store"></a>

Supports devices running Ubuntu 24+ and OpenGL 4.1+.

SDL3 and SDL3_image are statically linked (for now) as latest Ubuntu LTS (24.04) [does not include SDL3](https://launchpad.net/ubuntu/+source/libsdl3).

## Building Locally

### Dependencies

See [CMakeLists.txt](CMakeLists.txt) for dependency information.

If your distribution doesn't provide SDL3 and SDL3_image packages, see the build instructions:
- [SDL3 build instructions for Unix](https://wiki.libsdl.org/SDL3/README-cmake#building-sdl-on-unix)
- [SDL3_image build instructions](https://github.com/libsdl-org/SDL_image)

Check your installed versions with:
- `pkg-config --modversion sdl3`
- `pkg-config --modversion SDL3_image`

### Build Steps

Build the app with CMake:
```bash
cmake --fresh && cmake --build .
```

If SDL3 is installed in a non-standard location, specify it with `CMAKE_PREFIX_PATH`:
```bash
cmake --fresh -DCMAKE_PREFIX_PATH=/path/to/sdl3/install && cmake --build .
```

## UI Testing

Automated UI tests verify the application works correctly by launching it in a virtual X server (Xvfb), simulating user input, and capturing screenshots.

**Running UI tests:**
```bash
./ui_test.sh
```

**Test Environment:**
- **Virtual Display**: Xvfb creates a virtual X server (display :99, 1920x1080x24) - no physical display needed
- **Dependencies**: `xvfb`, `scrot` (screenshots), `xdotool` (input simulation)
- **Input Simulation**: xdotool sends keyboard events (`xdotool key n`) and mouse clicks (`xdotool click 1`)
- **Screenshot Capture**: scrot captures the virtual display to PNG files
- **OpenGL**: Uses hardware-accelerated OpenGL (no software rendering needed on CI)

**Test Sequence:**
1. Start Xvfb on display :99
2. Launch acidwarp-linux with 30-second timeout
3. Capture startup screenshot
4. Check for SDL error dialogs
5. Test pattern cycling (5 iterations, 'n' key)
6. Test mouse input (center screen click)
7. Test palette change ('p' key)
8. Graceful shutdown ('q' key)

Screenshots are saved to `screenshots/` directory. CI uploads screenshots and logs (acidwarp.log, acidwarp-error.log) as artifacts.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
