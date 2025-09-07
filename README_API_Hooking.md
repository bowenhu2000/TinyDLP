# TinyDLP - Data Loss Prevention with API Hooking

## Overview

TinyDLP is a Data Loss Prevention (DLP) application that monitors and blocks attempts to save PDF files to USB drives. This implementation uses **DLL Injection with API Hooking** to intercept file operations at the system level, providing real-time blocking of PDF save operations.

## Architecture

### Components

1. **Main Application (TinyDLP.exe)**
   - Monitors running processes
   - Injects hook DLL into target processes
   - Manages the overall DLP system

2. **Hook DLL (TinyDLP_Hook.dll)**
   - Injected into target processes
   - Intercepts Windows API calls
   - Blocks PDF operations on USB drives

### API Hooking Mechanism

The system intercepts the following Windows API functions:
- `CreateFileW` - File creation operations
- `WriteFile` - File writing operations  
- `CopyFileW` - File copying operations
- `MoveFileW` - File moving operations

## How It Works

1. **Process Monitoring**: Main application continuously monitors running processes
2. **Target Detection**: Identifies processes that might save PDF files (Office apps, browsers, etc.)
3. **DLL Injection**: Injects `TinyDLP_Hook.dll` into target processes using `CreateRemoteThread`
4. **API Interception**: DLL hooks the file operation APIs in the target process
5. **Real-time Blocking**: When a PDF file operation to USB drive is detected, it's blocked immediately
6. **User Notification**: Shows alert dialog and logs the blocked operation

## Target Processes

The system monitors and injects into these types of applications:
- **Microsoft Office**: Word, Excel, PowerPoint
- **Adobe Applications**: Acrobat, Reader
- **Web Browsers**: Chrome, Firefox, Edge
- **Text Editors**: Notepad, VS Code, Notepad++
- **Development Tools**: Visual Studio, etc.
- **File Managers**: Windows Explorer

## Building the Project

### Prerequisites
- Visual Studio 2019 or later
- Windows 10/11 SDK
- Administrator privileges (for testing)

### Build Steps

1. **Build the complete solution**:
   ```batch
   .\build_with_hooking.bat
   ```

2. **Files created**:
   - `x64\Release\TinyDLP.exe` - Main application
   - `x64\Release\TinyDLP_Hook.dll` - Hook DLL

## Running the Application

### Requirements
- **Administrator privileges** (required for DLL injection)
- Windows 10/11
- Target applications must be running

### Test the System

1. **Run as Administrator**:
   ```batch
   .\test_api_hooking.bat
   ```

2. **Test scenarios**:
   - Open Microsoft Word and try to save a PDF to a USB drive
   - Use a web browser to download a PDF to a USB drive
   - Copy a PDF file to a USB drive using Windows Explorer

## Logging

### Log Files

1. **TinyDLP.log** (Main application)
   - Process monitoring activity
   - DLL injection success/failure
   - System events

2. **C:\TinyDLP\TinyDLP_Hook.log** (DLL activity)
   - API hook installation
   - Blocked file operations
   - Process-specific events

### Log Format
```
[2024-01-15 14:30:25.123] [WARN] BLOCKED: CREATE_FILE | Process: winword.exe (PID: 1234) | File: E:\document.pdf
```

## Technical Implementation

### DLL Injection Process

1. **Process Enumeration**: Uses `CreateToolhelp32Snapshot` to enumerate processes
2. **Target Selection**: Filters processes based on executable names
3. **Memory Allocation**: Allocates memory in target process for DLL path
4. **Thread Creation**: Creates remote thread to load the DLL
5. **Hook Installation**: DLL installs API hooks on load

### API Hooking Details

The hook DLL uses function pointer redirection:
- Saves original function addresses
- Replaces function calls with hooked versions
- Checks file paths for PDF files on USB drives
- Blocks operations and shows user alerts

### USB Drive Detection

```cpp
bool IsUSBDrive(const std::wstring& filePath) {
    std::wstring drive = filePath.substr(0, 2);
    UINT driveType = GetDriveTypeW(drive.c_str());
    return (driveType == DRIVE_REMOVABLE);
}
```

## Security Considerations

### Administrator Privileges
- Required for DLL injection into other processes
- Necessary for accessing process memory and creating remote threads

### Antivirus Compatibility
- May trigger antivirus warnings due to DLL injection
- Consider adding to antivirus exclusions for testing

### Process Compatibility
- Works with most Windows applications
- Some protected processes may not be injectable
- 64-bit applications require 64-bit DLL

## Limitations

1. **Process Protection**: Some system processes cannot be injected
2. **Architecture Matching**: 64-bit processes require 64-bit DLL
3. **Antivirus Interference**: May be blocked by security software
4. **Performance Impact**: Minimal overhead on file operations

## Future Enhancements

1. **Microsoft Detours Integration**: Use professional hooking library
2. **Kernel-level Filtering**: Implement file system filter driver
3. **Policy Management**: Configurable rules and exceptions
4. **Network Monitoring**: Extend to network file operations
5. **Encryption Detection**: Block encrypted file transfers

## Troubleshooting

### Common Issues

1. **"Access Denied" errors**:
   - Ensure running as Administrator
   - Check antivirus exclusions

2. **DLL injection failures**:
   - Verify target process is 64-bit
   - Check if process is protected

3. **Hooks not working**:
   - Verify DLL is loaded in target process
   - Check log files for error messages

### Debug Information

Enable detailed logging by checking:
- `TinyDLP.log` for injection status
- `C:\TinyDLP\TinyDLP_Hook.log` for hook activity
- Windows Event Viewer for system errors

## License

This project is for educational and research purposes. Use responsibly and in accordance with applicable laws and regulations.
