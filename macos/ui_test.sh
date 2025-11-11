#!/bin/bash
set -e

SCREENSHOT_DIR="screenshots"
APP_PATH="build/Build/Products/Release/Acid Warp.app"
APP_BINARY="$APP_PATH/Contents/MacOS/Acid Warp"

mkdir -p "$SCREENSHOT_DIR"

echo "Launching Acid Warp..."
# Launch the app binary directly in background
"$APP_BINARY" &
APP_PID=$!

# Wait for app to start
sleep 2

echo "App launched with PID: $APP_PID"

# Verify app is running
if ! ps -p $APP_PID > /dev/null 2>&1; then
    echo "✗ App failed to start"
    exit 1
fi

# Wait for app initialization
echo "Waiting for app initialization..."
sleep 5
screencapture "$SCREENSHOT_DIR/01-startup.png"
echo "✓ Captured startup screenshot"

# Check for SDL error dialogs
echo "Checking for error dialogs..."
ERROR_DIALOG=$(osascript -e 'tell application "System Events" to get name of every window of every process whose name contains "SDL Error"' 2>/dev/null || true)
if [ -n "$ERROR_DIALOG" ]; then
  echo "✗ Found SDL Error dialog!"
  screencapture "$SCREENSHOT_DIR/error-dialog.png"
  echo "✓ Captured error dialog screenshot"
  kill $APP_PID || true
  exit 1
fi
echo "✓ No error dialogs detected"

# Test pattern cycling with space key
for i in {1..5}; do
  echo "Test $i: Triggering next pattern..."
  osascript -e 'tell application "System Events" to keystroke space'
  sleep 2
  screencapture "$SCREENSHOT_DIR/02-pattern-$i.png"
  echo "✓ Captured pattern $i screenshot"
done

# Test mouse click - use AppleScript to click center of active window
echo "Testing mouse click..."
osascript <<'EOF'
tell application "System Events"
    try
        set frontApp to name of first application process whose frontmost is true
        set frontWindow to window 1 of application process frontApp
        set {x, y, width, height} to (get position of frontWindow) & (get size of frontWindow)
        set centerX to x + (width / 2)
        set centerY to y + (height / 2)
        click at {centerX, centerY}
    end try
end tell
EOF
sleep 1
screencapture "$SCREENSHOT_DIR/03-after-click.png"
echo "✓ Captured post-click screenshot"

# Test palette change
echo "Testing palette change (p key)..."
osascript -e 'tell application "System Events" to keystroke "p"'
sleep 1
screencapture "$SCREENSHOT_DIR/04-palette-change.png"
echo "✓ Captured palette change screenshot"

# Capture final state
sleep 2
screencapture "$SCREENSHOT_DIR/05-final.png"
echo "✓ Captured final screenshot"

# Quit gracefully
echo "Sending quit signal (Escape key)..."
osascript -e 'tell application "System Events" to key code 53' # Escape key code

# Wait for app to exit
sleep 1
if ps -p $APP_PID > /dev/null 2>&1; then
    # Force quit if still running
    kill $APP_PID 2>/dev/null || true
    sleep 1
fi

# Verify screenshots were created
SCREENSHOT_COUNT=$(ls -1 "$SCREENSHOT_DIR" | wc -l)

if [ "$SCREENSHOT_COUNT" -ge 9 ]; then
    echo ""
    echo "================================"
    echo "✓ UI test passed successfully!"
    echo "================================"
    echo "Screenshots captured: $SCREENSHOT_COUNT"
    exit 0
else
    echo ""
    echo "================================"
    echo "✗ UI test failed!"
    echo "================================"
    echo "Expected 9 screenshots, got $SCREENSHOT_COUNT"
    exit 1
fi
