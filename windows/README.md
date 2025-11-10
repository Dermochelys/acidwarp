# Acid Warp for Windows

<a href="https://apps.microsoft.com/detail/9N7W8XK7GGHC?referrer=appbadge&mode=direct"><img src="https://get.microsoft.com/images/en-us%20dark.svg" height="41" alt="Get it from Microsoft"></a>

Supports devices running Windows 10 or later.

Does not require any runtime dependencies as required libs are statically linked in the executable file.

## Building

- [MSYS2](https://www.msys2.org/) is used for the build environment, along with [pacman](https://wiki.archlinux.org/title/Pacman) for dependency installation, and [CMake](https://cmake.org/) for the actual build orchestration.
- See the [SDL3 Wiki](https://wiki.libsdl.org/SDL3/README-windows) for more information about the build environment setup.
- SDL3 and SDL3_image are installed via MSYS2 pacman packages: `mingw-w64-x86_64-sdl3` and `mingw-w64-x86_64-sdl3-image`
- Statically links and uses [GLEW](https://github.com/nigels-com/glew)

### Runtime Dependencies

While most libraries are statically linked, SDL3_image dynamically loads PNG support at runtime. The executable requires the following DLLs to be present in the same directory:

- `libpng16-16.dll` - PNG image support
- `zlib1.dll` - Compression library (required by libpng)

These DLLs are available in the MSYS2 environment at `/mingw64/bin/` and should be distributed with the application.

### Code Signing

The `signexe.bat` script is used to sign the `acidwarp-windows.exe` file using Microsoft's Azure Code Signing service. The script uses `signtool.exe` with SHA256 hashing and timestamping to ensure the executable is properly signed for distribution.

## Release Process

Follow these steps to create a new release:

1. **Update Version**: Bump the version number in the resource file
2. **Build**: Run the following commands to perform a clean build:
   ```
   cmake --fresh .
   cmake --build .
   ```
3. **Sign Executable**: Run `signexe.bat` to sign the generated `acidwarp-windows.exe` file
4. **Create Release Folder**: Create a new folder in the release directory for the new version
5. **Generate MSIX Package**: Ask Claude to create a new MSIX package based on the last version
6. **Create Upgrade Test Script**: Ask Claude to create a PowerShell script to verify the upgrade flow. This will require signing with a temporary certificate
7. **Manual Testing**: Manually test that the app works correctly and the upgrade process functions as expected
8. **Cleanup Test Artifacts**: Uninstall the app and remove the temporary certificate used for testing
9. **Cleanup Project Files**: Remove any temporary files created during steps 5-8 from the project folder

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
