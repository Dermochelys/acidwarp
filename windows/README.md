# Acid Warp for Windows

- [App Store Link](https://apps.microsoft.com/detail/9N7W8XK7GGHC)
- Supports devices running Windows 10 or later.
- Does not require any runtime dependencies as required libs are statically linked in the executable file.

## Technical Details

- Based on a [fork](https://github.com/Dermochelys/acidwarp) of [dreamlayers/acidwarp](https://github.com/dreamlayers/acidwarp), which is embedded as a [submodule](acidwarp/acidwarp).
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.
- Statically links and uses [GLEW](https://github.com/nigels-com/glew).

## Building

- [MSYS2](https://www.msys2.org/) is used for the build environment, along with [pacman](https://wiki.archlinux.org/title/Pacman) for dependency installation, and [CMake](https://cmake.org/) for the actual build orchestration.
- See the [SDL3 Wiki](https://wiki.libsdl.org/SDL3/README-windows) for more information about the build environment setup.

## Previous ports
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](gpl-3.0.md)

![Acid Warp logo](store-assets/icon-512.png)
