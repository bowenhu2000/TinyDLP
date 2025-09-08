@echo off
echo Building TinyDLP_Hook DLL...

REM Build the hook DLL
cd TinyDLP\TinyDLP_Hook
msbuild TinyDLP_Hook.vcxproj /p:Configuration=Debug /p:Platform=x64 /verbosity:minimal

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo TinyDLP_Hook DLL built successfully!
    echo ========================================
    echo.
    echo DLL Location: TinyDLP\TinyDLP_Hook\x64\Debug\TinyDLP_Hook.dll
    echo.
    echo You can now run TinyDLP.exe to test PDF blocking.
) else (
    echo.
    echo ========================================
    echo Build failed! Check the errors above.
    echo ========================================
)

cd ..\..
pause
