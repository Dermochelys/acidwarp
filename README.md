# Acid Warp for Android / iOS / iPadOS / macOS / Windows

## What is Acid Warp?

Acid Warp is an eye candy program which displays various patterns and animates them by changing the palette. Originally it was an MS-DOS / Windows program by Noah Spurrier and Mark Bilk and was made in 1992/1993.

This is a fork by [Matthew Zavislak](https://github.com/elevenfive) based on the previous [SDL2-based port](https://github.com/dreamlayers/acidwarp) by [Boris Gjenero](https://github.com/dreamlayers) to the SDL 1.2 / 2 library. That, in turn, is based on the Linux SVGALib port by Steven Wills.

This fork can be built for:
 - Android phones, tablets, and TVs
 - iOS phones and iPadOS tablets
 - macOS devices
 - Linux devices (Confirmed working on Ubuntu 22.04+)
 - Windows devices (requires DirectX 11+)

## Platform-Specific Build Instructions

For detailed build instructions for each platform, see:

| Platform | Status / Download Link                                                                                                                                                                                                                                                                                                                                                                                           |
| :--- |:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **[Android](android/README.md)** | <a href="https://play.google.com/store/apps/details?id=com.dermochelys.acidwarp"><img src="https://play.google.com/intl/en_us/badges/images/generic/en_badge_web_generic.png" height="55" alt="Get it on Google Play"></a>                                                                                                                                                                                       |
| **[iOS / iPadOS](ios/README.md)** | Not in iOS App Store yet, stay tuned!                                                                                                                                                                                                                                                                                                                                                                            |
| **[Linux](linux/README.md)** | <a href="https://snapcraft.io/acidwarp"><img src="https://snapcraft.io/en/dark/install.svg" height="41" alt="Get it from the Snap Store"></a>                                                                                                                                                                                                                                                                    |
| **[macOS](macos/README.md)** | <a href="https://apps.apple.com/us/app/acid-warp/id6753610977?mt=12&itscg=30200&itsct=apps_box_badge&mttnsubad=6753610977" style="display: inline-block;"><img src="https://toolbox.marketingtools.apple.com/api/v2/badges/download-on-the-app-store/black/en-us?releaseDate=1760486400" alt="Download on the App Store" style="width: 123px; height: 41px; vertical-align: middle; object-fit: contain;" /></a> |
| **[Windows](windows/README.md)** | <a href="https://apps.microsoft.com/detail/9N7W8XK7GGHC?referrer=appbadge&mode=direct"><img src="https://get.microsoft.com/images/en-us%20dark.svg" height="41" alt="Get it from Microsoft"></a>                                                                                                                                                                                                                 |

## Using the program (Android TV or devices with keyboards)

Use the following keys or remote control buttons to control the program:

| Key                 | Action                         |
|---------------------|--------------------------------|
| **Up**              | Rotate palette faster          |
| **Down**            | Rotate palette slower          |
| **Left**            | Lock/unlock on current pattern |
| **Right**           | Switch to the next palette     |
| **Select** or **N** | Switch to the next pattern     |
| **P**               | Pause / Resume animation       |
| **Back**            | Exit the app                   |
| **Home**            | Exit the app                   |

## Updating SDL3

SDL3 version is centrally managed in the `SDL_VERSION` file at the repository root. SDL3 artifacts are automatically downloaded during builds and are not checked into version control.

To update SDL3 across all platforms:
- Update the version in `SDL_VERSION` (e.g., `3.2.26`)

Build systems will automatically download the new version:
- **Android**: Gradle downloads SDL3-{version}.aar automatically before build, removing old versions
- **iOS/macOS**: Xcode Run Script build phase checks the installed SDL3.xcframework version and automatically downloads/updates if needed
- **Linux/Windows**: Installed via package managers

Note: Steps 2 and 3 have comments in their respective files pointing to each other as a reminder to keep them in sync.

### Checking Installed SDL3 Version (Local Development)

To check which SDL3 version is currently installed on your system:

- **Linux**: `pkg-config --modversion sdl3`
- **Windows (MSYS2)**: `pacman -Q mingw-w64-x86_64-sdl3`

## Version History (see commit history for full details)

### 5.0.0
Standardize version numbering across all platforms, continuing from the original MS-DOS version 4.2 (1993). Update to SDL 3.2.26.

### 1.3.0
Add iOS / iPadOS / macOS / Windows / Linux support; support logo display on smaller screen sizes; other minor enhancements.

### 1.2.0-android
Add a new pattern created by the Anthropic Claude agent and also some minor code cleanups.

### 1.1.0-android
Adds screen orientation change handling and minor cleanups targeting initial Android
phone & tablet support.

### 1.0-android
Uses SDL 3 and OpenGL ES 2.0, and features a streamlined and enhanced
version targeted at Android TV.

#### Additions / Enhancements
- Increase logo size (when screen size allows)
- Smoother "next" handling - simply treat as a timer expiration, which enables
  the same fadeout/fadein transition as would happen without "next" being issued
- Add Android TV remote support (See above)
- Fix most warnings related to int/long/double handling

#### Removals
- Support for anything but Android, iOS, iPadOS
- Compatibility with SDL before 3
- OpenGL support other than ES
- Lookup table usage for pattern layout
- Single-threaded pattern layout generation
- CPU-based palette rotation
- Icon generation (but add bitmap for generating 16x9 logo for use in icons)

## Further resources

- For more information about the original DOS version, see: [noah.org](https://www.noah.org/acidwarp/).
- For more information about the Linux port, see: [ibiblio.org](https://www.ibiblio.org/pub/Linux/apps/graphics/hacks/svgalib/acidwarp-1.0.tar.gz).
- For more information about the SDL port, see: [dreamlayers/acidwarp](https://github.com/dreamlayers/acidwarp)
- For more information about the Amiga port, see: [aminet.net](https://aminet.net/package/demo/misc/acidwarp)

- For more information about SDL, see: [libsdl-org/SDL](https://github.com/libsdl-org/SDL/).
- For more information about Acid Warp's history, see: [eyecandyarchive.com](http://eyecandyarchive.com/Acidwarp/).

*Note*: there are alternate spellings: `Acidwarp`, `AcidWarp`.  We use `Acid Warp` here, based on the original DOS `README`.

## License

- As this is a descendent of Steven Will's "AcidWarp for Linux" which was GPL licensed, this too
  is and must be also GPL licensed.  See [gpl-3.0.md](gpl-3.0.md)

- Note that the original project predated modern open-source licensing and was described as "All Rights reserved. Private Proprietary Source Code", and:
  - v. 3.22: `This program is free and public domain. It may not be used for profit.` (1992)
  - v. 4.2 : `This is free software`. (1993)

## Thanks

- Many heartfelt thanks to all previous authors especially (of course) Noah.
- Kudos to the Anthropic team as Claude was quite useful during the porting process.
- Additional thanks to [Scott Ostler](https://github.com/scottostler), for his mentoring with the iOS / iPadOS / macOS ports.
