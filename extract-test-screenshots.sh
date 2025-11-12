#!/bin/bash
set -e

# Usage: extract-test-screenshots.sh <platform> <build-dir> <output-dir>
# Example: extract-test-screenshots.sh macos ./build ./screenshots

PLATFORM=$1
BUILD_DIR=$2
OUTPUT_DIR=$3

if [ -z "$PLATFORM" ] || [ -z "$BUILD_DIR" ] || [ -z "$OUTPUT_DIR" ]; then
  echo "Usage: $0 <platform> <build-dir> <output-dir>"
  echo "Example: $0 macos ./build ./screenshots"
  exit 1
fi

echo "=== Extracting test screenshots for $PLATFORM ==="
echo "Build directory: $BUILD_DIR"
echo "Output directory: $OUTPUT_DIR"

mkdir -p "$OUTPUT_DIR"

# Find the test results bundle
RESULTS_BUNDLE=$(find "$BUILD_DIR/Logs/Test" -name "*.xcresult" 2>/dev/null | head -1)

if [ -n "$RESULTS_BUNDLE" ]; then
  echo "Found test results: $RESULTS_BUNDLE"

  # Export attachments from xcresult bundle using xcresulttool
  xcrun xcresulttool export attachments \
    --path "$RESULTS_BUNDLE" \
    --output-path "$OUTPUT_DIR" || {
    echo "Failed to export attachments, trying fallback method..."
    # Fallback: search entire bundle for PNGs
    find "$RESULTS_BUNDLE" -name "*.png" -exec cp {} "$OUTPUT_DIR/" \; 2>/dev/null || true
  }

  # Rename files if rename script exists
  RENAME_SCRIPT="$PLATFORM/rename_screenshots.py"
  if [ -f "$RENAME_SCRIPT" ]; then
    echo "Running rename script: $RENAME_SCRIPT"
    cd "$OUTPUT_DIR"
    python3 "../$RENAME_SCRIPT"
    cd - > /dev/null
  fi

  echo "Screenshots extracted:"
  ls -lh "$OUTPUT_DIR" || echo "No screenshots found"
else
  echo "No xcresult bundle found in $BUILD_DIR/Logs/Test"
  exit 1
fi

echo "=== Screenshot extraction completed ==="
