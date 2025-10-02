# cURL Library Build Script for TinyDLP
# This script downloads and builds cURL library for use with TinyDLP HTTP client

param(
    [string]$CurlVersion = "8.5.0",
    [string]$TargetDir = "C:\Users\BowenGit\Documents\GitHub\TinyDLP\curl",
    [string]$OpenSSLDir = "C:\Users\BowenGit\Documents\GitHub\TinyDLP\openssl350"
)

Write-Host "cURL Library Build Script for TinyDLP" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor Green
Write-Host ""

Write-Host "cURL Version: $CurlVersion" -ForegroundColor Yellow
Write-Host "Target Directory: $TargetDir" -ForegroundColor Yellow
Write-Host "OpenSSL Directory: $OpenSSLDir" -ForegroundColor Yellow
Write-Host ""

# Create target directories
$Directories = @(
    "$TargetDir",
    "$TargetDir\x64",
    "$TargetDir\x86",
    "$TargetDir\source"
)

foreach ($dir in $Directories) {
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "Created directory: $dir" -ForegroundColor Green
    }
}

# Check for required tools
Write-Host "Checking for required build tools..." -ForegroundColor Yellow
Write-Host ""

$tools = @{
    "Visual Studio C++" = "cl"
    "Perl" = "perl"
    "NASM" = "nasm"
    "CMake" = "cmake"
}

$missingTools = @()

foreach ($tool in $tools.GetEnumerator()) {
    try {
        $path = Get-Command $tool.Value -ErrorAction Stop
        Write-Host " $($tool.Key): $($path.Source)" -ForegroundColor Green
    } catch {
        Write-Host " $($tool.Key): Not found" -ForegroundColor Red
        $missingTools += $tool.Key
    }
}

if ($missingTools.Count -gt 0) {
    Write-Host ""
    Write-Host "Missing tools detected. Please install:" -ForegroundColor Red
    foreach ($tool in $missingTools) {
        Write-Host "- $tool" -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "After installing, restart this script." -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "All required tools found!" -ForegroundColor Green
Write-Host ""

# Set variables
$CurlSource = "curl-$CurlVersion"
$CurlArchive = "$CurlSource.tar.gz"
$DownloadUrl = "https://curl.se/download/$CurlArchive"

# Download cURL source if not exists
if (!(Test-Path $CurlArchive)) {
    Write-Host "Downloading cURL $CurlVersion source code..." -ForegroundColor Yellow
    Write-Host "URL: $DownloadUrl" -ForegroundColor Yellow
    Write-Host ""
    
    try {
        Invoke-WebRequest -Uri $DownloadUrl -OutFile $CurlArchive -UseBasicParsing
        Write-Host " Download completed!" -ForegroundColor Green
    } catch {
        Write-Host " ERROR: Failed to download cURL source" -ForegroundColor Red
        Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    Write-Host ""
} else {
    Write-Host " cURL source already exists, skipping download." -ForegroundColor Green
    Write-Host ""
}

# Extract source if not exists
if (!(Test-Path $CurlSource)) {
    Write-Host "Extracting cURL source..." -ForegroundColor Yellow
    Write-Host ""
    
    try {
        if (Get-Command tar -ErrorAction SilentlyContinue) {
            tar -xzf $CurlArchive
        } else {
            Write-Host "tar not found, trying 7-Zip..." -ForegroundColor Yellow
            if (Get-Command 7z -ErrorAction SilentlyContinue) {
                7z x $CurlArchive
            } else {
                Write-Host " ERROR: Neither tar nor 7-Zip found for extraction" -ForegroundColor Red
                Write-Host "Please install tar (Windows 10+) or 7-Zip" -ForegroundColor Red
                exit 1
            }
        }
        Write-Host " Extraction completed!" -ForegroundColor Green
    } catch {
        Write-Host " ERROR: Failed to extract cURL source" -ForegroundColor Red
        Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    Write-Host ""
} else {
    Write-Host " cURL source already extracted, skipping extraction." -ForegroundColor Green
    Write-Host ""
}

# Change to source directory
Set-Location $CurlSource

Write-Host "Building cURL $CurlVersion..." -ForegroundColor Yellow
Write-Host ""

# Build for x64
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building for x64 (64-bit)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

try {
    Write-Host "Configuring for x64..." -ForegroundColor Yellow
    
    # Configure with OpenSSL support
    $configureArgs = @(
        "--prefix=$TargetDir\x64",
        "--with-ssl=$OpenSSLDir\x64",
        "--enable-http",
        "--enable-https",
        "--enable-ftp",
        "--enable-ftps",
        "--enable-file",
        "--enable-ldap",
        "--enable-ldaps",
        "--enable-rtsp",
        "--enable-proxy",
        "--enable-dict",
        "--enable-telnet",
        "--enable-tftp",
        "--enable-pop3",
        "--enable-imap",
        "--enable-smtp",
        "--enable-gopher",
        "--enable-manual",
        "--enable-libcurl-option",
        "--enable-ipv6",
        "--enable-versioned-symbols",
        "--enable-threaded-resolver",
        "--enable-verbose",
        "--disable-shared",
        "--enable-static"
    )
    
    & perl configure @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "Configure failed for x64" }
    
    Write-Host "Building x64..." -ForegroundColor Yellow
    & nmake
    if ($LASTEXITCODE -ne 0) { throw "Build failed for x64" }
    
    Write-Host "Installing x64..." -ForegroundColor Yellow
    & nmake install
    if ($LASTEXITCODE -ne 0) { throw "Install failed for x64" }
    
    Write-Host " x64 build completed successfully!" -ForegroundColor Green
} catch {
    Write-Host " ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Clean for x86 build
Write-Host "Cleaning for x86 build..." -ForegroundColor Yellow
& nmake clean

# Build for x86
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building for x86 (32-bit)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

try {
    Write-Host "Configuring for x86..." -ForegroundColor Yellow
    
    # Configure with OpenSSL support for x86
    $configureArgs = @(
        "--prefix=$TargetDir\x86",
        "--with-ssl=$OpenSSLDir\x86",
        "--enable-http",
        "--enable-https",
        "--enable-ftp",
        "--enable-ftps",
        "--enable-file",
        "--enable-ldap",
        "--enable-ldaps",
        "--enable-rtsp",
        "--enable-proxy",
        "--enable-dict",
        "--enable-telnet",
        "--enable-tftp",
        "--enable-pop3",
        "--enable-imap",
        "--enable-smtp",
        "--enable-gopher",
        "--enable-manual",
        "--enable-libcurl-option",
        "--enable-ipv6",
        "--enable-versioned-symbols",
        "--enable-threaded-resolver",
        "--enable-verbose",
        "--disable-shared",
        "--enable-static"
    )
    
    & perl configure @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "Configure failed for x86" }
    
    Write-Host "Building x86..." -ForegroundColor Yellow
    & nmake
    if ($LASTEXITCODE -ne 0) { throw "Build failed for x86" }
    
    Write-Host "Installing x86..." -ForegroundColor Yellow
    & nmake install
    if ($LASTEXITCODE -ne 0) { throw "Install failed for x86" }
    
    Write-Host " x86 build completed successfully!" -ForegroundColor Green
} catch {
    Write-Host " ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Return to parent directory
Set-Location ..

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "cURL $CurlVersion Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Build results are located in:" -ForegroundColor Yellow
Write-Host "  x64: $TargetDir\x64" -ForegroundColor White
Write-Host "  x86: $TargetDir\x86" -ForegroundColor White
Write-Host ""
Write-Host "Libraries available:" -ForegroundColor Yellow
Write-Host "  x64: $TargetDir\x64\lib\libcurl.lib" -ForegroundColor White
Write-Host "  x86: $TargetDir\x86\lib\libcurl.lib" -ForegroundColor White
Write-Host ""
Write-Host "Headers available:" -ForegroundColor Yellow
Write-Host "  x64: $TargetDir\x64\include" -ForegroundColor White
Write-Host "  x86: $TargetDir\x86\include" -ForegroundColor White
Write-Host ""

# Verify the build
Write-Host "Verifying build..." -ForegroundColor Yellow
$libs = @(
    "$TargetDir\x64\lib\libcurl.lib",
    "$TargetDir\x86\lib\libcurl.lib"
)

$allGood = $true
foreach ($lib in $libs) {
    if (Test-Path $lib) {
        Write-Host " $lib" -ForegroundColor Green
    } else {
        Write-Host " Missing: $lib" -ForegroundColor Red
        $allGood = $false
    }
}

if ($allGood) {
    Write-Host ""
    Write-Host " All libraries built successfully!" -ForegroundColor Green
    Write-Host "cURL is ready for use with TinyDLP HTTP client." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host " Some libraries are missing. Please check the build process." -ForegroundColor Red
}

Write-Host ""
Write-Host "Press any key to continue..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
