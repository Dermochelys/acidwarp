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
echo "DEBUG: Checking PID=$APP_PID"

# First, check if bash still knows about the process
if ps -p $APP_PID > /dev/null 2>&1; then
  echo "DEBUG: Bash reports process $APP_PID exists"
else
  echo "DEBUG: Bash reports process $APP_PID does NOT exist"
fi

# List all acidwarp processes
echo "DEBUG: Looking for acidwarp processes..."
ps aux | grep -i acidwarp | grep -v grep || echo "DEBUG: No acidwarp processes found by ps"

# Now check with PowerShell
echo "DEBUG: Checking with PowerShell..."
echo "DEBUG: About to run PowerShell command at $(date '+%H:%M:%S')"

# Use timeout to prevent hanging (10 second timeout)
timeout 10s powershell.exe -Command "
Write-Output 'PowerShell starting process check for PID $APP_PID'
try {
    \$process = Get-Process -Id $APP_PID -ErrorAction SilentlyContinue
    if (\$process) {
        Write-Output 'Process found: running'
        Write-Output 'running'
    } else {
        Write-Output 'Process not found'
    }
} catch {
    Write-Output 'Error checking process'
    Write-Output \$_.Exception.Message
}
Write-Output 'PowerShell command completed'
" > /tmp/ps_output.txt 2>&1

PS_EXIT_CODE=$?
echo "DEBUG: PowerShell command finished at $(date '+%H:%M:%S') with exit code $PS_EXIT_CODE"

if [ -f /tmp/ps_output.txt ]; then
    echo "DEBUG: PowerShell output file exists, size: $(stat -c%s /tmp/ps_output.txt 2>/dev/null || stat -f%z /tmp/ps_output.txt 2>/dev/null || echo 'unknown') bytes"
    PROCESS_RUNNING=$(cat /tmp/ps_output.txt)
else
    echo "DEBUG: PowerShell output file does not exist!"
    PROCESS_RUNNING=""
fi

echo "DEBUG: PowerShell output:"
echo "$PROCESS_RUNNING"
echo "DEBUG: End of PowerShell output"

if echo "$PROCESS_RUNNING" | grep -q "running"; then
  echo "✓ Acid Warp process is running"
else
  echo "✗ Acid Warp process ($APP_PID) is not running!"
  echo "The process may have crashed during initialization."
  exit 1
fi

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
echo "DEBUG: Listing all windows with titles at $(date '+%H:%M:%S')..."

timeout 10s powershell.exe -Command "
Add-Type -AssemblyName System.Windows.Forms
Write-Output '=== All processes with window titles ==='
\$allProcesses = Get-Process | Where-Object { \$_.MainWindowTitle -ne '' }
foreach (\$proc in \$allProcesses) {
    Write-Output ('Process: ' + \$proc.ProcessName + ' (PID: ' + \$proc.Id + ')')
    Write-Output ('  Window Title: ' + \$proc.MainWindowTitle)
}
Write-Output '=== End of window list ==='
Write-Output ''
Write-Output 'Checking for SDL Error windows...'
\$errorWindows = Get-Process | Where-Object { \$_.MainWindowTitle -like '*SDL Error*' }
if (\$errorWindows) {
    Write-Output 'ERROR: Found SDL Error windows:'
    foreach (\$win in \$errorWindows) {
        Write-Output ('  Process: ' + \$win.ProcessName + ', Title: ' + \$win.MainWindowTitle)
    }
    Write-Output 'found'
} else {
    Write-Output 'No SDL Error windows found'
}
" > /tmp/error_check.txt 2>&1

ERROR_CHECK_EXIT=$?
echo "DEBUG: Error check command finished at $(date '+%H:%M:%S') with exit code $ERROR_CHECK_EXIT"

if [ -f /tmp/error_check.txt ]; then
    ERROR_CHECK_OUTPUT=$(cat /tmp/error_check.txt)
else
    ERROR_CHECK_OUTPUT=""
fi

echo "$ERROR_CHECK_OUTPUT"

if echo "$ERROR_CHECK_OUTPUT" | grep -q "^found"; then
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
