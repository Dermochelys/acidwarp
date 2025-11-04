# Acid Warp for iOS / iPadOS

Not in iOS App Store yet, stay tuned!

Supports devices running iOS / iPadOS 15.6 or later.

## Technical Details
- Based on a [fork](https://github.com/Dermochelys/acidwarp) of [dreamlayers/acidwarp](https://github.com/dreamlayers/acidwarp), which is embedded as a [submodule](app/jni).
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.

## Building

- Use Xcode 26 or later.
- SDL3 and SDL3_image are automatically downloaded during the build via Run Script build phases. The scripts check if `SDL3.xcframework` and `SDL3_image.xcframework` exist in the repo root and download them if needed based on the versions in `SDL_VERSION` and `SDL3_IMAGE_VERSION`.

## Previous ports
- See the `previous_ports` folder inside the [submodule](https://github.com/Dermochelys/acidwarp).

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
