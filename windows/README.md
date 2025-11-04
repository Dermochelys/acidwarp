# Acid Warp for Windows

<a href="https://apps.microsoft.com/detail/9N7W8XK7GGHC?referrer=appbadge&mode=direct"><img src="https://get.microsoft.com/images/en-us%20dark.svg" height="41" alt="Get it from Microsoft"></a>

Supports devices running Windows 10 or later.

Does not require any runtime dependencies as required libs are statically linked in the executable file.

## Technical Details

- Based on a [fork](https://github.com/Dermochelys/acidwarp) of [dreamlayers/acidwarp](https://github.com/dreamlayers/acidwarp), which is embedded as a [submodule](acidwarp/acidwarp).
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.
- Statically links and uses [GLEW](https://github.com/nigels-com/glew).

## Building

- [MSYS2](https://www.msys2.org/) is used for the build environment, along with [pacman](https://wiki.archlinux.org/title/Pacman) for dependency installation, and [CMake](https://cmake.org/) for the actual build orchestration.
- See the [SDL3 Wiki](https://wiki.libsdl.org/SDL3/README-windows) for more information about the build environment setup.
- SDL3 and SDL3_image are installed via MSYS2 pacman packages: `mingw-w64-x86_64-sdl3` and `mingw-w64-x86_64-sdl3-image`

### Runtime Dependencies

While most libraries are statically linked, SDL3_image dynamically loads PNG support at runtime. The executable requires `libpng16-16.dll` to be present in the same directory. This DLL is available at `/mingw64/bin/libpng16-16.dll` in the MSYS2 environment and should be distributed with the application.

## Previous ports
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](gpl-3.0.md)

![Acid Warp logo](store-assets/icon-512.png)
