@echo off
echo Testing TinyDLP with DLL Injection API Hooking...
echo.
echo This version uses DLL injection to intercept file operations at the API level.
echo.
echo Key features:
echo - Monitors running processes for applications that might save PDFs
echo - Injects TinyDLP_Hook.dll into target processes
echo - Intercepts CreateFileW, WriteFile, CopyFileW, MoveFileW API calls
echo - Blocks PDF file operations to USB drives in real-time
echo - Shows alert dialogs when blocking occurs
echo - Logs all activities to TinyDLP.log and TinyDLP_Hook.log
echo.
echo Target processes include:
echo - Microsoft Office (Word, Excel, PowerPoint)
echo - Adobe Acrobat/Reader
echo - Web browsers (Chrome, Firefox, Edge)
echo - Text editors (Notepad, VS Code, etc.)
echo - Windows Explorer
echo.
echo IMPORTANT: Run as Administrator
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
