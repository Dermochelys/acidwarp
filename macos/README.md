# Acid Warp for macOS

<a href="https://apps.apple.com/us/app/acid-warp/id6753610977?mt=12&itscg=30200&itsct=apps_box_badge&mttnsubad=6753610977" style="display: inline-block;"><img src="https://toolbox.marketingtools.apple.com/api/v2/badges/download-on-the-app-store/black/en-us?releaseDate=1760486400" alt="Download on the App Store" style="width: 123px; height: 41px; vertical-align: middle; object-fit: contain;" /></a>

Supports devices running macOS 11.5 or later.

## Technical Details
- Based on a [fork](https://github.com/Dermochelys/acidwarp) of [dreamlayers/acidwarp](https://github.com/dreamlayers/acidwarp), which is embedded as a [submodule](acidwarp/acidwarp).
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.

## Building

Xcode 26+ has been used to develop the app.  It may be possible to use older Xcode versions, but that is left as an exercise to those interested.

SDL3 and SDL3_image are automatically downloaded during the build via Run Script build phases. The scripts check if `SDL3.xcframework` and `SDL3_image.xcframework` exist in the repo root and download them if needed based on the versions in `SDL_VERSION` and `SDL3_IMAGE_VERSION`.

## Previous ports
- See the submodule's [README.md](https://github.com/Dermochelys/acidwarp) for more details.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
