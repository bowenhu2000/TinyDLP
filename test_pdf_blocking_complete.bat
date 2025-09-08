@echo off
echo ========================================
echo TinyDLP PDF Copy Blocking Test
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
echo Step 2: Start TinyDLP (run as Administrator)
echo Please run TinyDLP.exe as Administrator in another window
echo.
echo Step 3: Test PDF Copy Blocking
echo 1. Make sure you have a USB drive connected (e.g., K:)
echo 2. Create a test PDF file: echo test > test.pdf
echo 3. Try to copy test.pdf to your USB drive
echo 4. Check C:\TinyDLP\TinyDLP_Hook.log for detailed logs
echo.

echo Creating test PDF file...
echo test > test.pdf
echo Test PDF created: test.pdf

echo.
echo ========================================
echo Now try copying test.pdf to your USB drive
echo Check the log file: C:\TinyDLP\TinyDLP_Hook.log
echo ========================================
echo.

echo Press any key to view the latest log entries...
pause

echo.
echo Latest log entries:
type "C:\TinyDLP\TinyDLP_Hook.log" | findstr /C:"CopyFileW" /C:"PDF" /C:"USB" /C:"BLOCKED"
echo.

pause
