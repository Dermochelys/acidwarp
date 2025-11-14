#!/bin/bash
set -e

# Cleanup function to remove hosts file block
cleanup() {
    echo ""
    echo "Removing developerservices2.apple.com block from /etc/hosts..."
    sudo sed -i '' '/developerservices2\.apple\.com/d' /etc/hosts
}
trap cleanup EXIT

# Block Apple developer services to prevent provisioning checks
echo "Blocking developerservices2.apple.com..."
sudo bash -c "echo '127.0.0.1 developerservices2.apple.com' >>/etc/hosts"

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

  # Verify the simulator actually booted
  FINAL_STATE=$(xcrun simctl list devices | grep "$DEVICE_UUID" | grep -oE '\((Booted|Shutdown|Shutting Down)\)' | tr -d '()')
  if [ "$FINAL_STATE" != "Booted" ]; then
    echo "Error: Simulator failed to boot within 60 seconds (final state: $FINAL_STATE)"
    exit 1
  fi
else
  echo "Simulator is already booted"
fi

# Give it a bit more time to fully initialize
echo "Waiting 5 more seconds for the emulator to finish initializing..."
sleep 5

echo ""
echo "Building for testing..."
xcodebuild build-for-testing \
  -project acidwarp-ios.xcodeproj \
  -scheme acidwarpUITests \
  -configuration Release \
  -destination "platform=iOS Simulator,id=$DEVICE_UUID" \
  -derivedDataPath ./build \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGNING_ALLOWED=NO

echo ""
echo "Running UI tests..."
xcodebuild test-without-building \
  -project acidwarp-ios.xcodeproj \
  -scheme acidwarpUITests \
  -configuration Release \
  -destination "platform=iOS Simulator,id=$DEVICE_UUID" \
  -derivedDataPath ./build \
  -parallel-testing-enabled NO \
  -resultBundlePath ./build/Logs/Test/Test-results.xcresult \
  -retry-tests-on-failure

echo ""
echo "=== UI Tests Completed ==="

# Shutdown the simulator
echo ""
echo "Shutting down simulator..."
xcrun simctl shutdown "$DEVICE_UUID" || true
echo "Simulator shut down"
