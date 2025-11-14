# PowerShell UI test script for Windows
$ErrorActionPreference = "Stop"

$SCREENSHOT_DIR = "screenshots"

# GPU Detection and Mesa3d Installation
Write-Host "=== GPU Detection Phase ==="
Write-Host "Checking for hardware GPU..."
$videoControllers = Get-WmiObject Win32_VideoController
$videoControllers | Select-Object Name, DriverVersion, DriverDate, Status | Format-Table -AutoSize

# Check for any hardware GPU (NVIDIA, AMD, Intel)
$hardwareGpu = $videoControllers | Where-Object {
  $_.Name -like "*NVIDIA*" -or
  $_.Name -like "*GeForce*" -or
  $_.Name -like "*Tesla*" -or
  $_.Name -like "*AMD*" -or
  $_.Name -like "*Radeon*" -or
  $_.Name -like "*Intel*HD*" -or
  $_.Name -like "*Intel*UHD*" -or
  $_.Name -like "*Intel*Iris*"
}

$USING_MESA = $false

if ($hardwareGpu) {
  Write-Host "`n[SUCCESS] Hardware GPU detected: $($hardwareGpu.Name)"
  Write-Host "Driver version: $($hardwareGpu.DriverVersion)"
  Write-Host "Will use hardware acceleration"
} else {
  Write-Host "`n[INFO] No hardware GPU detected"
  Write-Host "Available adapters:"
  $videoControllers | Format-Table Name -AutoSize
  Write-Host "Will install Mesa3d for software rendering"

  Write-Host "`n=== Mesa3d Installation Phase ==="

  # Download Mesa3d
  $mesaUrl = "https://github.com/pal1000/mesa-dist-win/releases/download/24.3.1/mesa3d-24.3.1-release-msvc.7z"
  $mesaArchive = "$env:TEMP\mesa3d.7z"
  $mesaExtractDir = "$env:TEMP\mesa3d"

  Write-Host "Downloading Mesa3d from $mesaUrl..."
  try {
    Invoke-WebRequest -Uri $mesaUrl -OutFile $mesaArchive -UseBasicParsing -TimeoutSec 300
    Write-Host "Download completed: $(([System.IO.FileInfo]$mesaArchive).Length / 1MB) MB"
  } catch {
    Write-Host "[ERROR] Failed to download Mesa3d: $_"
    exit 1
  }

  # Extract Mesa3d (requires 7-Zip)
  Write-Host "Extracting Mesa3d..."
  if (Test-Path "C:\Program Files\7-Zip\7z.exe") {
    & "C:\Program Files\7-Zip\7z.exe" x $mesaArchive -o"$mesaExtractDir" -y
  } else {
    Write-Host "[ERROR] 7-Zip not found. Installing 7-Zip..."
    choco install 7zip -y
    & "C:\Program Files\7-Zip\7z.exe" x $mesaArchive -o"$mesaExtractDir" -y
  }

  # Copy Mesa3d OpenGL DLLs to application directory
  Write-Host "Installing Mesa3d OpenGL drivers..."

  # Determine if we're in CI or local mode to find the right directory
  if (Test-Path "acidwarp-windows.exe") {
    $appDir = Get-Location
  } elseif (Test-Path "build/acidwarp-windows.exe") {
    $appDir = Join-Path (Get-Location) "build"
  } else {
    Write-Host "[ERROR] Could not find acidwarp-windows.exe"
    exit 1
  }

  # Copy required Mesa DLLs for desktop OpenGL software rendering
  # - opengl32.dll: WGL runtime loader for desktop OpenGL
  # - libgallium_wgl.dll: Gallium OpenGL megadriver (contains llvmpipe)
  Copy-Item "$mesaExtractDir\x64\opengl32.dll" "$appDir\opengl32.dll" -Force
  Copy-Item "$mesaExtractDir\x64\libgallium_wgl.dll" "$appDir\libgallium_wgl.dll" -Force

  Write-Host "[SUCCESS] Mesa3d installed for software rendering"
  $USING_MESA = $true

  # Set environment variables to use Mesa llvmpipe software renderer
  $env:GALLIUM_DRIVER = "llvmpipe"
  $env:MESA_GL_VERSION_OVERRIDE = "4.1COMPAT"
  $env:MESA_GLSL_VERSION_OVERRIDE = "410"

  Write-Host "Mesa environment configured:"
  Write-Host "  GALLIUM_DRIVER=llvmpipe (software rendering)"
  Write-Host "  MESA_GL_VERSION_OVERRIDE=4.1COMPAT"
  Write-Host "  MESA_GLSL_VERSION_OVERRIDE=410"
}

Write-Host ""

# Detect environment: CI (flat structure) vs Local (build/ subdirectory)
if (Test-Path "acidwarp-windows.exe") {
    # CI mode: all files in current directory
    $APP_BINARY = Join-Path (Get-Location) "acidwarp-windows.exe"
    $LOG_DIR = Get-Location
    Write-Host "Running in CI mode (flat directory structure)"
} elseif (Test-Path "build/acidwarp-windows.exe") {
    # Local mode: exe in build/ subdirectory
    $APP_BINARY = Join-Path (Get-Location) "build/acidwarp-windows.exe"
    $LOG_DIR = Join-Path (Get-Location) "build"
    Write-Host "Running in local mode (build/ subdirectory)"

    # Copy required DLLs to build directory if needed
    $dlls = @("libpng16-16.dll", "zlib1.dll")
    foreach ($dll in $dlls) {
        if (Test-Path $dll) {
            $destPath = "build/$dll"
            if (-not (Test-Path $destPath)) {
                Copy-Item $dll $destPath
                Write-Host "Copied $dll to build directory"
            }
        }
    }
} else {
    Write-Host "ERROR: Could not find acidwarp-windows.exe in current directory or build/ subdirectory"
    exit 1
}

# Create screenshot directory
$SCREENSHOT_DIR_FULL = Join-Path (Get-Location) $SCREENSHOT_DIR
New-Item -ItemType Directory -Force -Path $SCREENSHOT_DIR_FULL | Out-Null

Write-Host "`n=== Environment Cleanup ==="
# Close any distracting windows that might interfere
Write-Host "Closing Visual Studio Code windows if open..."
$vscodeProcesses = Get-Process -Name "Code" -ErrorAction SilentlyContinue
if ($vscodeProcesses) {
  foreach ($proc in $vscodeProcesses) {
    Write-Host "  Closing VS Code process (PID: $($proc.Id))"
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
  }
  Start-Sleep -Seconds 2
  Write-Host "[OK] Closed Visual Studio Code"
} else {
  Write-Host "[INFO] No Visual Studio Code windows found"
}

# Give the system time to settle after GPU initialization and cleanup
Write-Host "`nWaiting 10 seconds for environment to settle..."
Start-Sleep -Seconds 10
Write-Host "[OK] Environment ready"

Write-Host "`nLaunching Acid Warp..."
# Launch the app in background
$LOG_STDOUT = Join-Path $LOG_DIR "acidwarp.log"
$LOG_STDERR = Join-Path $LOG_DIR "acidwarp-error.log"
Write-Host "Log files will be written to:"
Write-Host "  stdout: $LOG_STDOUT"
Write-Host "  stderr: $LOG_STDERR"
$appProcess = Start-Process -FilePath $APP_BINARY -PassThru -RedirectStandardOutput $LOG_STDOUT -RedirectStandardError $LOG_STDERR -NoNewWindow

Write-Host "App launched with PID: $($appProcess.Id)"

# Wait for app initialization
Write-Host "Waiting for app initialization..."
Start-Sleep -Seconds 5

# Check if log files exist and show recent output
if (Test-Path $LOG_STDERR) {
    Write-Host "=== Recent app stderr output ==="
    Get-Content $LOG_STDERR -Tail 50 -ErrorAction SilentlyContinue
    Write-Host "=== End of stderr output ==="
} else {
    Write-Host "[WARN] Error log file not found at $LOG_STDERR"
}

if (Test-Path $LOG_STDOUT) {
    Write-Host "=== Recent app stdout output ==="
    Get-Content $LOG_STDOUT -Tail 50 -ErrorAction SilentlyContinue
    Write-Host "=== End of stdout output ==="
} else {
    Write-Host "[WARN] Output log file not found at $LOG_STDOUT"
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

# Add Window detection helper
Add-Type @"
    using System;
    using System.Runtime.InteropServices;
    public class WindowHelper {
        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [DllImport("user32.dll")]
        public static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsWindowVisible(IntPtr hWnd);
    }
"@

$windowProcesses = Get-Process | Where-Object { $_.ProcessName -like "*acidwarp*" -or $_.MainWindowTitle -like "*Acid Warp*" }

if ($windowProcesses) {
    Write-Host "Window found:"
    foreach ($proc in $windowProcesses) {
        Write-Host "  Process: $($proc.ProcessName), PID: $($proc.Id), Window: $($proc.MainWindowTitle)"
        Write-Host "  Window Handle: $($proc.MainWindowHandle)"

        # Check if window is visible
        $isVisible = [WindowHelper]::IsWindowVisible($proc.MainWindowHandle)
        Write-Host "  Window is visible: $isVisible"

        # Check if window is in foreground
        $foregroundWindow = [WindowHelper]::GetForegroundWindow()
        $isForeground = ($proc.MainWindowHandle -eq $foregroundWindow)
        Write-Host "  Window is in foreground: $isForeground"
        Write-Host "  Current foreground window handle: $foregroundWindow"

        # Try to bring window to foreground
        Write-Host "  Attempting to bring window to foreground..."
        [WindowHelper]::ShowWindow($proc.MainWindowHandle, 9) | Out-Null  # SW_RESTORE = 9
        Start-Sleep -Milliseconds 100
        $bringToFrontResult = [WindowHelper]::SetForegroundWindow($proc.MainWindowHandle)
        Write-Host "  SetForegroundWindow result: $bringToFrontResult"

        Start-Sleep -Milliseconds 200

        # Check again after attempting to bring to foreground
        $foregroundWindowAfter = [WindowHelper]::GetForegroundWindow()
        $isForegroundAfter = ($proc.MainWindowHandle -eq $foregroundWindowAfter)
        Write-Host "  Window is in foreground after SetForegroundWindow: $isForegroundAfter"
        Write-Host "  Foreground window handle after: $foregroundWindowAfter"
    }
    if (-not $PROCESS_FOUND) {
        $actualPid = $windowProcesses[0].Id
        $PROCESS_FOUND = $true
        Write-Host "DEBUG: Process verified via window check"
    }
}

if (-not $PROCESS_FOUND) {
    Write-Host "[ERROR] Acid Warp process is not running!"
    Write-Host "The process may have crashed during initialization."
    Write-Host ""
    Write-Host "=== Full app stderr log (if available) ==="
    if (Test-Path $LOG_STDERR) {
        Get-Content $LOG_STDERR -ErrorAction SilentlyContinue
    } else {
        Write-Host "Error log file not found"
    }
    Write-Host "=== End of stderr log ==="
    Write-Host ""
    Write-Host "=== Full app stdout log (if available) ==="
    if (Test-Path $LOG_STDOUT) {
        Get-Content $LOG_STDOUT -ErrorAction SilentlyContinue
    } else {
        Write-Host "Output log file not found"
    }
    Write-Host "=== End of stdout log ==="
    Write-Host ""
    Write-Host "Capturing screenshot to check if app window is visible..."

    # Take a screenshot even if process detection failed
    Add-Type -AssemblyName System.Drawing
    $screens = [System.Windows.Forms.Screen]::AllScreens
    $bounds = $screens[0].Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $filepath = Join-Path (Get-Location) "$SCREENSHOT_DIR/00-process-not-detected.png"
    $bitmap.Save($filepath)
    $graphics.Dispose()
    $bitmap.Dispose()

    Write-Host "Screenshot saved to $SCREENSHOT_DIR/00-process-not-detected.png"
    exit 1
}

Write-Host "[OK] Acid Warp process is running"
if ($actualPid) {
    Write-Host "DEBUG: Using actual PID: $actualPid"
    $appProcess = Get-Process -Id $actualPid
}

# Helper function to check if process is still alive
function Test-ProcessAlive {
    param (
        [System.Diagnostics.Process]$process
    )

    $isAlive = -not $process.HasExited
    if (-not $isAlive) {
        Write-Host "[ERROR] Acid Warp process has exited unexpectedly!"
        Write-Host "Exit code: $($process.ExitCode)"
        Write-Host ""
        Write-Host "=== Full app stderr log ==="
        if (Test-Path $LOG_STDERR) {
            Get-Content $LOG_STDERR -ErrorAction SilentlyContinue
        }
        Write-Host "=== End of stderr log ==="
        Write-Host ""
        Write-Host "=== Full app stdout log ==="
        if (Test-Path $LOG_STDOUT) {
            Get-Content $LOG_STDOUT -ErrorAction SilentlyContinue
        }
        Write-Host "=== End of stdout log ==="
        exit 1
    }
    return $isAlive
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
    $filepath = Join-Path (Get-Location) "$SCREENSHOT_DIR/$name.png"
    $bitmap.Save($filepath)
    $graphics.Dispose()
    $bitmap.Dispose()
}

# Helper function to send key
function Send-Key {
    param (
        [string]$key,
        [System.Diagnostics.Process]$process
    )

    # Bring window to foreground (WindowHelper already defined above)
    if ($process.MainWindowHandle -ne [IntPtr]::Zero) {
        Write-Host "  Bringing window to foreground before sending key '$key'..."
        $isVisible = [WindowHelper]::IsWindowVisible($process.MainWindowHandle)
        Write-Host "  Window visible before key send: $isVisible"

        # Show and activate window (SW_RESTORE = 9)
        [WindowHelper]::ShowWindow($process.MainWindowHandle, 9) | Out-Null
        Start-Sleep -Milliseconds 100

        $result = [WindowHelper]::SetForegroundWindow($process.MainWindowHandle)
        Write-Host "  SetForegroundWindow result: $result"

        Start-Sleep -Milliseconds 100

        # Verify it's in foreground
        $foregroundWindow = [WindowHelper]::GetForegroundWindow()
        $isForeground = ($process.MainWindowHandle -eq $foregroundWindow)
        Write-Host "  Window is in foreground: $isForeground"
    }

    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($key)
}

# Capture startup screenshot
Take-Screenshot "01-startup"
Write-Host "[OK] Captured startup screenshot"

# Check process is still alive
Test-ProcessAlive $appProcess | Out-Null

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

Write-Host "[OK] No error dialogs detected"

# Test pattern cycling with n key
# Fade transition time: (63 fade steps * 30ms timer interval) * 2 (fade-out + fade-in) = 3780ms ~= 3.8s
for ($i = 1; $i -le 5; $i++) {
    Write-Host "Test $i : Triggering next pattern..."
    Send-Key "n" $appProcess
    Start-Sleep -Seconds 4  # Wait for fade-out + fade-in to complete
    Take-Screenshot "02-pattern-$i"
    Write-Host "[OK] Captured pattern $i screenshot"

    # Check process is still alive
    Test-ProcessAlive $appProcess | Out-Null
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
Write-Host "[OK] Captured post-click screenshot"

# Check process is still alive
Test-ProcessAlive $appProcess | Out-Null

# Test palette change
Write-Host "Testing palette change (p key)..."
Send-Key "p" $appProcess
Start-Sleep -Seconds 1
Take-Screenshot "04-palette-change"
Write-Host "[OK] Captured palette change screenshot"

# Check process is still alive
Test-ProcessAlive $appProcess | Out-Null

# Capture final state
Start-Sleep -Seconds 2
Take-Screenshot "05-final"
Write-Host "[OK] Captured final screenshot"

# Final process check
Test-ProcessAlive $appProcess | Out-Null

# Quit gracefully
Write-Host "Sending quit signal (q key)..."
Send-Key "q" $appProcess

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
    Write-Host "[PASS] UI test passed successfully!"
    Write-Host "================================"
    Write-Host "Screenshots captured: $SCREENSHOT_COUNT"
    exit 0
} else {
    Write-Host ""
    Write-Host "================================"
    Write-Host "[FAIL] UI test failed!"
    Write-Host "================================"
    Write-Host "Expected 9 screenshots, got $SCREENSHOT_COUNT"
    exit 1
}
