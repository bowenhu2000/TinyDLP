@echo off
echo Cleaning up TinyDLP processes and DLL locks...
echo.

echo Checking for running TinyDLP processes...
tasklist /FI "IMAGENAME eq TinyDLP.exe" 2>nul | find /I "TinyDLP.exe" >nul
if %errorLevel% equ 0 (
    echo Found running TinyDLP.exe processes. Terminating...
    taskkill /F /IM TinyDLP.exe 2>nul
    timeout /t 2 /nobreak >nul
)

echo.
echo Checking for processes with TinyDLP_Hook.dll loaded...
for /f "tokens=2" %%i in ('tasklist /FI "IMAGENAME eq chrome.exe" /FO CSV ^| find /C "chrome.exe"') do (
    if %%i gtr 0 (
        echo Found Chrome processes that may have TinyDLP_Hook.dll loaded.
        echo Note: Chrome processes will be restarted automatically.
        taskkill /F /IM chrome.exe 2>nul
        timeout /t 3 /nobreak >nul
    )
)

echo.
echo Checking for Explorer processes...
for /f "tokens=2" %%i in ('tasklist /FI "IMAGENAME eq explorer.exe" /FO CSV ^| find /C "explorer.exe"') do (
    if %%i gtr 0 (
        echo Found Explorer processes that may have TinyDLP_Hook.dll loaded.
        echo Restarting Explorer...
        taskkill /F /IM explorer.exe 2>nul
        timeout /t 2 /nobreak >nul
        start explorer.exe
    )
)

echo.
echo Checking for any remaining processes with TinyDLP_Hook.dll...
powershell -Command "Get-Process | Where-Object {$_.Modules.ModuleName -like '*TinyDLP_Hook*'} | Select-Object ProcessName, Id"

echo.
echo Cleanup completed. You can now build the project.
echo.
pause
