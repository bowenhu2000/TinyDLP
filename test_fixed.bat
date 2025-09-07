@echo off
echo Testing TinyDLP with Mutex Deadlock Fix...
echo.
echo IMPORTANT: Run as Administrator
echo.
echo Starting TinyDLP...
echo This version should not have mutex deadlock issues.
echo.

REM Run the application
x64\Release\TinyDLP.exe

echo.
echo TinyDLP has stopped.
pause
