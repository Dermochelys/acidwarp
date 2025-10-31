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

    # Check if SDL.h exists
    if [ -f "$SDL_HEADER" ]; then
        # Extract version from header (format: "version X.Y.Z")
        INSTALLED_VERSION=$(grep -o "version [0-9]\+\.[0-9]\+\.[0-9]\+" "$SDL_HEADER" | head -1 | awk '{print $2}')

        if [ "$INSTALLED_VERSION" = "$SDL_VERSION" ]; then
            echo "SDL3 version $INSTALLED_VERSION matches required version $SDL_VERSION"
            exit 0
        else
            echo "SDL3 version mismatch: installed=$INSTALLED_VERSION, required=$SDL_VERSION"
            echo "Removing old framework..."
            rm -rf "$TARGET_FRAMEWORK"
        fi
    else
        echo "Warning: SDL.h not found in framework, will re-download"
        rm -rf "$TARGET_FRAMEWORK"
    fi
fi

echo "Downloading SDL3 ${SDL_VERSION} for macOS/iOS..."
echo "URL: $SDL_URL"

# Create temp directory
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Download DMG
curl -L -o "$TEMP_DIR/$SDL_DMG" "$SDL_URL"

echo "Mounting DMG..."
# Mount the DMG
MOUNT_POINT=$(hdiutil attach "$TEMP_DIR/$SDL_DMG" -nobrowse -mountpoint "$TEMP_DIR/sdl_mount" | grep "/Volumes/" | awk '{print $3}')

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
    hdiutil detach "$MOUNT_POINT"
    exit 1
fi

# Unmount the DMG
hdiutil detach "$MOUNT_POINT"

echo "Cleaning up DMG..."
rm -f "$TEMP_DIR/$SDL_DMG"

echo "SDL3 setup complete!"
