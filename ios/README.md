# Acid Warp for iOS / iPadOS

<a href="https://apps.apple.com/us/app/acid-warp/id6753610977?mt=12&itscg=30200&itsct=apps_box_badge&mttnsubad=6753610977" style="display: inline-block;"><img src="https://toolbox.marketingtools.apple.com/api/v2/badges/download-on-the-app-store/black/en-us?releaseDate=1760486400" alt="Download on the App Store" style="width: 123px; height: 41px; vertical-align: middle; object-fit: contain;" /></a>

Supports devices running iOS / iPadOS 15.6 or later.

## Building

- Use Xcode 26 or later.
- SDL3 and SDL3_image are automatically downloaded during the build via Run Script build phases. The scripts check if `SDL3.xcframework` and `SDL3_image.xcframework` exist in the repo root and download them if needed based on the versions in `SDL_VERSION` and `SDL3_IMAGE_VERSION`.
- SDL3_image is built from a pre-release commit (ef63a6a) that includes an exported symbols list for App Store compliance. This eliminates the need for JXL symbol renaming workarounds.

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
