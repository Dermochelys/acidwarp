# Acid Warp for iOS / iPadOS

<a href="https://apps.apple.com/us/app/acid-warp/id6753610977?mt=12&itscg=30200&itsct=apps_box_badge&mttnsubad=6753610977" style="display: inline-block;"><img src="https://toolbox.marketingtools.apple.com/api/v2/badges/download-on-the-app-store/black/en-us?releaseDate=1760486400" alt="Download on the App Store" style="width: 123px; height: 41px; vertical-align: middle; object-fit: contain;" /></a>

Supports devices running iOS / iPadOS 15.6 or later.

## Building

- Use Xcode 26 or later.
- SDL3 and SDL3_image are automatically downloaded during the build via Run Script build phases. The scripts check if `SDL3.xcframework` and `SDL3_image.xcframework` exist in the repo root and download them if needed based on the versions in `SDL_VERSION` and `SDL3_IMAGE_VERSION`.

## Workaround for SDL_image issue with libjxl

- Starting with iOS 18 and Xcode 16, Apple bundled libjxl (JPEG XL decoder library) into iOS as an internal/private API. When apps are submitted to the App Store, Apple's automated review
  process scans for symbols matching these private APIs and rejects apps that appear to use them - even if the app is using its own bundled version of libjxl rather than calling Apple's
  private APIs.

- The workaround (implemented in build-sdl3-image-xcode.sh:56-125) renames JXL symbols by prefixing them with SDL_ to avoid App Store rejection. Specifically, it renames 8 JXL decoder
  functions:

  - JxlDecoderCreate → SDL_JxlDecoderCreate
  - JxlDecoderDestroy → SDL_JxlDecoderDestroy
  - JxlDecoderGetBasicInfo → SDL_JxlDecoderGetBasicInfo
  - JxlDecoderImageOutBufferSize → SDL_JxlDecoderImageOutBufferSize
  - JxlDecoderProcessInput → SDL_JxlDecoderProcessInput
  - JxlDecoderSetImageOutBuffer → SDL_JxlDecoderSetImageOutBuffer
  - JxlDecoderSetInput → SDL_JxlDecoderSetInput
  - JxlDecoderSubscribeEvents → SDL_JxlDecoderSubscribeEvents

- This is done via C preprocessor macros added during SDL3_image's build process.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
