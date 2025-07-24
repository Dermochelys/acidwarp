# Acid Warp for Android

## What is Acid Warp?

Acid Warp is an eye candy program which displays various patterns and
animates them by changing the palette. Originally it was an MS-DOS
program by Noah Spurrier. This is a port by Matthew Zavislak based on the
port by Boris Gjenero using the SDL 1.2 / 2 / 3 library. It in turn is 
based on the Linux SVGALib port by Steven Wills.

This port can be built for Android devices and is currently limited to TV's
as a conscious decision due to the high CPU usage which would drain a 
battery powered device quickly.

## Using the program

Use the following remote control buttons to control the program:

| Key        | Action                     |
|------------|----------------------------|
| **Up**     | Rotate palette faster      |
| **Down**   | Rotate palette slower      |
| **Right**  | Switch to the next palette |
| **Select** | Switch to the next pattern |
| **Back**   | Exit the app               |
| **Home**   | Exit the app               |

Acid Warp originally worked in 320x200 256 colour VGA mode and generated
patterns using lookup tables to avoid slow floating point calculations.
This port defaults to using floating point for image generation. This
allows many patterns to be scaled up to high resolution.

## Building the program

Build the program by running `./gradlew build`. Version 3 of the SDL library
is required, and is included as an Android AAR in `app/libs`.

## Version History

### 1.0
The app uses SDL 3 and features a stripped down version targeted at Android
TV. The reasoning is that due to the high CPU computation use the device must
be plugged in to a power source, and is also best enjoyed on a large screen.

#### Additions / Enhancements
- Increase logo size by 4x (thanks to Anthropic Claude for accomplishing that)
- Smoother "next" handling - simply treat as a timer expiration, which enables
  the same fadeout/fadein transition as would happen without "next" being issued
- Add Android TV remote support
  - Faster / slower via TV remote UP/DOWN
  - Next            via TV remote SELECT
- Fix most warnings related to int/long/double handling

#### Removals
- Lookup table feature
- Compatibility with SDL before 3
- Icon generation
- Web support

## Further resources

For more information about the SDL port, see: [README_SDL.md](README_SDL.md)
For more information about the Linux port, see: [README_LINUX_V_1_0](README_LINUX_V_1_0).
For more information about the original DOS version, see: [README_DOS_V_4_1](README_DOS_V_4_1).

*Note*: there is an alternate spelling which is `Acidwarp`.  Based on the original DOS `README`,
it is believed that `Acid Warp` is the correct spelling.