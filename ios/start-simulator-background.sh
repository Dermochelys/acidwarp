#!/bin/bash
set -e

echo "=== Starting iOS Simulator (Background) ==="
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
  echo "Booting simulator in background..."
  xcrun simctl boot "$DEVICE_UUID" || true
  echo "Boot command issued (not waiting for completion)"
else
  echo "Simulator is already booted"
fi

echo ""
echo "=== Simulator boot initiated ==="
