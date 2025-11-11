#!/bin/bash
set -e

SCREENSHOT_DIR="screenshots"
APP_PATH="build/Build/Products/Release/Acid Warp.app"
APP_BINARY="$APP_PATH/Contents/MacOS/Acid Warp"

mkdir -p "$SCREENSHOT_DIR"

echo "Launching Acid Warp..."
# Launch the app using 'open' to properly register with macOS
open "$APP_PATH"

# Wait for app to start
sleep 3

# Get the PID of the launched app
APP_PID=$(pgrep -f "Acid Warp.app" | head -1)
echo "App launched with PID: $APP_PID"

# Verify app is running
if [ -z "$APP_PID" ] || ! ps -p $APP_PID > /dev/null 2>&1; then
    echo "✗ App failed to start"
    exit 1
fi

# Activate/bring the app to front
echo "Activating Acid Warp window..."
osascript -e 'tell application "Acid Warp" to activate'
sleep 1

# Wait for app initialization and rendering
echo "Waiting for app initialization..."
sleep 4
screencapture "$SCREENSHOT_DIR/01-startup.png"
echo "✓ Captured startup screenshot"

# Check for SDL error dialogs
echo "Checking for error dialogs..."

# First, list all running processes and their windows for debugging
echo "=== Debug: All running processes and windows ==="
DEBUG_OUTPUT=$(osascript <<'EOFDBG'
tell application "System Events"
    set output to ""
    set allProcesses to every process
    repeat with proc in allProcesses
        try
            set procName to name of proc
            set procWindows to every window of proc
            if (count of procWindows) > 0 then
                set output to output & "Process: " & procName & return
                repeat with win in procWindows
                    try
                        set winName to name of win
                        set output to output & "  Window: " & winName & return
                    end try
                end repeat
            end if
        end try
    end repeat
    return output
end tell
EOFDBG
)
echo "$DEBUG_OUTPUT"
echo "=== End debug output ==="

# Check specifically for processes matching "SDL Error"
echo "Checking for processes containing 'SDL Error'..."
MATCHING_PROCESSES=$(osascript <<'EOFCHECK'
tell application "System Events"
    set output to ""
    set allProcesses to every process
    repeat with proc in allProcesses
        try
            set procName to name of proc
            if procName contains "SDL Error" then
                set output to output & "Matched process: " & procName & return
                set procWindows to every window of proc
                if (count of procWindows) > 0 then
                    repeat with win in procWindows
                        try
                            set winName to name of win
                            set output to output & "  Window name: " & winName & return
                        end try
                    end repeat
                else
                    set output to output & "  (no windows)" & return
                end if
            end if
        end try
    end repeat
    return output
end tell
EOFCHECK
)
if [ -n "$MATCHING_PROCESSES" ]; then
    echo "$MATCHING_PROCESSES"
else
    echo "No processes containing 'SDL Error' found"
fi

ERROR_DIALOG=$(osascript -e 'tell application "System Events" to get name of every window of every process whose name contains "SDL Error"' 2>/dev/null || true)
if [ -n "$ERROR_DIALOG" ]; then
  echo "✗ Found SDL Error dialog!"
  echo "Dialog details: $ERROR_DIALOG"
  screencapture "$SCREENSHOT_DIR/error-dialog.png"
  echo "✓ Captured error dialog screenshot"
  kill $APP_PID || true
  exit 1
fi
echo "✓ No error dialogs detected"

# Test pattern cycling with space key
for i in {1..5}; do
  echo "Test $i: Triggering next pattern..."
  osascript -e 'tell application "System Events" to tell process "Acid Warp" to keystroke space'
  sleep 2
  screencapture "$SCREENSHOT_DIR/02-pattern-$i.png"
  echo "✓ Captured pattern $i screenshot"
done

# Test mouse click - use AppleScript to click center of Acid Warp window
echo "Testing mouse click..."
osascript <<'EOF'
tell application "System Events"
    try
        tell process "Acid Warp"
            set frontWindow to window 1
            set {x, y, width, height} to (get position of frontWindow) & (get size of frontWindow)
            set centerX to x + (width / 2)
            set centerY to y + (height / 2)
            click at {centerX, centerY}
        end tell
    end try
end tell
EOF
sleep 1
screencapture "$SCREENSHOT_DIR/03-after-click.png"
echo "✓ Captured post-click screenshot"

# Test palette change
echo "Testing palette change (p key)..."
osascript -e 'tell application "System Events" to tell process "Acid Warp" to keystroke "p"'
sleep 1
screencapture "$SCREENSHOT_DIR/04-palette-change.png"
echo "✓ Captured palette change screenshot"

# Capture final state
sleep 2
screencapture "$SCREENSHOT_DIR/05-final.png"
echo "✓ Captured final screenshot"

# Quit gracefully
echo "Sending quit signal (Escape key)..."
osascript -e 'tell application "System Events" to tell process "Acid Warp" to key code 53' # Escape key code

# Wait for app to exit
sleep 1
if ps -p $APP_PID > /dev/null 2>&1; then
    # Force quit if still running using AppleScript first, then kill as fallback
    echo "App still running, trying to quit via AppleScript..."
    osascript -e 'tell application "Acid Warp" to quit' 2>/dev/null || true
    sleep 1
    if ps -p $APP_PID > /dev/null 2>&1; then
        echo "Force killing app..."
        kill $APP_PID 2>/dev/null || true
        sleep 1
    fi
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
