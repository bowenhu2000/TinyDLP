$file = "TinyDLP/DLLInjector.cpp"
$content = Get-Content $file
$newContent = @()

foreach ($line in $content) {
    if ($line -match 'L"explorer\.exe"      // Windows Explorer') {
        $newContent += '        L"explorer.exe",     // Windows File Explorer'
        $newContent += '        L"dwm.exe",          // Desktop Window Manager (handles file operations)'
        $newContent += '        L"shell32.dll"       // Windows Shell (file operations)'
    } else {
        $newContent += $line
    }
}

Set-Content $file -Value $newContent
Write-Host "File updated successfully"
