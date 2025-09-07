@echo off
echo Building TinyDLP...

REM Check if MSBuild is available
where msbuild >nul 2>nul
if %errorlevel% neq 0 (
    echo MSBuild not found. Please ensure Visual Studio 2019 is installed.
    echo You can also build using Visual Studio IDE by opening TinyDLP.sln
    pause
    exit /b 1
)

REM Build the solution
msbuild TinyDLP.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal

if %errorlevel% equ 0 (
    echo.
    echo Build successful! Executable should be in: TinyDLP\x64\Release\TinyDLP.exe
    echo.
    echo IMPORTANT: Run as Administrator for proper functionality
) else (
    echo.
    echo Build failed. Please check the error messages above.
)

pause
