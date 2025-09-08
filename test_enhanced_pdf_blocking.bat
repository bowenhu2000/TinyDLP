@echo off
echo ========================================
echo TinyDLP Enhanced PDF Copy Blocking Test
echo ========================================
echo.

echo Step 1: Build the hook DLL
call build_hook_dll.bat
if %ERRORLEVEL% NEQ 0 (
    echo Build failed! Please check the errors above.
    pause
    exit /b 1
)

echo.
echo Step 2: Clear previous logs
if exist "C:\TinyDLP\TinyDLP_Hook.log" del "C:\TinyDLP\TinyDLP_Hook.log"
echo Previous logs cleared.

echo.
echo Step 3: Start TinyDLP (run as Administrator)
echo Please run TinyDLP.exe as Administrator in another window
echo Wait for DLL injection to complete, then press any key...
pause

echo.
echo Step 4: Create test PDF file
echo test content > test.pdf
echo Test PDF created: test.pdf

echo.
echo Step 5: Test PDF Copy Blocking
echo 1. Make sure you have a USB drive connected (e.g., K:)
echo 2. Try to copy test.pdf to your USB drive using Windows Explorer
echo 3. Check the logs below for detailed information
echo.

echo Press any key after attempting the copy operation...
pause

echo.
echo ========================================
echo LOG ANALYSIS
echo ========================================
echo.

echo Checking for PDF file operations...
findstr /C:"test.pdf" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No test.pdf operations found in logs
) else (
    echo [FOUND] test.pdf operations detected!
)

echo.
echo Checking for USB drive operations...
findstr /C:"K:" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No K: drive operations found in logs
) else (
    echo [FOUND] K: drive operations detected!
)

echo.
echo Checking for CopyFileW calls...
findstr /C:"CopyFileW called" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No CopyFileW calls found - Windows is using different APIs
) else (
    echo [FOUND] CopyFileW calls detected!
)

echo.
echo Checking for blocking attempts...
findstr /C:"BLOCKED" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No blocking attempts found
) else (
    echo [FOUND] Blocking attempts detected!
)

echo.
echo ========================================
echo FULL LOG (Last 50 lines)
echo ========================================
powershell "Get-Content 'C:\TinyDLP\TinyDLP_Hook.log' -Tail 50"

echo.
echo ========================================
echo Test complete. Press any key to exit.
pause
