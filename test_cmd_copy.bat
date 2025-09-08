@echo off
echo ========================================
echo TinyDLP Command Line Copy Test
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
echo Step 3: Create test PDF file
echo test content > test.pdf
echo Test PDF created: test.pdf

echo.
echo Step 4: Start TinyDLP (run as Administrator)
echo Please run TinyDLP.exe as Administrator in another window
echo Wait for DLL injection to complete, then press any key...
pause

echo.
echo Step 5: Test Command Line Copy
echo Now we will attempt to copy test.pdf to K:\ using cmd.exe
echo This should trigger our hooks if they work...
echo.

echo Press any key to start the copy test...
pause

echo.
echo Attempting copy operation...
copy test.pdf K:\test.pdf
echo Copy command completed.

echo.
echo Press any key to check the logs...
pause

echo.
echo ========================================
echo LOG ANALYSIS
echo ========================================
echo.

echo Checking for test.pdf operations...
findstr /C:"test.pdf" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No test.pdf operations found in logs
) else (
    echo [FOUND] test.pdf operations detected!
)

echo.
echo Checking for CopyFileW calls...
findstr /C:"CopyFileW called" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No CopyFileW calls found
) else (
    echo [FOUND] CopyFileW calls detected!
)

echo.
echo Checking for any file operations on K: drive...
findstr /C:"K:" "C:\TinyDLP\TinyDLP_Hook.log" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] No K: drive operations found
) else (
    echo [FOUND] K: drive operations detected!
)

echo.
echo ========================================
echo FULL LOG (Last 30 lines)
echo ========================================
powershell "Get-Content 'C:\TinyDLP\TinyDLP_Hook.log' -Tail 30"

echo.
echo ========================================
echo Test complete. Press any key to exit.
pause
