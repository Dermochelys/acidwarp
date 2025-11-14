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

## UI Testing

Automated UI tests use XCTest's UI testing framework to verify the app works correctly on iOS and iPadOS simulators.

**Running UI tests:**
```bash
# Start simulator in background (parallel with build)
./start-simulator-background.sh

# Build tests (can run while simulator boots)
./build-uitests.sh

# Wait for simulator to be ready
./wait-for-simulator.sh

# Run tests
./test-uitests.sh
```

**Test Framework:**
- **Location**: `acidwarp-iosUITests/` (Swift UI tests)
- **Framework**: XCTest with XCUIApplication and XCUIElement APIs
- **Simulator**: iPhone 17 Pro (iOS 18.2+)

**Key Features:**
- **Split Build/Test Phases**: Separate build and test scripts improve reliability and allow parallel simulator startup
- **Simulator Warmup**: Background simulator start runs parallel to test build for faster CI runs
- **Simulator Wait Logic**: `wait-for-simulator.sh` polls for "Booted" state before running tests
- **Screenshot Handling**: Same extraction as macOS using `xcresulttool` and rename script
- **Network Blocking**: Blocks `developerservices2.apple.com` to prevent Xcode activation delays in CI
- **Graceful Cleanup**: Simulator shutdown after tests complete
- **Device Selection**: Uses iPhone 17 Pro simulator by UUID lookup

**Screenshot Extraction:**
Screenshots are embedded in `.xcresult` bundles and extracted using:
```bash
../extract-test-screenshots.sh ios ./build ./screenshots
```

CI uploads screenshots (PNG files), xcodebuild logs (build-without-testing and test-without-building), and .xcresult bundles as artifacts.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
