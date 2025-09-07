@echo off
echo Testing TinyDLP with Deadlock Fix...
echo.
echo The previous version had a deadlock issue in the DLLInjector class.
echo This version fixes the mutex deadlock by using separate functions
echo for thread-safe and unsafe access to the injected processes list.
echo.
echo Key fixes:
echo - Added IsProcessInjectedUnsafe() for use within already-locked functions
echo - InjectDLL() now uses IsProcessInjectedUnsafe() to avoid deadlock
echo - IsProcessInjected() still provides thread-safe access from external calls
echo.
echo IMPORTANT: Run as Administrator
echo.

REM Check if running as administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This application must be run as Administrator!
    echo.
    echo Please right-click on this batch file and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

echo Starting TinyDLP with deadlock fix...
echo.

REM Run the application
x64\Release\TinyDLP.exe

echo.
echo TinyDLP has stopped.
echo.
echo If no deadlock occurred, the fix was successful!
echo Check the log files for detailed activity.
echo.
pause
