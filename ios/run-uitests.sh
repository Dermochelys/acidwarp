#!/bin/bash
set -e

echo "=== Running iOS UI Tests ==="
echo ""

cd "$(dirname "$0")"

# Detect architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

xcodebuild test \
  -project acidwarp-ios.xcodeproj \
  -scheme acidwarpUITests \
  -configuration Release \
  -destination "platform=iOS Simulator,arch=$ARCH,OS=26.1,name=iPhone 17 Pro" \
  -parallel-testing-enabled NO \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGNING_ALLOWED=NO

echo ""
echo "=== UI Tests Completed ==="
