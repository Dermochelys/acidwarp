#!/bin/bash
set -e

echo "=== Running iOS UI Tests ==="
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

# Run UI tests with verbose output
echo ""
echo "Running UI tests (verbose mode)..."
set -o pipefail
xcodebuild test-without-building \
  -project acidwarp-ios.xcodeproj \
  -scheme acidwarpUITests \
  -configuration Debug \
  -destination "platform=iOS Simulator,id=$DEVICE_UUID" \
  -derivedDataPath ./build \
  -parallel-testing-enabled NO \
  -retry-tests-on-failure \
  -showBuildTimingSummary \
  -verbose | tee xcodebuild-test-without-building.log

echo ""
echo "=== UI Tests Completed ==="

# Shutdown the simulator
echo ""
echo "Shutting down simulator..."
xcrun simctl shutdown "$DEVICE_UUID" || true
echo "Simulator shut down"
