# Cleanup script for TinyDLP DLL locks
Write-Host "Cleaning up TinyDLP processes and DLL locks..." -ForegroundColor Yellow
Write-Host ""

# Check for running TinyDLP processes
$tinyDLPProcesses = Get-Process -Name "TinyDLP" -ErrorAction SilentlyContinue
if ($tinyDLPProcesses) {
    Write-Host "Found running TinyDLP.exe processes. Terminating..." -ForegroundColor Red
    $tinyDLPProcesses | Stop-Process -Force
    Start-Sleep -Seconds 2
}

# Check for processes with TinyDLP_Hook.dll loaded
Write-Host "Checking for processes with TinyDLP_Hook.dll loaded..." -ForegroundColor Yellow
$processesWithDLL = Get-Process | Where-Object {$_.Modules.ModuleName -like "*TinyDLP_Hook*"}

if ($processesWithDLL) {
    Write-Host "Found processes with TinyDLP_Hook.dll loaded:" -ForegroundColor Red
    $processesWithDLL | Select-Object ProcessName, Id | Format-Table -AutoSize
    
    Write-Host "Terminating processes to release DLL locks..." -ForegroundColor Yellow
    
    foreach ($process in $processesWithDLL) {
        try {
            Write-Host "Terminating process: $($process.ProcessName) (PID: $($process.Id))" -ForegroundColor Cyan
            Stop-Process -Id $process.Id -Force -ErrorAction Stop
        }
        catch {
            Write-Host "Failed to terminate process: $($process.ProcessName) (PID: $($process.Id))" -ForegroundColor Red
        }
    }
    
    # Wait for processes to fully terminate
    Start-Sleep -Seconds 3
    
    # Restart Explorer if it was terminated
    $explorerProcesses = Get-Process -Name "explorer" -ErrorAction SilentlyContinue
    if (-not $explorerProcesses) {
        Write-Host "Restarting Explorer..." -ForegroundColor Green
        Start-Process "explorer.exe"
    }
    
    # Note about Chrome
    $chromeProcesses = Get-Process -Name "chrome" -ErrorAction SilentlyContinue
    if (-not $chromeProcesses) {
        Write-Host "Chrome was terminated. It will restart automatically when needed." -ForegroundColor Green
    }
} else {
    Write-Host "No processes found with TinyDLP_Hook.dll loaded." -ForegroundColor Green
}

Write-Host ""
Write-Host "Cleanup completed. You can now build the project." -ForegroundColor Green
Write-Host ""
