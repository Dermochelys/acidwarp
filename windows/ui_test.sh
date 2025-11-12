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
# Launch the app in background with output redirection to capture logs
"$APP_BINARY" > build/acidwarp.log 2>&1 &
APP_PID=$!

echo "App launched with PID: $APP_PID"

# Wait for app initialization
echo "Waiting for app initialization..."
sleep 5

# Check if log file exists and show recent output
if [ -f build/acidwarp.log ]; then
    echo "=== Recent app output ==="
    tail -50 build/acidwarp.log || echo "Log file exists but empty or unreadable"
    echo "=== End of app output ==="
else
    echo "⚠ Log file not found at build/acidwarp.log"
fi

# Verify the app process is still running
echo "Checking if Acid Warp process is running..."
echo "DEBUG: Original PID from bash: $APP_PID"

# Check for process by name instead of PID (more reliable in MSYS2)
# PowerShell check for process by executable name
echo "DEBUG: Checking for acidwarp-windows.exe process..."
timeout 10s powershell.exe -Command "
Write-Output 'PowerShell starting process check by name'
try {
    \$processes = Get-Process -Name 'acidwarp-windows' -ErrorAction SilentlyContinue
    if (\$processes) {
        Write-Output 'Process found by name: running'
        foreach (\$proc in \$processes) {
            Write-Output ('  Found PID: ' + \$proc.Id + ', Name: ' + \$proc.ProcessName)
        }
        Write-Output ('FOUND_PID=' + (\$processes[0].Id))
        Write-Output 'running'
    } else {
        Write-Output 'Process not found by name'
        # Also try checking by PID as fallback
        \$process = Get-Process -Id $APP_PID -ErrorAction SilentlyContinue
        if (\$process) {
            Write-Output 'Process found by PID: running'
            Write-Output ('FOUND_PID=' + \$process.Id)
            Write-Output 'running'
        } else {
            Write-Output 'Process not found by PID either'
        }
    }
} catch {
    Write-Output 'Error checking process'
    Write-Output \$_.Exception.Message
}
Write-Output 'PowerShell command completed'
" > /tmp/ps_output.txt 2>&1

PS_EXIT_CODE=$?
echo "DEBUG: PowerShell command finished with exit code $PS_EXIT_CODE"

if [ -f /tmp/ps_output.txt ]; then
    PROCESS_RUNNING=$(cat /tmp/ps_output.txt)
    echo "DEBUG: PowerShell output:"
    echo "$PROCESS_RUNNING"
else
    PROCESS_RUNNING=""
    echo "DEBUG: PowerShell output file does not exist!"
fi

# Also check if a window exists (alternative verification)
echo "DEBUG: Checking for Acid Warp window..."
WINDOW_CHECK=$(timeout 10s powershell.exe -Command "
Add-Type -AssemblyName System.Windows.Forms
\$processes = Get-Process | Where-Object { \$_.ProcessName -like '*acidwarp*' -or \$_.MainWindowTitle -like '*Acid Warp*' }
if (\$processes) {
    Write-Output 'window-found'
    foreach (\$proc in \$processes) {
        Write-Output ('Process: ' + \$proc.ProcessName + ', PID: ' + \$proc.Id + ', Window: ' + \$proc.MainWindowTitle)
    }
    Write-Output ('FOUND_PID=' + (\$processes[0].Id))
} else {
    Write-Output 'no-window'
}
" 2>&1)

echo "DEBUG: Window check result:"
echo "$WINDOW_CHECK"

# Determine if process is running
PROCESS_FOUND=false
if echo "$PROCESS_RUNNING" | grep -q "running"; then
  PROCESS_FOUND=true
elif echo "$WINDOW_CHECK" | grep -q "window-found"; then
  PROCESS_FOUND=true
  echo "DEBUG: Process verified via window check"
fi

if [ "$PROCESS_FOUND" = true ]; then
  echo "✓ Acid Warp process is running"
  # Update APP_PID to the actual PID if we found it
  # Extract PID from FOUND_PID=1234 format
  ACTUAL_PID=$(echo "$PROCESS_RUNNING" "$WINDOW_CHECK" | grep "FOUND_PID=" | cut -d= -f2 | head -1)
  if [ -n "$ACTUAL_PID" ] && [ "$ACTUAL_PID" != "" ]; then
    APP_PID=$ACTUAL_PID
    echo "DEBUG: Using actual PID: $APP_PID"
  else
    echo "DEBUG: Could not extract PID, using original: $APP_PID"
  fi
else
  echo "✗ Acid Warp process is not running!"
  echo "The process may have crashed during initialization."
  echo ""
  echo "Process check output:"
  echo "$PROCESS_RUNNING"
  echo ""
  echo "Window check output:"
  echo "$WINDOW_CHECK"
  echo ""
  echo "=== Full app log (if available) ==="
  if [ -f build/acidwarp.log ]; then
    cat build/acidwarp.log || echo "Could not read log file"
  else
    echo "Log file not found"
  fi
  echo "=== End of app log ==="
  echo ""
  echo "Capturing screenshot to check if app window is visible..."
  # Take a screenshot even if process detection failed - app might be running
  powershell.exe -Command "
  Add-Type -AssemblyName System.Windows.Forms,System.Drawing
  \$screens = [System.Windows.Forms.Screen]::AllScreens
  \$bounds = \$screens[0].Bounds
  \$bitmap = New-Object System.Drawing.Bitmap \$bounds.Width, \$bounds.Height
  \$graphics = [System.Drawing.Graphics]::FromImage(\$bitmap)
  \$graphics.CopyFromScreen(\$bounds.Location, [System.Drawing.Point]::Empty, \$bounds.Size)
  \$bitmap.Save('$SCREENSHOT_DIR/00-process-not-detected.png')
  \$graphics.Dispose()
  \$bitmap.Dispose()
  " || echo "Failed to capture screenshot"
  echo "Screenshot saved to $SCREENSHOT_DIR/00-process-not-detected.png"
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

# Test pattern cycling with n key
# Fade transition time: (63 fade steps * 30ms timer interval) * 2 (fade-out + fade-in) = 3780ms ≈ 3.8s
# Note: Timer interval is 30ms (ROTATION_DELAY=30000/1000), not the 16ms event processing timeout
for i in {1..5}; do
  echo "Test $i: Triggering next pattern..."
  send_key "n"
  sleep 4  # Wait for fade-out + fade-in to complete
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
