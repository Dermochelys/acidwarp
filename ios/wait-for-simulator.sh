#!/bin/bash
set -e

echo "=== Waiting for iOS Simulator to be Ready ==="
echo ""

cd "$(dirname "$0")"

# Start the simulator
DEVICE_NAME="iPhone 17 Pro"
echo "Checking simulator: $DEVICE_NAME"

# Get the device UUID
DEVICE_UUID=$(xcrun simctl list devices | grep "$DEVICE_NAME" | grep -v "unavailable" | head -n 1 | grep -oE '\([0-9A-F-]+\)' | tr -d '()')

if [ -z "$DEVICE_UUID" ]; then
  echo "Error: Could not find device '$DEVICE_NAME'"
  exit 1
fi

echo "Device UUID: $DEVICE_UUID"

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

# Give it a bit more time to fully initialize
echo "Waiting 5 more seconds for the emulator to finish initializing..."
sleep 5

echo ""
echo "=== Simulator Ready ==="
