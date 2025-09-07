@echo off
echo Testing TinyDLP with USB Drive Comparison Fix...
echo.
echo This version fixes the issue where:
echo - device.driveLetter = "K:\" (3 characters)
echo - drivePath.substr(0,2) = "K:" (2 characters)
echo - Comparison was failing because "K:\" != "K:"
echo.
echo The fix now properly compares both as "K:"
echo.
echo IMPORTANT: Run as Administrator
echo.

REM Run the application
x64\Release\TinyDLP.exe

echo.
echo TinyDLP has stopped.
pause
