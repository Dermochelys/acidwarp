#!/bin/bash
set -e

DISPLAY=:99
SCREENSHOT_DIR="screenshots"
mkdir -p "$SCREENSHOT_DIR"

echo "Starting Xvfb..."
Xvfb $DISPLAY -screen 0 1920x1080x24 &
XVFB_PID=$!
export DISPLAY

sleep 2

echo "Launching Acid Warp..."
timeout 30s ./build/acidwarp-linux &
APP_PID=$!

# Test sequence
echo "Waiting for app initialization..."
sleep 5
scrot "$SCREENSHOT_DIR/01-startup.png"
echo "✓ Captured startup screenshot"

# Check for SDL error dialogs
echo "Checking for error dialogs..."
ERROR_WINDOWS=$(xdotool search --name "SDL Error" 2>/dev/null || true)
if [ -n "$ERROR_WINDOWS" ]; then
  echo "✗ Found SDL Error dialog window!"
  kill $APP_PID || true
  kill $XVFB_PID || true
  exit 1
fi
echo "✓ No error dialogs detected"

# Test pattern cycling with n key
for i in {1..5}; do
  echo "Test $i: Triggering next pattern..."
  xdotool key n
  sleep 4  # Wait for fade-out + fade-in to complete
  scrot "$SCREENSHOT_DIR/02-pattern-$i.png"
  echo "✓ Captured pattern $i screenshot"
done

# Test mouse input
echo "Testing mouse click..."
xdotool mousemove 960 540 click 1
sleep 1
scrot "$SCREENSHOT_DIR/03-after-click.png"
echo "✓ Captured post-click screenshot"

# Test palette change
echo "Testing palette change (p key)..."
xdotool key p
sleep 1
scrot "$SCREENSHOT_DIR/04-palette-change.png"
echo "✓ Captured palette change screenshot"

# Capture final state
sleep 2
scrot "$SCREENSHOT_DIR/05-final.png"
echo "✓ Captured final screenshot"

# Quit gracefully
echo "Sending quit signal (Escape key)..."
xdotool key Escape

# Wait for app to exit
wait $APP_PID
EXIT_CODE=$?

# Cleanup Xvfb
kill $XVFB_PID || true

# Evaluate results
if [ $EXIT_CODE -eq 0 ] || [ $EXIT_CODE -eq 124 ]; then
  echo ""
  echo "================================"
  echo "✓ UI test passed successfully!"
  echo "================================"
  echo "Screenshots captured: $(ls -1 $SCREENSHOT_DIR | wc -l)"
  exit 0
else
  echo ""
  echo "================================"
  echo "✗ UI test failed!"
  echo "================================"
  echo "App crashed with exit code $EXIT_CODE"
  exit 1
fi
