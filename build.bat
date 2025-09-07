@echo off
echo Building TinyDLP...

REM Try to find MSBuild
set "MSBUILD_PATH="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
) else (
    echo MSBuild not found. Please ensure Visual Studio 2019 is installed.
    echo You can also build using Visual Studio IDE by opening TinyDLP.sln
    pause
    exit /b 1
)

REM Build the solution
"%MSBUILD_PATH%" TinyDLP.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal

if %errorlevel% equ 0 (
    echo.
    echo Build successful! Executable created: x64\Release\TinyDLP.exe
    echo.
    echo IMPORTANT: Run as Administrator for proper functionality
) else (
    echo.
    echo Build failed. Please check the error messages above.
)

pause
