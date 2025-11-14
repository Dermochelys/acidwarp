#!/bin/bash
set -e

echo "=== Running macOS UI Tests ==="
echo ""

cd "$(dirname "$0")"

# Detect architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

# Build for testing
echo ""
echo "Building for testing..."
xcodebuild build-for-testing \
  -project acidwarp-macos.xcodeproj \
  -scheme acidwarp-macos \
  -configuration Release \
  -destination 'platform=macOS' \
  -derivedDataPath ./build \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGNING_ALLOWED=NO

# Sign test runner
echo ""
echo "Signing test runner..."
codesign --force --deep --sign - build/Build/Products/Release/acidwarp-macosUITests-Runner.app || true

# Run UI tests
echo ""
echo "Running UI tests..."
set -o pipefail
xcodebuild test-without-building \
  -project acidwarp-macos.xcodeproj \
  -scheme acidwarp-macos \
  -configuration Release \
  -destination 'platform=macOS' \
  -derivedDataPath ./build | tee xcodebuild.log

echo ""
echo "=== UI Tests Completed ==="
