# Download and setup Microsoft Detours library
Write-Host "Setting up Microsoft Detours library for TinyDLP..."

# Create detours directory
$detoursDir = ".\detours"
if (!(Test-Path $detoursDir)) {
    New-Item -ItemType Directory -Path $detoursDir
    Write-Host "Created detours directory"
}

# Download Detours from GitHub
$detoursUrl = "https://github.com/Microsoft/Detours/archive/refs/heads/main.zip"
$detoursZip = ".\detours\detours.zip"

try {
    Write-Host "Downloading Detours from GitHub..."
    Invoke-WebRequest -Uri $detoursUrl -OutFile $detoursZip
    Write-Host "Download completed"
    
    # Extract the zip file
    Write-Host "Extracting Detours..."
    Expand-Archive -Path $detoursZip -DestinationPath $detoursDir -Force
    Write-Host "Extraction completed"
    
    # Find the extracted folder
    $extractedFolder = Get-ChildItem -Path $detoursDir -Directory | Where-Object { $_.Name -like "Detours-*" } | Select-Object -First 1
    
    if ($extractedFolder) {
        # Copy the source files to our project
        $sourceDir = Join-Path $extractedFolder.FullName "src"
        $targetDir = ".\TinyDLP\TinyDLP_Hook\detours"
        
        if (Test-Path $sourceDir) {
            if (!(Test-Path $targetDir)) {
                New-Item -ItemType Directory -Path $targetDir
            }
            
            # Copy essential files
            Copy-Item "$sourceDir\detours.h" $targetDir -Force
            Copy-Item "$sourceDir\detours.lib" $targetDir -Force -ErrorAction SilentlyContinue
            Copy-Item "$sourceDir\detours.dll" $targetDir -Force -ErrorAction SilentlyContinue
            
            Write-Host "Detours source files copied to project"
        }
    }
    
    # Clean up
    Remove-Item $detoursZip -Force
    Remove-Item $extractedFolder.FullName -Recurse -Force
    Write-Host "Cleanup completed"
    
} catch {
    Write-Host "Error downloading Detours: $($_.Exception.Message)"
    Write-Host "Please manually download Detours from: https://github.com/Microsoft/Detours"
    Write-Host "And place detours.h and detours.lib in TinyDLP\TinyDLP_Hook\detours\"
}

Write-Host "Detours setup completed!"
