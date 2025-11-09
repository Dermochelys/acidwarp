#!/bin/bash
set -e

# Read SDL_image version from repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDL_IMAGE_VERSION=$(cat "$SCRIPT_DIR/SDL3_IMAGE_VERSION")

# Target location for SDL3_image.xcframework (repo root)
TARGET_FRAMEWORK="$SCRIPT_DIR/SDL3_image.xcframework"
SDL_IMAGE_HEADER="$TARGET_FRAMEWORK/ios-arm64/SDL3_image.framework/Headers/SDL_image.h"

echo "Checking for SDL3_image.xcframework..."

# Check if framework already exists
if [ -d "$TARGET_FRAMEWORK" ]; then
    echo "SDL3_image.xcframework found. Checking version..."

    # Check if SDL_image.h exists
    if [ -f "$SDL_IMAGE_HEADER" ]; then
        # Extract version from header (format: "version X.Y.Z")
        INSTALLED_VERSION=$(grep -o "version [0-9]\+\.[0-9]\+\.[0-9]\+" "$SDL_IMAGE_HEADER" | head -1 | awk '{print $2}')

        if [ "$INSTALLED_VERSION" = "$SDL_IMAGE_VERSION" ]; then
            echo "SDL3_image version $INSTALLED_VERSION matches required version $SDL_IMAGE_VERSION"
            echo "Custom-built framework already exists."
            exit 0
        else
            echo "SDL3_image version mismatch: installed=$INSTALLED_VERSION, required=$SDL_IMAGE_VERSION"
            echo "Removing old framework..."
            rm -rf "$TARGET_FRAMEWORK"
        fi
    else
        echo "Warning: SDL_image.h not found in framework, will rebuild"
        rm -rf "$TARGET_FRAMEWORK"
    fi
fi

echo "Building SDL3_image ${SDL_IMAGE_VERSION} from source for iOS using Xcode..."

# Create temp directory
TEMP_DIR=$(mktemp -d)

# Cleanup function
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

cd "$TEMP_DIR"

# Clone SDL_image
echo "Cloning SDL_image..."
git clone --depth 1 --branch "release-${SDL_IMAGE_VERSION}" https://github.com/libsdl-org/SDL_image.git
cd SDL_image

# Configure the build to rename JXL symbols for App Store compliance
echo "Configuring build to rename JXL symbols..."

# Add symbol renaming to IMG_jxl.c
if [ -f "src/IMG_jxl.c" ]; then
    # Add symbol renaming macros at the top of IMG_jxl.c
    cat > src/IMG_jxl.c.new <<'PREFIX_CODE'
/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>
*/

/* Rename JXL symbols to avoid App Store private API detection */
#define JxlDecoderCreate SDL_JxlDecoderCreate
#define JxlDecoderDestroy SDL_JxlDecoderDestroy
#define JxlDecoderGetBasicInfo SDL_JxlDecoderGetBasicInfo
#define JxlDecoderImageOutBufferSize SDL_JxlDecoderImageOutBufferSize
#define JxlDecoderProcessInput SDL_JxlDecoderProcessInput
#define JxlDecoderSetImageOutBuffer SDL_JxlDecoderSetImageOutBuffer
#define JxlDecoderSetInput SDL_JxlDecoderSetInput
#define JxlDecoderSubscribeEvents SDL_JxlDecoderSubscribeEvents

PREFIX_CODE
    # Append the original file content
    cat src/IMG_jxl.c >> src/IMG_jxl.c.new
    mv src/IMG_jxl.c.new src/IMG_jxl.c
    echo "Added symbol renaming to IMG_jxl.c"
fi

# Modify the jxl library build to export symbols with prefix
# We need to add -alias or -reexport flags to rename symbols in the built library
echo "Configuring JXL library build with symbol prefixes..."
python3 <<'PYTHON_SCRIPT'
import sys
import re

project_file = "Xcode/jxl/jxl.xcodeproj/project.pbxproj"

# Read the project file
with open(project_file, 'r') as f:
    content = f.read()

# Add preprocessor macros to rename JXL symbols during compilation
# Find the GCC_PREPROCESSOR_DEFINITIONS line and add our macros
symbols_to_rename = [
    'JxlDecoderCreate=SDL_JxlDecoderCreate',
    'JxlDecoderDestroy=SDL_JxlDecoderDestroy',
    'JxlDecoderGetBasicInfo=SDL_JxlDecoderGetBasicInfo',
    'JxlDecoderImageOutBufferSize=SDL_JxlDecoderImageOutBufferSize',
    'JxlDecoderProcessInput=SDL_JxlDecoderProcessInput',
    'JxlDecoderSetImageOutBuffer=SDL_JxlDecoderSetImageOutBuffer',
    'JxlDecoderSetInput=SDL_JxlDecoderSetInput',
    'JxlDecoderSubscribeEvents=SDL_JxlDecoderSubscribeEvents',
]

# Add the defines to the preprocessor definitions
for symbol in symbols_to_rename:
    # This will add them to both debug and release configs
    content = re.sub(
        r'(GCC_PREPROCESSOR_DEFINITIONS = \()',
        r'\1\n\t\t\t\t\t' + symbol + ',',
        content
    )

# Write back
with open(project_file, 'w') as f:
    f.write(content)

print("Modified JXL library project to rename symbols")
PYTHON_SCRIPT

# Disable code signing and fix deployment target in the SDL_image Xcode project
echo "Configuring Xcode project settings..."
python3 <<'PYTHON_SCRIPT'
import sys
import re

project_file = "Xcode/SDL_image.xcodeproj/project.pbxproj"

# Read the project file
with open(project_file, 'r') as f:
    content = f.read()

# Remove ProvisioningStyle attributes from target attributes
# This is often what triggers the provisioning profile requirement
content = re.sub(r'ProvisioningStyle = [^;]+;', '', content)

# Remove or disable code signing settings (all variations)
content = re.sub(r'CODE_SIGN_IDENTITY = "[^"]*";', 'CODE_SIGN_IDENTITY = "";', content)
content = re.sub(r'CODE_SIGN_IDENTITY = [^;"\n]+;', 'CODE_SIGN_IDENTITY = "";', content)
content = re.sub(r'"CODE_SIGN_IDENTITY\[sdk=[^\]]+\]" = "[^"]*";', '', content)

# Disable code signing requirements
content = re.sub(r'CODE_SIGNING_REQUIRED = YES;', 'CODE_SIGNING_REQUIRED = NO;', content)
content = re.sub(r'CODE_SIGNING_ALLOWED = YES;', 'CODE_SIGNING_ALLOWED = NO;', content)

# Add CODE_SIGNING_REQUIRED = NO if not present
if 'CODE_SIGNING_REQUIRED' not in content:
    # Add it to the first buildSettings section found
    content = re.sub(
        r'(buildSettings = \{)',
        r'\1\n\t\t\t\tCODE_SIGNING_REQUIRED = NO;',
        content,
        count=1
    )

# Clear development team
content = re.sub(r'DEVELOPMENT_TEAM = "[^"]*";', 'DEVELOPMENT_TEAM = "";', content)
content = re.sub(r'DEVELOPMENT_TEAM = [^;"\n]+;', 'DEVELOPMENT_TEAM = "";', content)

# Remove DevelopmentTeam from attributes
content = re.sub(r'DevelopmentTeam = [^;]+;', '', content)

# Remove signing certificate
content = re.sub(r'CODE_SIGN_STYLE = [^;]+;', 'CODE_SIGN_STYLE = Manual;', content)

# Fix iOS deployment target to 12.0 (minimum supported by current Xcode)
# Match the app's deployment target of 15.6 for consistency
content = re.sub(r'IPHONEOS_DEPLOYMENT_TARGET = [^;]+;', 'IPHONEOS_DEPLOYMENT_TARGET = 15.6;', content)

# Fix headermap settings to address headermap warning
# Replace any existing values (xcodebuild command line will also set these)
content = re.sub(r'ALWAYS_SEARCH_USER_PATHS = [^;]+;', 'ALWAYS_SEARCH_USER_PATHS = NO;', content)
content = re.sub(r'USE_HEADERMAP = [^;]+;', 'USE_HEADERMAP = NO;', content)

# Remove Carbon Resources build phases to suppress deprecation warning
# Find and remove PBXRezBuildPhase sections
content = re.sub(r'/\* Begin PBXRezBuildPhase section \*/\n.*?/\* End PBXRezBuildPhase section \*/\n', '', content, flags=re.DOTALL)
# Also remove references to Rez build phases from buildPhases arrays
content = re.sub(r'[0-9A-F]{24} /\* Rez \*/,?\s*\n?', '', content)

# Write back
with open(project_file, 'w') as f:
    f.write(content)

print("Configured Xcode project: disabled code signing, updated deployment target to 15.6, fixed headermap settings (ALWAYS_SEARCH_USER_PATHS=NO, USE_HEADERMAP=NO), removed Carbon Resources phases")
PYTHON_SCRIPT

# Build the xcframework using direct platform builds
echo "Building SDL3_image.xcframework for each platform..."

# Unset conflicting deployment target variables to avoid platform conflicts
unset MACOSX_DEPLOYMENT_TARGET
unset IPHONEOS_DEPLOYMENT_TARGET
unset TVOS_DEPLOYMENT_TARGET
unset WATCHOS_DEPLOYMENT_TARGET

# Common build settings to disable code signing, set deployment target, and fix headermap
BUILD_SETTINGS=(
    CODE_SIGN_IDENTITY=""
    CODE_SIGNING_REQUIRED=NO
    CODE_SIGNING_ALLOWED=NO
    DEVELOPMENT_TEAM=""
    IPHONEOS_DEPLOYMENT_TARGET=15.6
    ALWAYS_SEARCH_USER_PATHS=NO
    USE_HEADERMAP=NO
)

# Build for iOS devices
echo "Building for iOS (arm64)..."
echo "Note: 'no debug symbols' warnings are expected and harmless for Release builds"
xcodebuild archive \
    -project Xcode/SDL_image.xcodeproj \
    -scheme SDL3_image \
    -configuration Release \
    -destination "generic/platform=iOS" \
    -archivePath "build/ios.xcarchive" \
    SKIP_INSTALL=NO \
    BUILD_LIBRARY_FOR_DISTRIBUTION=YES \
    "${BUILD_SETTINGS[@]}" 2>&1 | grep -v "warning: no debug symbols" || true

# Build for iOS Simulator
echo "Building for iOS Simulator (arm64, x86_64)..."
xcodebuild archive \
    -project Xcode/SDL_image.xcodeproj \
    -scheme SDL3_image \
    -configuration Release \
    -destination "generic/platform=iOS Simulator" \
    -archivePath "build/iossimulator.xcarchive" \
    SKIP_INSTALL=NO \
    BUILD_LIBRARY_FOR_DISTRIBUTION=YES \
    "${BUILD_SETTINGS[@]}" 2>&1 | grep -v "warning: no debug symbols" || true

# Create the xcframework from the archives
echo "Creating xcframework..."
xcodebuild -create-xcframework \
    -framework "build/ios.xcarchive/Products/Library/Frameworks/SDL3_image.framework" \
    -framework "build/iossimulator.xcarchive/Products/Library/Frameworks/SDL3_image.framework" \
    -output "build/SDL3_image.xcframework"

# The build output will be in the build directory
echo "Copying built xcframework..."
if [ -d "build/SDL3_image.xcframework" ]; then
    cp -R "build/SDL3_image.xcframework" "$TARGET_FRAMEWORK"
else
    echo "Error: SDL3_image.xcframework not found after build"
    echo "Searched in: build/SDL3_image.xcframework"
    ls -la build/ || echo "build directory does not exist"
    exit 1
fi

echo "SDL3_image.xcframework built successfully!"
echo "JXL symbols have been renamed to SDL_Jxl* to comply with App Store requirements."
