# Acid Warp for Android

## What is Acid Warp?

Acid Warp is an eye candy program which displays various patterns and
animates them by changing the palette. Originally it was an MS-DOS / Windows
program by Noah Spurrier and Mark Bilk and was made in 1992/1993. 

This is a port by Matthew Zavislak based on the previous port by Boris Gjenero
to the SDL 1.2 / 2 library. That in turn is based on the Linux SVGALib port 
by Steven Wills.

This port can be built for Android devices including phones, tablets, and TVs.

## Using the program

Use the following remote control buttons to control the program:

| Key        | Action                            |
|------------|-----------------------------------|
| **Up**     | Rotate palette faster             |
| **Down**   | Rotate palette slower             |
| **Left**   | Lock/unlock on current pattern    |
| **Right**  | Switch to the next palette        |
| **Select** | Switch to the next pattern        |
| **Back**   | Exit the app                      |
| **Home**   | Exit the app                      |

## Version History

### 1.1.0-android
The library adds screen orientation change handling and minor cleanups targeting initial Android 
phone & tablet support.

### 1.0-android
The library uses SDL 3 and OpenGL ES 2.0, and features a streamlined and enhanced 
version targeted at Android TV.

#### Additions / Enhancements
- Increase logo size by 4x
- Smoother "next" handling - simply treat as a timer expiration, which enables
  the same fadeout/fadein transition as would happen without "next" being issued
- Add Android TV remote support (See above)
- Fix most warnings related to int/long/double handling

#### Removals
- Support for anything but Android
- Compatibility with SDL before 3
- OpenGL support other than ES
- Lookup table usage for pattern layout
- Single-threaded pattern layout generation
- CPU-based palette rotation
- Icon generation

## Further resources

For more information about the SDL port, see: [README_SDL.md](previous_ports/acidwarp-sdl/README_SDL.md)
For more information about the Linux port, see: [README_LINUX_V_1_0](README_LINUX_V_1_0).
For more information about the original DOS version, see: [README_DOS_V_4_1](README_DOS_V_4_1).

*Note*: there is an alternate spelling which is `Acidwarp`.  Based on the original DOS `README`,
it is believed that `Acid Warp` is the correct spelling.

## License

As this is a descendent of Steven Will's "AcidWarp for Linux" which was GPL licensed, this too
is and must be also GPL licensed.  See [gpl-3.0.md](gpl-3.0.md)

## Thanks

- Many heartfelt thanks to all previous authors especially Noah.
- Kudos to the Anthropic team as Claude helped out quite a bit - perhaps most notably 
  with the SDL 3 support, but also with cleanup, and increasing the logo scaling.
