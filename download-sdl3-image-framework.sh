#!/bin/bash
set -e

# Read SDL_image version from repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDL_IMAGE_VERSION=$(cat "$SCRIPT_DIR/SDL3_IMAGE_VERSION")
SDL_IMAGE_DMG="SDL3_image-${SDL_IMAGE_VERSION}.dmg"
SDL_IMAGE_URL="https://github.com/libsdl-org/SDL_image/releases/download/release-${SDL_IMAGE_VERSION}/${SDL_IMAGE_DMG}"

# Target location for SDL3_image.xcframework (repo root)
TARGET_FRAMEWORK="$SCRIPT_DIR/SDL3_image.xcframework"
SDL_IMAGE_HEADER="$TARGET_FRAMEWORK/ios-arm64/SDL3_image.framework/Headers/SDL_image.h"

echo "Checking for SDL3_image.xcframework..."

# Check if framework already exists
if [ -d "$TARGET_FRAMEWORK" ]; then
    echo "SDL3_image.xcframework found. Checking version..."

    # Check if SDL_image.h exists
    if [ -f "$SDL_IMAGE_HEADER" ]; then
        # Extract version from header macros (SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_MICRO_VERSION)
        MAJOR=$(grep "^#define SDL_IMAGE_MAJOR_VERSION" "$SDL_IMAGE_HEADER" | awk '{print $3}')
        MINOR=$(grep "^#define SDL_IMAGE_MINOR_VERSION" "$SDL_IMAGE_HEADER" | awk '{print $3}')
        MICRO=$(grep "^#define SDL_IMAGE_MICRO_VERSION" "$SDL_IMAGE_HEADER" | awk '{print $3}')
        INSTALLED_VERSION="${MAJOR}.${MINOR}.${MICRO}"

        if [ "$INSTALLED_VERSION" = "$SDL_IMAGE_VERSION" ]; then
            echo "SDL3_image version $INSTALLED_VERSION matches required version $SDL_IMAGE_VERSION"
            exit 0
        else
            echo "SDL3_image version mismatch: installed=$INSTALLED_VERSION, required=$SDL_IMAGE_VERSION"
            echo "Removing old framework..."
            rm -rf "$TARGET_FRAMEWORK"
        fi
    else
        echo "Warning: SDL_image.h not found in framework, will re-download"
        rm -rf "$TARGET_FRAMEWORK"
    fi
fi

echo "Downloading SDL3_image ${SDL_IMAGE_VERSION} for macOS/iOS..."
echo "URL: $SDL_IMAGE_URL"

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
curl -L -o "$TEMP_DIR/$SDL_IMAGE_DMG" "$SDL_IMAGE_URL"

echo "Mounting DMG..."
# Mount the DMG (let hdiutil choose the mount point)
MOUNT_POINT=$(hdiutil attach "$TEMP_DIR/$SDL_IMAGE_DMG" -nobrowse | grep "/Volumes/" | awk '{print $3}')

if [ -z "$MOUNT_POINT" ]; then
    echo "Failed to mount DMG"
    exit 1
fi

echo "Extracting SDL3_image.xcframework..."
# Copy the framework to the repo root
if [ -d "$MOUNT_POINT/SDL3_image.xcframework" ]; then
    cp -R "$MOUNT_POINT/SDL3_image.xcframework" "$TARGET_FRAMEWORK"
    echo "SDL3_image.xcframework extracted successfully to $TARGET_FRAMEWORK"
else
    echo "Error: SDL3_image.xcframework not found in DMG"
    exit 1
fi

echo "SDL3_image setup complete!"
