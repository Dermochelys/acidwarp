# Acid Warp for Android

<a href="https://play.google.com/store/apps/details?id=com.dermochelys.acidwarp"><img src="https://play.google.com/intl/en_us/badges/images/generic/en_badge_web_generic.png" height="55" alt="Get it on Google Play"></a>

Supports phones, tablets, and [Android TV](https://www.android.com/tv/) / [Google TV](https://tv.google/) devices running Android 10+.

## Building

- SDL3 and SDL3_image are automatically downloaded during the build process based on the versions specified in `SDL_VERSION` and `SDL3_IMAGE_VERSION` at the repo root. The Gradle build will download the correct `.aar` files from [SDL releases](https://github.com/libsdl-org/SDL/releases) and [SDL_image releases](https://github.com/libsdl-org/SDL_image/releases) to [app/libs](app/libs) if they're not already present.
- Build the app by running `./gradlew build`.

Most build dependencies are dynamically loaded, however, you must also ensure that some dependencies are preloaded via `Tools > SDK Manager` in Android Studio, or using [sdkmanager](https://developer.android.com/tools/sdkmanager):
- `cmake` with version matching that inside of [build.gradle](app/build.gradle)
- `Android SDK Tools` with version matching that of `buildToolsVersion`, found in [build.gradle](app/build.gradle)

## UI Testing

Automated UI tests use Android Instrumentation testing with UiAutomator to verify the app works correctly on Android devices and emulators.

**Running UI tests:**
```bash
./gradlew :app:connectedDebugAndroidTest
```

**Test Framework:**
- **Location**: `app/src/androidTest/java/com/dermochelys/acidwarp/ActivityUITest.kt`
- **Framework**: JUnit4 with UiAutomator for device control
- **Emulator**: API 36, x86_64, Nexus 6 profile (in CI)

**Key Features:**
- **Input Simulation**: `UiDevice.pressKeyCode()` for keyboard (KeyEvent.KEYCODE_N), `device.click()` for touch
- **Screenshot Capture**: `device.takeScreenshot()` saves screenshots to app's external files directory
- **Screenshot Retrieval**: `adb pull` copies screenshots from emulator to local machine
- **Error Detection**: Checks for SDL error dialogs during app launch
- **GPU Rendering**: Uses swiftshader_indirect for software GPU emulation in CI

**Test Sequence:**
1. Launch app and wait for initialization
2. Capture startup screenshot
3. Verify no SDL error dialogs appear
4. Test pattern cycling (5 iterations, 'n' key with 4s fade delay)
5. Test touch input (center screen tap)
6. Test palette change ('p' key)
7. Capture final screenshot

Screenshots are pulled to `screenshots/screenshots/` directory. CI uploads screenshots, logcat (build and test phases), and test result XMLs as artifacts.

## License

As this is a descendent of Steven Will's `AcidWarp for Linux` which was GPL licensed, this too
is and must also be GPL licensed.  See [gpl-3.0.md](../gpl-3.0.md)

![Acid Warp logo](../logo.png)
