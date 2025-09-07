@echo off
echo Testing TinyDLP with Proper File Name Detection...
echo.
echo This version now properly detects actual file names using ReadDirectoryChangesW
echo instead of just hardcoded "example.pdf"
echo.
echo Key improvements:
echo - Uses ReadDirectoryChangesW to get real file change notifications
echo - Extracts actual file names from FILE_NOTIFY_INFORMATION
echo - Properly checks if files are PDF files
echo - Logs actual file operations with real file names
echo.
echo IMPORTANT: Run as Administrator
echo.

REM Run the application
x64\Release\TinyDLP.exe

echo.
echo TinyDLP has stopped.
pause
