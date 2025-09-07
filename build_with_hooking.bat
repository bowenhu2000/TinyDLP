@echo off
echo Building TinyDLP with DLL Injection API Hooking...
echo.

REM Find MSBuild
set "MSBUILD_PATH="
for /f "usebackq tokens=*" %%i in (`where msbuild 2^>nul`) do (
    set "MSBUILD_PATH=%%i"
    goto :found_msbuild
)

REM Try common Visual Studio 2019 paths
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
) else (
    echo MSBuild not found. Please install Visual Studio 2019 or add MSBuild to PATH.
    pause
    exit /b 1
)

:found_msbuild
echo Using MSBuild: %MSBUILD_PATH%
echo.

REM Build the solution
echo Building TinyDLP solution...
"%MSBUILD_PATH%" TinyDLP.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.
echo Files created:
echo - TinyDLP.exe (Main application)
echo - TinyDLP_Hook.dll (API hooking DLL)
echo.
echo The application now uses DLL injection to intercept file operations
echo and block PDF saves to USB drives at the API level.
echo.
pause
