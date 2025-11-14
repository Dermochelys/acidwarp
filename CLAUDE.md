# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Acid Warp is a cross-platform eye candy application that displays animated mathematical patterns with palette rotation. This is a multiplatform port supporting Android, iOS/iPadOS, macOS, Linux, and Windows. The project uses SDL3 for cross-platform windowing/input and OpenGL/OpenGL ES for rendering.

**Repository Structure:**
- `shared/` - Core C implementation shared across all platforms
- `android/` - Android-specific Gradle project
- `ios/` - iOS/iPadOS Xcode project
- `macos/` - macOS Xcode project
- `linux/` - Linux CMake build + Snap packaging
- `windows/` - Windows CMake build (MSYS2/MinGW)

**Main Branch:** `main`

## Build Commands

### Android
```bash
cd android
./gradlew build
```
Requirements: See `android/app/build.gradle` for JDK version, compileSdkVersion, ndkVersion, and CMake version. See `android/gradle/libs.versions.toml` for test dependency versions.

### Linux
```bash
cd linux
cmake --fresh && cmake --build .
```
Requirements: See `linux/CMakeLists.txt` for CMake minimum version. Requires SDL3, SDL3_image, and OpenGL 4.1+. Check SDL3 version: `pkg-config --modversion sdl3`

### Windows
```bash
cd windows
cmake --fresh && cmake --build .
```
Requirements: See `windows/CMakeLists.txt` for CMake minimum version. Requires MSYS2 environment, SDL3, SDL3_image, GLEW, and OpenGL (all statically linked). Check SDL3: `pacman -Q mingw-w64-x86_64-sdl3`

### iOS/iPadOS/macOS
Use Xcode 26+. SDL3 must be installed as an xcframework. See platform-specific README files for SDL framework installation instructions.

## Architecture

**Rendering Pipeline:**
The application uses a two-stage GPU-based palette rotation system:
1. Pattern generation creates an 8-bit indexed color buffer (CPU, multi-threaded)
2. OpenGL shaders perform real-time palette mapping:
   - Index texture (GL_R8) holds pattern data
   - Palette texture (GL_RGBA) holds 256-color palette
   - Fragment shader does palette lookup for each pixel

This allows smooth palette animation without regenerating patterns.

**Key Components:**
- `shared/acidwarp.c` - Main loop, event handling, timing
- `shared/gen_img.c` - Pattern generation dispatcher
- `shared/img_float.c` - 40+ mathematical pattern functions
- `shared/display.c` - OpenGL setup, shader compilation, texture management
- `shared/draw.c` - Pattern layout coordination
- `shared/palinit.c` - Palette generation (8 types)
- `shared/rolnfade.c` - Palette rotation and fade effects

**Threading Model:**
Pattern layout is generated using multiple threads. The abort_draw flag allows interrupting generation when user skips to next pattern.

**SDL3 Compatibility:**
The code includes compatibility wrappers for SDL3 API changes from SDL2 (mutex functions, cursor API). See `shared/acidwarp.c:19-28` and `shared/display.c:14-21`.

## SDL3 Version Management

SDL3 version is centrally managed via the `SDL_VERSION` file in the repo root. SDL3 artifacts are **not** checked into the repository - they are downloaded automatically during builds.

**To update SDL3 across all platforms:**
1. Update the version number in `SDL_VERSION` (e.g., `3.2.26`)
2. Update `source-tag` in `linux/snap/snapcraft.yaml` to match (e.g., `release-3.2.26`)
3. Update `SDL3_VERSION` in `.github/workflows/build-all-platforms.yml` to match

**Automatic downloads:**
- **Android**: Gradle automatically downloads SDL3-{version}.aar from GitHub releases during build (see `android/app/build.gradle:downloadSDL3` task). Only one SDL3-*.aar is kept in `android/app/libs/`.
- **iOS/macOS**: Xcode Run Script build phase automatically runs `download-sdl3-framework.sh` before compilation. The script:
  - Checks if SDL3.xcframework exists
  - If it exists, verifies the version in `ios-arm64/SDL3.framework/Headers/SDL.h` matches `SDL_VERSION`
  - Automatically removes and re-downloads if versions don't match
  - See the "Download SDL3" build phase in both Xcode projects
- **Linux/Windows**: SDL3 is installed via package managers (snap build, MSYS2 pacman)

SDL3 artifacts are gitignored and stored locally only.

## Platform-Specific Notes

**OpenGL Variants:**
- Android/Linux: OpenGL ES 2.0 (`GLES2/gl2.h`)
- iOS: OpenGL ES 2.0 (`OpenGLES/ES2/gl.h`)
- macOS: OpenGL 3.x+ (`OpenGL/gl3.h`)
- Windows: OpenGL 4.1+ with GLEW (`GL/glew.h`)

**Static Linking:**
- Linux Snap: SDL3 is statically linked (Ubuntu 24.04 LTS doesn't package SDL3 yet)
- Windows: All dependencies statically linked (`-static` linker flag)

**Version Numbering:**
As of 5.0.0, version numbers are standardized across platforms, continuing from original MS-DOS version 4.2. Android uses monotonic `versionCode` (no hotfix scheme needed).

## UI Testing

All platforms have automated UI tests that run in CI. The tests launch the application, capture screenshots, simulate user input (keyboard/touch), and verify the app responds correctly. Each test suite follows the same pattern:

1. Launch app and wait for initialization (5 seconds)
2. Capture startup screenshot
3. Check for error dialogs (SDL errors)
4. Test pattern cycling (n key, 5 iterations with 4s fade delay)
5. Test mouse/touch input (center screen click)
6. Test palette change (p key)
7. Capture final screenshot
8. Graceful shutdown (q key)
9. Verify expected number of screenshots captured

### Android
**Test Framework**: Android Instrumentation tests with JUnit4 and UiAutomator
**Test Location**: `android/app/src/androidTest/java/com/dermochelys/acidwarp/ActivityUITest.kt`
**Environment**: Android Emulator (API 36, x86_64, Nexus 6 profile)

**Key Features**:
- **AVD Caching**: Emulator snapshot cached to speed up CI runs
- **GPU Rendering**: Uses `swiftshader_indirect` for software GPU emulation in CI
- **Input Simulation**: `UiDevice.pressKeyCode()` for keyboard, `device.click()` for touch
- **Screenshot Capture**: `device.takeScreenshot()` saves to external files directory
- **Screenshot Retrieval**: `adb pull` copies screenshots from emulator to CI runner
- **Logcat Collection**: Full logcat captured for build and test phases

**Running locally**:
```bash
cd android
./gradlew :app:connectedDebugAndroidTest
```

### Linux
**Test Framework**: Bash script with Xvfb, xdotool, and scrot
**Test Location**: `linux/ui_test.sh`
**Environment**: Ubuntu 22.04+ with virtual X server

**Key Features**:
- **Virtual Display**: Xvfb creates virtual X server (display :99, 1920x1080x24)
- **Hardware Acceleration**: Uses actual OpenGL (no software rendering needed on CI)
- **Input Simulation**: xdotool for keyboard (`xdotool key n`) and mouse (`xdotool mousemove/click`)
- **Screenshot Capture**: scrot captures full screen to PNG files
- **Timeout Protection**: 30-second timeout prevents hung processes

**Running locally**:
```bash
cd linux
./ui_test.sh
```

### Windows
**Test Framework**: PowerShell script with Win32 APIs and .NET System.Windows.Forms
**Test Location**: `windows/ui_test.ps1`
**Environment**: Windows 10+ (Server 2022 in CI)

**Key Features**:
- **GPU Detection**: Automatically detects hardware GPU (NVIDIA, AMD, Intel)
- **Mesa3D Fallback**: Installs Mesa3D llvmpipe software renderer when no GPU present
- **Mesa Installation**: Via MSYS2 `mingw-w64-x86_64-mesa` package
- **Mesa Configuration**:
  - Copies `opengl32.dll` and `libgallium_wgl.dll` to app directory
  - Sets `GALLIUM_DRIVER=llvmpipe`, `MESA_GL_VERSION_OVERRIDE=4.1COMPAT`, `MESA_GLSL_VERSION_OVERRIDE=410`
  - Adds MSYS2 bin to PATH for LLVM dependencies
- **Window Management**: Win32 API `SetForegroundWindow()` and `ShowWindow()` ensure window focus
- **Input Simulation**: `[System.Windows.Forms.SendKeys]::SendWait()` for keyboard, `mouse_event()` for mouse
- **Screenshot Capture**: `[System.Drawing.Graphics]::CopyFromScreen()` captures display
- **Path Flexibility**: Detects CI (D:\a\_temp\msys64) vs local (C:\msys64) MSYS2 paths
- **Process Monitoring**: Extensive logging of window state, process lifecycle, and foreground status
- **Environment Cleanup**: Closes VS Code windows to prevent interference

**Running locally**:
```powershell
cd windows
.\ui_test.ps1
```

### macOS
**Test Framework**: XCTest UI testing framework via xcodebuild
**Test Location**: `macos/acidwarp-macosUITests/` (Swift UI tests)
**Environment**: macOS 14+ (Sonoma)

**Key Features**:
- **Native UI Testing**: XCTest's `XCUIApplication` and `XCUIElement` APIs
- **Hardware Acceleration**: Uses native macOS OpenGL (Metal translation layer)
- **Input Simulation**: XCTest's native keyboard/mouse event injection
- **Screenshot Capture**: `XCUIScreen.main.screenshot()` with attachments API
- **Screenshot Extraction**: `xcresulttool export attachments` extracts PNGs from .xcresult bundle
- **Screenshot Renaming**: Python script uses xcresult manifest for human-readable names
- **Code Signing**: Ad-hoc signing (`codesign --sign -`) for test runner
- **Build Separation**: `build-for-testing` then `test-without-building` pattern

**Running locally**:
```bash
cd macos
./run-uitests.sh
```

### iOS / iPadOS
**Test Framework**: XCTest UI testing framework via xcodebuild
**Test Location**: `ios/acidwarp-iosUITests/` (Swift UI tests)
**Environment**: iOS Simulator (iPhone 17 Pro, iOS 18.2+)

**Key Features**:
- **Simulator Warmup**: Background simulator start (`start-simulator-background.sh`) runs parallel to build
- **Split Build/Test**: Separate build (`build-uitests.sh`) and test (`test-uitests.sh`) phases for reliability
- **Simulator Wait Logic**: `wait-for-simulator.sh` polls for "Booted" state before testing
- **Screenshot Handling**: Same extraction process as macOS (xcresulttool + rename script)
- **Network Blocking**: Blocks `developerservices2.apple.com` to prevent Xcode activation delays
- **Simulator Cleanup**: Graceful simulator shutdown after tests complete
- **Device Selection**: Uses iPhone 17 Pro simulator by UUID lookup

**Running locally** (from ios directory):
```bash
# Start simulator in background
./start-simulator-background.sh

# Build tests (in another terminal)
./build-uitests.sh

# Wait for simulator readiness
./wait-for-simulator.sh

# Run tests
./test-uitests.sh
```

### Test Artifacts
All platforms upload test artifacts to GitHub Actions:
- **Screenshots**: PNG files numbered by test sequence (01-startup.png, 02-pattern-1.png, etc.)
- **Logs**: Platform-specific logs (logcat for Android, xcodebuild.log for iOS/macOS, acidwarp.log for Linux/Windows)
- **Test Results**: XML test results (Android) or .xcresult bundles (iOS/macOS)

## Development Guidelines

**Adding New Patterns:**
1. Add function to `shared/img_float.c`
2. Update `NUM_IMAGE_FUNCTIONS` in `shared/acidwarp.h`
3. Register in pattern dispatcher

**Platform-Specific Code:**
Minimize platform-specific code. Use `#ifdef __APPLE__`, `TARGET_OS_IOS`, `_WIN32`, `ANDROID` conditionals only when necessary. Most platform differences are in build configuration, not source code.

**License:**
GPL-3.0 (inherited from Steven Wills' Linux port). Original DOS version evolved from proprietary to "free software" in 1993.
