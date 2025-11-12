#!/bin/bash
set -e

echo "=== Running iOS UI Tests ==="
echo ""

cd "$(dirname "$0")"

# Detect architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

# Start the simulator
DEVICE_NAME="iPhone 17 Pro"
echo "Starting simulator: $DEVICE_NAME"

# Get the device UUID
DEVICE_UUID=$(xcrun simctl list devices | grep "$DEVICE_NAME" | grep -v "unavailable" | head -n 1 | grep -oE '\([0-9A-F-]+\)' | tr -d '()')

if [ -z "$DEVICE_UUID" ]; then
  echo "Error: Could not find device '$DEVICE_NAME'"
  exit 1
fi

echo "Device UUID: $DEVICE_UUID"

# Check if already booted, if not boot it
DEVICE_STATE=$(xcrun simctl list devices | grep "$DEVICE_UUID" | grep -oE '\((Booted|Shutdown|Shutting Down)\)' | tr -d '()')
echo "Device state: $DEVICE_STATE"

if [ "$DEVICE_STATE" != "Booted" ]; then
  echo "Booting simulator..."
  xcrun simctl boot "$DEVICE_UUID" || true

  # Wait for simulator to boot
  echo "Waiting for simulator to boot..."
  for i in {1..30}; do
    sleep 2
    STATE=$(xcrun simctl list devices | grep "$DEVICE_UUID" | grep -oE '\((Booted|Shutdown|Shutting Down)\)' | tr -d '()')
    if [ "$STATE" == "Booted" ]; then
      echo "Simulator booted successfully"
      break
    fi
    echo "Still waiting... ($i/30)"
  done
else
  echo "Simulator is already booted"
fi

# Give it a bit more time to fully initialize
sleep 5

echo ""
echo "Running tests..."

xcodebuild test \
  -project acidwarp-ios.xcodeproj \
  -scheme acidwarpUITests \
  -configuration Release \
  -destination "platform=iOS Simulator,arch=$ARCH,name=iPhone 17 Pro" \
  -parallel-testing-enabled NO \
  -resultBundlePath ./build/Logs/Test/Test-results.xcresult \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGNING_ALLOWED=NO

echo ""
echo "=== UI Tests Completed ==="
