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

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
