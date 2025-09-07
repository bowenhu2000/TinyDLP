@echo off
echo ========================================
echo TinyDLP - API Hooking Implementation
echo ========================================
echo.
echo This version implements DLL injection with API hooking to intercept
echo file operations at the system level and block PDF saves to USB drives.
echo.
echo Architecture:
echo - Main Application (TinyDLP.exe): Monitors processes and injects DLL
echo - Hook DLL (TinyDLP_Hook.dll): Intercepts API calls in target processes
echo.
echo How it works:
echo 1. Main app monitors running processes
echo 2. When target processes are detected, DLL is injected
echo 3. DLL hooks CreateFileW, WriteFile, CopyFileW, MoveFileW APIs
echo 4. When PDF files are detected on USB drives, operations are blocked
echo 5. User sees alert dialog and operation is logged
echo.
echo Target processes include:
echo - Microsoft Office (Word, Excel, PowerPoint)
echo - Adobe Acrobat/Reader
echo - Web browsers (Chrome, Firefox, Edge)
echo - Text editors (Notepad, VS Code, etc.)
echo - Windows Explorer
echo.
echo IMPORTANT: Must run as Administrator for DLL injection to work
echo.
echo Log files:
echo - TinyDLP.log (Main application activity)
echo - C:\TinyDLP\TinyDLP_Hook.log (DLL injection activity)
echo.
echo ========================================
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

echo Starting TinyDLP with API hooking...
echo.

REM Run the application
x64\Release\TinyDLP.exe

echo.
echo TinyDLP has stopped.
echo.
echo Check the log files for detailed activity:
echo - TinyDLP.log (Main application logs)
echo - C:\TinyDLP\TinyDLP_Hook.log (DLL injection logs)
echo.
pause
