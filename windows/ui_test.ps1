# PowerShell UI test script for Windows
$ErrorActionPreference = "Stop"

$SCREENSHOT_DIR = "screenshots"

# GPU Detection and Driver Installation
Write-Host "=== GPU Detection Phase ==="
Write-Host "Checking all display adapters..."
$videoControllers = Get-WmiObject Win32_VideoController
$videoControllers | Select-Object Name, DriverVersion, DriverDate, Status | Format-Table -AutoSize

Write-Host "`nChecking PCI devices for GPUs..."
$pciDevices = Get-PnpDevice -Class Display
$pciDevices | Format-Table Name, Status, InstanceId -AutoSize

Write-Host "`nChecking for hidden/disabled devices..."
$allDisplayDevices = Get-PnpDevice -Class Display -Status ERROR,UNKNOWN -ErrorAction SilentlyContinue
if ($allDisplayDevices) {
  Write-Host "Found disabled/error devices:"
  $allDisplayDevices | Format-Table Name, Status -AutoSize
} else {
  Write-Host "No disabled/error display devices found"
}

# Check if NVIDIA GPU exists in PCI devices but not activated
$nvidiaInPci = $pciDevices | Where-Object { $_.Name -like "*NVIDIA*" -or $_.Name -like "*Tesla*" -or $_.Name -like "*GeForce*" }
$nvidiaInWmi = $videoControllers | Where-Object { $_.Name -like "*NVIDIA*" -or $_.Name -like "*Tesla*" -or $_.Name -like "*GeForce*" }

if ($nvidiaInPci -and -not $nvidiaInWmi) {
  Write-Host "`n[INFO] NVIDIA GPU found in PCI but not active in video controllers"
  Write-Host "GPU is present but drivers may not be loaded"
} elseif ($nvidiaInWmi) {
  Write-Host "`n[SUCCESS] NVIDIA GPU is active: $($nvidiaInWmi.Name)"
  Write-Host "Driver version: $($nvidiaInWmi.DriverVersion)"
  Write-Host "No driver installation needed"
} else {
  Write-Host "`n[ERROR] No NVIDIA GPU detected"
  Write-Host "This runner does not have a suitable GPU for UI testing"
  Write-Host "Available adapters:"
  $videoControllers | Format-Table Name -AutoSize
  exit 1
}

Write-Host "`n=== Driver Installation Phase ==="
if ($nvidiaInPci -and -not $nvidiaInWmi) {
  Write-Host "Installing NVIDIA drivers for detected GPU..."
  
  # Use latest Game Ready driver (more reliable than GRID)
  $nvidiaUrl = "https://us.download.nvidia.com/Windows/566.03/566.03-desktop-win10-win11-64bit-international-dch-whql.exe"
  $installerPath = "$env:TEMP\nvidia-driver.exe"
  
  Write-Host "Downloading NVIDIA driver (this may take a few minutes)..."
  try {
    Invoke-WebRequest -Uri $nvidiaUrl -OutFile $installerPath -UseBasicParsing -TimeoutSec 300
    Write-Host "Download completed: $(([System.IO.FileInfo]$installerPath).Length / 1MB) MB"
  } catch {
    Write-Host "[ERROR] Failed to download driver: $_"
    exit 1
  }
  
  Write-Host "Installing NVIDIA driver (this may take 5-10 minutes)..."
  $process = Start-Process -FilePath $installerPath -ArgumentList "/s", "/noreboot", "/noeula" -Wait -NoNewWindow -PassThru
  
  if ($process.ExitCode -eq 0) {
    Write-Host "[SUCCESS] NVIDIA driver installation completed"
  } else {
    Write-Host "[WARNING] Driver installer exit code: $($process.ExitCode)"
  }
  
  # Force driver service restart
  Write-Host "Restarting display driver service..."
  Get-Service -Name "nvlddmkm" -ErrorAction SilentlyContinue | Restart-Service -Force -ErrorAction SilentlyContinue
  
  Start-Sleep -Seconds 5
}

Write-Host "`n=== Post-Installation Verification ==="
Get-WmiObject Win32_VideoController | Select-Object Name, DriverVersion, Status | Format-Table -AutoSize

# Check if nvidia-smi is available
Write-Host "`nChecking nvidia-smi..."
$nvidiaSmi = Get-Command nvidia-smi -ErrorAction SilentlyContinue
if ($nvidiaSmi) {
  Write-Host "Running nvidia-smi:"
  & nvidia-smi
} else {
  Write-Host "nvidia-smi not found in PATH"
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

Write-Host "Launching Acid Warp..."
# Launch the app in background
$appProcess = Start-Process -FilePath $APP_BINARY -PassThru -RedirectStandardOutput "$LOG_DIR/acidwarp.log" -RedirectStandardError "$LOG_DIR/acidwarp-error.log" -NoNewWindow

Write-Host "App launched with PID: $($appProcess.Id)"

# Wait for app initialization
Write-Host "Waiting for app initialization..."
Start-Sleep -Seconds 5

# Check if log file exists and show recent output
if (Test-Path "$LOG_DIR/acidwarp.log") {
    Write-Host "=== Recent app output ==="
    Get-Content "$LOG_DIR/acidwarp.log" -Tail 50 -ErrorAction SilentlyContinue
    Write-Host "=== End of app output ==="
} else {
    Write-Host "[WARN] Log file not found at $LOG_DIR/acidwarp.log"
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
    Write-Host "[ERROR] Acid Warp process is not running!"
    Write-Host "The process may have crashed during initialization."
    Write-Host ""
    Write-Host "=== Full app log (if available) ==="
    if (Test-Path "$LOG_DIR/acidwarp.log") {
        Get-Content "$LOG_DIR/acidwarp.log" -ErrorAction SilentlyContinue
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

    # Bring window to foreground
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
        }
"@

    if ($process.MainWindowHandle -ne [IntPtr]::Zero) {
        # Show and activate window (SW_RESTORE = 9)
        [WindowHelper]::ShowWindow($process.MainWindowHandle, 9) | Out-Null
        Start-Sleep -Milliseconds 100
        [WindowHelper]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
        Start-Sleep -Milliseconds 100
    }

    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($key)
}

# Capture startup screenshot
Take-Screenshot "01-startup"
Write-Host "[OK] Captured startup screenshot"

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

# Test palette change
Write-Host "Testing palette change (p key)..."
Send-Key "p" $appProcess
Start-Sleep -Seconds 1
Take-Screenshot "04-palette-change"
Write-Host "[OK] Captured palette change screenshot"

# Capture final state
Start-Sleep -Seconds 2
Take-Screenshot "05-final"
Write-Host "[OK] Captured final screenshot"

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
