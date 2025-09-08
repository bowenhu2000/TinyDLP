@echo off
echo ========================================
echo TinyDLP PDF Copy Blocking Test
echo ========================================
echo.

echo Checking if TinyDLP_Hook.dll exists...
if exist "TinyDLP\TinyDLP_Hook\x64\Debug\TinyDLP_Hook.dll" (
    echo [OK] TinyDLP_Hook.dll found
) else (
    echo [ERROR] TinyDLP_Hook.dll not found!
    echo Please run build_hook_dll.bat first
    pause
    exit /b 1
)

echo.
echo Checking if TinyDLP.exe exists...
if exist "TinyDLP\x64\Debug\TinyDLP.exe" (
    echo [OK] TinyDLP.exe found
) else (
    echo [ERROR] TinyDLP.exe not found!
    echo Please build the main project first
    pause
    exit /b 1
)

echo.
echo Checking if log directory exists...
if not exist "C:\TinyDLP" (
    echo Creating log directory...
    mkdir "C:\TinyDLP"
)

echo.
echo ========================================
echo Starting TinyDLP...
echo ========================================
echo.
echo 1. TinyDLP will inject into explorer.exe
echo 2. Try copying a PDF file to a USB drive
echo 3. Check C:\TinyDLP\TinyDLP_Hook.log for details
echo 4. Press Ctrl+C to stop TinyDLP
echo.

cd TinyDLP\x64\Debug
TinyDLP.exe

pause
