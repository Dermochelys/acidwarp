#!/bin/bash
set -e

SCREENSHOT_DIR="screenshots"
APP_BINARY="build/acidwarp-windows.exe"

mkdir -p "$SCREENSHOT_DIR"

echo "Setting up Mesa software rendering..."
# Copy Mesa's OpenGL DLL to the executable directory so Windows uses it instead of system OpenGL 1.1
MESA_OPENGL="/mingw64/bin/opengl32.dll"
if [ -f "$MESA_OPENGL" ]; then
    cp "$MESA_OPENGL" build/
    echo "✓ Copied Mesa OpenGL DLL to build directory"
else
    echo "⚠ Mesa OpenGL DLL not found at $MESA_OPENGL"
    ls -la /mingw64/bin/opengl32.dll || echo "File does not exist"
fi

echo "Launching Acid Warp..."
# Launch the app in background
"$APP_BINARY" &
APP_PID=$!

echo "App launched with PID: $APP_PID"

# Wait for app initialization
echo "Waiting for app initialization..."
sleep 5

# Verify the app process is still running
echo "Checking if Acid Warp process is running..."
PROCESS_RUNNING=$(powershell.exe -Command "
\$process = Get-Process -Id $APP_PID -ErrorAction SilentlyContinue
if (\$process) { Write-Output 'running' }
")
if [ "$PROCESS_RUNNING" != "running" ]; then
  echo "✗ Acid Warp process ($APP_PID) is not running!"
  echo "The process may have crashed during initialization."
  exit 1
fi
echo "✓ Acid Warp process is running"

# Capture startup screenshot using PowerShell
powershell.exe -Command "
Add-Type -AssemblyName System.Windows.Forms,System.Drawing
\$screens = [System.Windows.Forms.Screen]::AllScreens
\$bounds = \$screens[0].Bounds
\$bitmap = New-Object System.Drawing.Bitmap \$bounds.Width, \$bounds.Height
\$graphics = [System.Drawing.Graphics]::FromImage(\$bitmap)
\$graphics.CopyFromScreen(\$bounds.Location, [System.Drawing.Point]::Empty, \$bounds.Size)
\$bitmap.Save('$SCREENSHOT_DIR/01-startup.png')
\$graphics.Dispose()
\$bitmap.Dispose()
"
echo "✓ Captured startup screenshot"

# Check for error dialogs - look for SDL Error window
echo "Checking for error dialogs..."
ERROR_WINDOW=$(powershell.exe -Command "
Add-Type -AssemblyName System.Windows.Forms
\$windows = Get-Process | Where-Object {$_.MainWindowTitle -like '*SDL Error*'}
if (\$windows) { Write-Output 'found' }
")
if [ "$ERROR_WINDOW" = "found" ]; then
  echo "✗ Found SDL Error dialog!"
  taskkill //PID $APP_PID //F 2>/dev/null || true
  exit 1
fi
echo "✓ No error dialogs detected"

# Helper function to take screenshot
take_screenshot() {
  local name=$1
  powershell.exe -Command "
  Add-Type -AssemblyName System.Windows.Forms,System.Drawing
  \$screens = [System.Windows.Forms.Screen]::AllScreens
  \$bounds = \$screens[0].Bounds
  \$bitmap = New-Object System.Drawing.Bitmap \$bounds.Width, \$bounds.Height
  \$graphics = [System.Drawing.Graphics]::FromImage(\$bitmap)
  \$graphics.CopyFromScreen(\$bounds.Location, [System.Drawing.Point]::Empty, \$bounds.Size)
  \$bitmap.Save('$SCREENSHOT_DIR/$name.png')
  \$graphics.Dispose()
  \$bitmap.Dispose()
  "
}

# Helper function to send key
send_key() {
  local key=$1
  powershell.exe -Command "
  Add-Type -AssemblyName System.Windows.Forms
  [System.Windows.Forms.SendKeys]::SendWait('$key')
  "
}

# Test pattern cycling with space key
for i in {1..5}; do
  echo "Test $i: Triggering next pattern..."
  send_key " "
  sleep 2
  take_screenshot "02-pattern-$i"
  echo "✓ Captured pattern $i screenshot"
done

# Test mouse click - click center of screen
echo "Testing mouse click..."
powershell.exe -Command "
Add-Type -AssemblyName System.Windows.Forms
\$screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
\$x = \$screen.Width / 2
\$y = \$screen.Height / 2
[System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point(\$x, \$y)
Add-Type -MemberDefinition '[DllImport(\"user32.dll\")] public static extern void mouse_event(int flags, int dx, int dy, int cButtons, int info);' -Name U32 -Namespace W
[W.U32]::mouse_event(0x02, 0, 0, 0, 0)  # MOUSEEVENTF_LEFTDOWN
[W.U32]::mouse_event(0x04, 0, 0, 0, 0)  # MOUSEEVENTF_LEFTUP
"
sleep 1
take_screenshot "03-after-click"
echo "✓ Captured post-click screenshot"

# Test palette change
echo "Testing palette change (p key)..."
send_key "p"
sleep 1
take_screenshot "04-palette-change"
echo "✓ Captured palette change screenshot"

# Capture final state
sleep 2
take_screenshot "05-final"
echo "✓ Captured final screenshot"

# Quit gracefully
echo "Sending quit signal (Escape key)..."
send_key "{ESC}"

# Wait for app to exit
sleep 1
taskkill //PID $APP_PID //F 2>/dev/null || true

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
