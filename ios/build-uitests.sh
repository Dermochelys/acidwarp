#!/bin/bash
set -e

echo "=== Building iOS UI Tests ==="
echo ""

cd "$(dirname "$0")"

# Get the device UUID
DEVICE_NAME="iPhone 17 Pro"
DEVICE_UUID=$(xcrun simctl list devices | grep "$DEVICE_NAME" | grep -v "unavailable" | head -n 1 | grep -oE '\([0-9A-F-]+\)' | tr -d '()')

if [ -z "$DEVICE_UUID" ]; then
  echo "Error: Could not find device '$DEVICE_NAME'"
  exit 1
fi

echo "Device UUID: $DEVICE_UUID"

echo ""
echo "Building for testing..."
set -o pipefail
xcodebuild build-for-testing \
  -project acidwarp-ios.xcodeproj \
  -scheme acidwarpUITests \
  -configuration Debug \
  -destination "platform=iOS Simulator,id=$DEVICE_UUID" \
  -derivedDataPath ./build \
  -showBuildTimingSummary \
  ONLY_ACTIVE_ARCH=YES \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGNING_ALLOWED=NO | tee xcodebuild-build-without-testing.log

# Sign test runner
echo ""
echo "Signing test runner..."
codesign --force --deep --sign - build/Build/Products/Debug-iphonesimulator/acidwarpUITests-Runner.app || true

echo ""
echo "=== Build Completed ==="
