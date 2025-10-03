# Acid Warp for Android and iOS / iPadOS

## What is Acid Warp?

Acid Warp is an eye candy program which displays various patterns and
animates them by changing the palette. Originally it was an MS-DOS / Windows
program by Noah Spurrier and Mark Bilk and was made in 1992/1993. 

This is a port by Matthew Zavislak based on the previous [port](https://github.com/dreamlayers/acidwarp) by [Boris Gjenero](https://github.com/dreamlayers)
to the SDL 1.2 / 2 library. That in turn is based on the Linux SVGALib port 
by Steven Wills.

This port can be built for Android phones, tablets, and TVs, as well as for iOS phones and iPadOS tablets.

## Using the program (Android TV or devices with keyboards)

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

### 1.3.0
Add iOS / iPadOS support and also support logo display on smaller screen sizes.

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

As this is a descendent of Steven Will's "AcidWarp for Linux" which was GPL licensed, this too
is and must be also GPL licensed.  See [gpl-3.0.md](gpl-3.0.md)

## Thanks

- Many heartfelt thanks to all previous authors especially Noah.
- Kudos to the Anthropic team as Claude helped out quite a bit - perhaps most notably 
  with the SDL 3 support, but also with cleanup, and increasing the logo scaling.
- Additional thanks to [Scott Ostler](https://github.com/scottostler)), for his mentoring with the iOS / iPadOS port.
