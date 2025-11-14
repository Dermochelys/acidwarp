#!/bin/bash
set -e

# Read SDL version from repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDL_VERSION=$(cat "$SCRIPT_DIR/SDL_VERSION")
SDL_DMG="SDL3-${SDL_VERSION}.dmg"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/${SDL_DMG}"

# Target location for SDL3.xcframework (repo root)
TARGET_FRAMEWORK="$SCRIPT_DIR/SDL3.xcframework"
SDL_HEADER="$TARGET_FRAMEWORK/ios-arm64/SDL3.framework/Headers/SDL.h"

echo "Checking for SDL3.xcframework..."

# Check if framework already exists
if [ -d "$TARGET_FRAMEWORK" ]; then
    echo "SDL3.xcframework found. Checking version..."

    # Check if SDL.h exists - need to check SDL_version.h for version macros
    SDL_VERSION_HEADER="$TARGET_FRAMEWORK/ios-arm64/SDL3.framework/Headers/SDL_version.h"
    if [ -f "$SDL_VERSION_HEADER" ]; then
        # Extract version from header macros (SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION)
        MAJOR=$(grep "^#define SDL_MAJOR_VERSION" "$SDL_VERSION_HEADER" | awk '{print $3}')
        MINOR=$(grep "^#define SDL_MINOR_VERSION" "$SDL_VERSION_HEADER" | awk '{print $3}')
        MICRO=$(grep "^#define SDL_MICRO_VERSION" "$SDL_VERSION_HEADER" | awk '{print $3}')
        INSTALLED_VERSION="${MAJOR}.${MINOR}.${MICRO}"

        if [ "$INSTALLED_VERSION" = "$SDL_VERSION" ]; then
            echo "SDL3 version $INSTALLED_VERSION matches required version $SDL_VERSION"
            exit 0
        else
            echo "SDL3 version mismatch: installed=$INSTALLED_VERSION, required=$SDL_VERSION"
            echo "Removing old framework..."
            rm -rf "$TARGET_FRAMEWORK"
        fi
    else
        echo "Warning: SDL_version.h not found in framework, will re-download"
        rm -rf "$TARGET_FRAMEWORK"
    fi
fi

echo "Downloading SDL3 ${SDL_VERSION} for macOS/iOS..."
echo "URL: $SDL_URL"

# Create temp directory
TEMP_DIR=$(mktemp -d)
MOUNT_POINT=""

# Cleanup function
cleanup() {
    if [ -n "$MOUNT_POINT" ]; then
        echo "Unmounting DMG..."
        hdiutil detach "$MOUNT_POINT" 2>/dev/null || true
    fi
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

# Download DMG
curl -L -o "$TEMP_DIR/$SDL_DMG" "$SDL_URL"

echo "Mounting DMG..."
# Mount the DMG (let hdiutil choose the mount point)
MOUNT_POINT=$(hdiutil attach "$TEMP_DIR/$SDL_DMG" -nobrowse | grep "/Volumes/" | awk '{print $3}')

if [ -z "$MOUNT_POINT" ]; then
    echo "Failed to mount DMG"
    exit 1
fi

echo "Extracting SDL3.xcframework..."
# Copy the framework to the repo root
if [ -d "$MOUNT_POINT/SDL3.xcframework" ]; then
    cp -R "$MOUNT_POINT/SDL3.xcframework" "$TARGET_FRAMEWORK"
    echo "SDL3.xcframework extracted successfully to $TARGET_FRAMEWORK"
else
    echo "Error: SDL3.xcframework not found in DMG"
    exit 1
fi

echo "SDL3 setup complete!"
