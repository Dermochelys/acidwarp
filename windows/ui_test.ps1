# PowerShell UI test script for Windows
$ErrorActionPreference = "Stop"

$SCREENSHOT_DIR = "screenshots"
$APP_BINARY = "build/acidwarp-windows.exe"

# Create screenshot directory
New-Item -ItemType Directory -Force -Path $SCREENSHOT_DIR | Out-Null

Write-Host "Launching Acid Warp..."
# Launch the app in background
$appProcess = Start-Process -FilePath $APP_BINARY -PassThru -RedirectStandardOutput "build/acidwarp.log" -RedirectStandardError "build/acidwarp-error.log" -NoNewWindow

Write-Host "App launched with PID: $($appProcess.Id)"

# Wait for app initialization
Write-Host "Waiting for app initialization..."
Start-Sleep -Seconds 5

# Check if log file exists and show recent output
if (Test-Path "build/acidwarp.log") {
    Write-Host "=== Recent app output ==="
    Get-Content "build/acidwarp.log" -Tail 50 -ErrorAction SilentlyContinue
    Write-Host "=== End of app output ==="
} else {
    Write-Host "⚠ Log file not found at build/acidwarp.log"
}

# Verify the app process is still running
Write-Host "Checking if Acid Warp process is running..."
Write-Host "DEBUG: Original PID from PowerShell: $($appProcess.Id)"

# Check for process by name
Write-Host "DEBUG: Checking for acidwarp-windows.exe process..."
$processes = Get-Process -Name "acidwarp-windows" -ErrorAction SilentlyContinue

$PROCESS_FOUND = $false
$actualPid = $null

if ($processes) {
    Write-Host "Process found by name: running"
    foreach ($proc in $processes) {
        Write-Host "  Found PID: $($proc.Id), Name: $($proc.ProcessName)"
    }
    $actualPid = $processes[0].Id
    $PROCESS_FOUND = $true
} else {
    Write-Host "Process not found by name, checking by PID..."
    $process = Get-Process -Id $appProcess.Id -ErrorAction SilentlyContinue
    if ($process) {
        Write-Host "Process found by PID: running"
        $actualPid = $process.Id
        $PROCESS_FOUND = $true
    } else {
        Write-Host "Process not found by PID either"
    }
}

# Also check if a window exists (alternative verification)
Write-Host "DEBUG: Checking for Acid Warp window..."
Add-Type -AssemblyName System.Windows.Forms
$windowProcesses = Get-Process | Where-Object { $_.ProcessName -like "*acidwarp*" -or $_.MainWindowTitle -like "*Acid Warp*" }

if ($windowProcesses) {
    Write-Host "Window found:"
    foreach ($proc in $windowProcesses) {
        Write-Host "  Process: $($proc.ProcessName), PID: $($proc.Id), Window: $($proc.MainWindowTitle)"
    }
    if (-not $PROCESS_FOUND) {
        $actualPid = $windowProcesses[0].Id
        $PROCESS_FOUND = $true
        Write-Host "DEBUG: Process verified via window check"
    }
}

if (-not $PROCESS_FOUND) {
    Write-Host "✗ Acid Warp process is not running!"
    Write-Host "The process may have crashed during initialization."
    Write-Host ""
    Write-Host "=== Full app log (if available) ==="
    if (Test-Path "build/acidwarp.log") {
        Get-Content "build/acidwarp.log" -ErrorAction SilentlyContinue
    } else {
        Write-Host "Log file not found"
    }
    Write-Host "=== End of app log ==="
    Write-Host ""
    Write-Host "Capturing screenshot to check if app window is visible..."

    # Take a screenshot even if process detection failed
    Add-Type -AssemblyName System.Drawing
    $screens = [System.Windows.Forms.Screen]::AllScreens
    $bounds = $screens[0].Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $bitmap.Save("$SCREENSHOT_DIR/00-process-not-detected.png")
    $graphics.Dispose()
    $bitmap.Dispose()

    Write-Host "Screenshot saved to $SCREENSHOT_DIR/00-process-not-detected.png"
    exit 1
}

Write-Host "✓ Acid Warp process is running"
if ($actualPid) {
    Write-Host "DEBUG: Using actual PID: $actualPid"
    $appProcess = Get-Process -Id $actualPid
}

# Helper function to take screenshot
function Take-Screenshot {
    param (
        [string]$name
    )

    Add-Type -AssemblyName System.Windows.Forms, System.Drawing
    $screens = [System.Windows.Forms.Screen]::AllScreens
    $bounds = $screens[0].Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $bitmap.Save("$SCREENSHOT_DIR/$name.png")
    $graphics.Dispose()
    $bitmap.Dispose()
}

# Helper function to send key
function Send-Key {
    param (
        [string]$key
    )

    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($key)
}

# Capture startup screenshot
Take-Screenshot "01-startup"
Write-Host "✓ Captured startup screenshot"

# Check for error dialogs - look for SDL Error window
Write-Host "Checking for error dialogs..."
Write-Host "DEBUG: Listing all windows with titles at $(Get-Date -Format 'HH:mm:ss')..."

Write-Host "=== All processes with window titles ==="
$allProcesses = Get-Process | Where-Object { $_.MainWindowTitle -ne '' }
foreach ($proc in $allProcesses) {
    Write-Host "Process: $($proc.ProcessName) (PID: $($proc.Id))"
    Write-Host "  Window Title: $($proc.MainWindowTitle)"
}
Write-Host "=== End of window list ==="
Write-Host ""
Write-Host "Checking for SDL Error windows..."
$errorWindows = Get-Process | Where-Object { $_.MainWindowTitle -like '*SDL Error*' }
if ($errorWindows) {
    Write-Host "ERROR: Found SDL Error windows:"
    foreach ($win in $errorWindows) {
        Write-Host "  Process: $($win.ProcessName), Title: $($win.MainWindowTitle)"
    }
    Stop-Process -Id $appProcess.Id -Force -ErrorAction SilentlyContinue
    exit 1
}

Write-Host "✓ No error dialogs detected"

# Test pattern cycling with n key
# Fade transition time: (63 fade steps * 30ms timer interval) * 2 (fade-out + fade-in) = 3780ms ≈ 3.8s
for ($i = 1; $i -le 5; $i++) {
    Write-Host "Test ${i}: Triggering next pattern..."
    Send-Key "n"
    Start-Sleep -Seconds 4  # Wait for fade-out + fade-in to complete
    Take-Screenshot "02-pattern-$i"
    Write-Host "✓ Captured pattern $i screenshot"
}

# Test mouse click - click center of screen
Write-Host "Testing mouse click..."
$screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$x = $screen.Width / 2
$y = $screen.Height / 2
[System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point($x, $y)

Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern void mouse_event(int flags, int dx, int dy, int cButtons, int info);' -Name U32 -Namespace W
[W.U32]::mouse_event(0x02, 0, 0, 0, 0)  # MOUSEEVENTF_LEFTDOWN
[W.U32]::mouse_event(0x04, 0, 0, 0, 0)  # MOUSEEVENTF_LEFTUP

Start-Sleep -Seconds 1
Take-Screenshot "03-after-click"
Write-Host "✓ Captured post-click screenshot"

# Test palette change
Write-Host "Testing palette change (p key)..."
Send-Key "p"
Start-Sleep -Seconds 1
Take-Screenshot "04-palette-change"
Write-Host "✓ Captured palette change screenshot"

# Capture final state
Start-Sleep -Seconds 2
Take-Screenshot "05-final"
Write-Host "✓ Captured final screenshot"

# Quit gracefully
Write-Host "Sending quit signal (q key)..."
Send-Key "q"

# Wait for app to exit
Write-Host "Waiting for app to exit..."
$appProcess.WaitForExit(5000) | Out-Null

# Force kill if still running
if (-not $appProcess.HasExited) {
    Write-Host "App did not exit gracefully, forcing termination..."
    Stop-Process -Id $appProcess.Id -Force -ErrorAction SilentlyContinue
}

# Verify screenshots were created
$screenshotFiles = Get-ChildItem -Path $SCREENSHOT_DIR -Filter "*.png" -ErrorAction SilentlyContinue
$SCREENSHOT_COUNT = if ($screenshotFiles) { $screenshotFiles.Count } else { 0 }

if ($SCREENSHOT_COUNT -ge 9) {
    Write-Host ""
    Write-Host "================================"
    Write-Host "✓ UI test passed successfully!"
    Write-Host "================================"
    Write-Host "Screenshots captured: $SCREENSHOT_COUNT"
    exit 0
} else {
    Write-Host ""
    Write-Host "================================"
    Write-Host "✗ UI test failed!"
    Write-Host "================================"
    Write-Host "Expected 9 screenshots, got $SCREENSHOT_COUNT"
    exit 1
}
