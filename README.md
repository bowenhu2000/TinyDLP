# TinyDLP - Data Loss Prevention System

A lightweight Data Loss Prevention (DLP) system that blocks PDF file transfers to USB drives using API hooking technology.

##  Features

- **PDF File Blocking**: Prevents PDF files from being copied to USB drives
- **API Hooking**: Uses Microsoft Detours library for low-level API interception
- **Process Injection**: Automatically injects hooks into target processes
- **Comprehensive Logging**: Detailed logging of all file operations and blocking attempts
- **Command Line Support**: Blocks PDF transfers via command line tools (cmd.exe, PowerShell)
- **Real-time Monitoring**: Continuous process monitoring and automatic hook injection
- **GUI Interface**: Modern system tray interface with real-time log viewer

##  How It Works

TinyDLP uses API hooking to intercept Windows file operations:

1. **DLL Injection**: Injects a hook DLL into target processes
2. **API Interception**: Hooks critical file APIs (CreateFileW, CopyFileW, WriteFile, etc.)
3. **File Analysis**: Analyzes file paths to detect PDF files and USB drives
4. **Blocking**: Prevents PDF file operations to USB drives with user notification

##  Requirements

- **Windows 10/11** (x64)
- **Visual Studio 2019** or later
- **Administrator privileges** (required for DLL injection)
- **USB drive** for testing

##  Installation

### 1. Clone the Repository
```bash
git clone https://github.com/yourusername/TinyDLP.git
cd TinyDLP
```

### 2. Build the Project
```bash
# Build the main application
build.bat

# Build the hook DLL
build_hook_dll.bat
```

### 3. Run as Administrator
```bash
# Start TinyDLP (must run as Administrator)
TinyDLP\x64\Release\TinyDLP.exe
```

##  Usage

### GUI Interface
1. **Start TinyDLP** as Administrator
2. **Look for the system tray icon** in the bottom-right corner
3. **Right-click the icon** to access the context menu:
   - **About**: View application information
   - **Logs**: Open real-time log viewer
   - **Exit**: Close the application

### Basic Usage
1. **Start TinyDLP** as Administrator
2. **Connect a USB drive** (e.g., K: drive)
3. **Try copying a PDF file** to the USB drive
4. **View logs** using the system tray menu

### Testing
```bash
# Create a test PDF file
echo test content > test.pdf

# Try to copy to USB drive (will be blocked)
copy test.pdf K:\test.pdf
```

### Log Monitoring
The GUI provides real-time log viewing:
- Right-click the system tray icon
- Select "Logs" to open the log viewer
- View real-time updates as events occur
- Auto-scrolls to show the latest entries

##  Project Structure

```
TinyDLP/
 TinyDLP/                    # Main application
    main.cpp               # Application entry point
    SystemTray.h/.cpp      # System tray functionality
    DLLInjector.cpp        # DLL injection logic
    Logger.cpp             # Logging system
    TinyDLP.vcxproj        # Main project file
 TinyDLP_Hook/              # Hook DLL
    TinyDLP_Hook.cpp       # API hook implementations
    TinyDLP_Hook.h         # Hook definitions
    TinyDLP_Hook.vcxproj   # Hook project file
 detours/                   # Microsoft Detours library
 build.bat                  # Main build script
 build_hook_dll.bat         # Hook DLL build script
 test_cmd_copy.bat          # Testing script
 test_gui.bat               # GUI testing script
```

##  Supported Operations

###  Currently Blocked
- **Command Line Copy**: `copy`, `xcopy`, `robocopy`
- **PowerShell Operations**: `Copy-Item`, `Move-Item`
- **Applications using Win32 APIs**: Most traditional file operations

###  Not Currently Blocked
- **Windows Explorer**: Uses IFileOperation COM interface
- **Modern Applications**: Some apps use different APIs

##  Logging

The system provides comprehensive logging with real-time GUI display:

```
[2025-09-12 22:35:58.112] [INFO] === TinyDLP Hook DLL Successfully Injected ===
[2025-09-12 22:35:58.112] [INFO] Target Process: cmd.exe
[2025-09-12 22:35:58.112] [INFO] Process ID: 2844
[2025-09-12 22:35:58.113] [INFO] All Detours hooks installed successfully
[2025-09-12 22:35:58.114] [WARNING] === PDF SAVE TO USB BLOCKED ===
[2025-09-12 22:35:58.114] [WARNING] File: K:\test.pdf
[2025-09-12 22:35:58.114] [WARNING] Reason: PDF file save to USB drive blocked by TinyDLP
```

##  Configuration

### Target Processes
The system automatically injects hooks into:
- `explorer.exe` (Windows Explorer)
- `cmd.exe` (Command Prompt)
- `powershell.exe` (PowerShell)
- `winword.exe` (Microsoft Word)
- `excel.exe` (Microsoft Excel)
- And more...

### File Types
Currently blocks:
- **PDF files** (`.pdf` extension)

### Drive Types
Blocks operations to:
- **Removable drives** (USB drives, external hard drives)

##  Development

### Building from Source
```bash
# Build main application
msbuild TinyDLP.sln /p:Configuration=Release /p:Platform=x64

# Build hook DLL only
msbuild TinyDLP\TinyDLP_Hook\TinyDLP_Hook.vcxproj /p:Configuration=Release /p:Platform=x64
```

### Adding New File Types
Edit `TinyDLP_Hook.cpp` and modify the `IsPDFFile` function:

```cpp
bool IsPDFFile(const std::wstring& filePath) {
    // Add support for other file types
    if (filePath.ends_with(L".docx") || filePath.ends_with(L".xlsx")) {
        return true;
    }
    // ... existing PDF logic
}
```

### Adding New Target Processes
Edit `DLLInjector.cpp` and modify the `ShouldInjectProcess` function:

```cpp
bool DLLInjector::ShouldInjectProcess(const std::wstring& processName) {
    // Add new target processes
    if (processName == L"notepad.exe") {
        return true;
    }
    // ... existing logic
}
```

##  Troubleshooting

### Common Issues

**1. DLL Injection Fails**
- Ensure TinyDLP is running as Administrator
- Check if antivirus is blocking the injection
- Verify the hook DLL exists at the correct path

**2. PDF Copy Not Blocked**
- Check if the operation is using Windows Explorer (not supported yet)
- Verify the file has `.pdf` extension
- Check if the target drive is detected as removable

**3. No Logs Generated**
- Ensure `C:\TinyDLP` directory exists
- Check file permissions
- Verify the hook DLL is properly injected

**4. System Tray Icon Not Visible**
- Check if the icon is hidden in the system tray
- Right-click the taskbar and select "Taskbar settings"
- Look for "Select which icons appear on the taskbar"

### Debug Mode
Enable detailed logging by setting the log level in the code:

```cpp
// In TinyDLP_Hook.cpp
LogMessage(LOG_INFO, L"Detailed debug information");
```

##  License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

##  Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

##  Support

If you encounter any issues or have questions:

1. Check the [Issues](https://github.com/yourusername/TinyDLP/issues) page
2. Create a new issue with detailed information
3. Include logs from the real-time log viewer

##  Future Enhancements

- [x] GUI interface with system tray
- [x] Real-time log viewer
- [ ] Windows Explorer support (IFileOperation hooks)
- [ ] Configuration interface
- [ ] Support for additional file types
- [ ] Network drive blocking
- [ ] Email attachment blocking
- [ ] Cloud storage blocking
- [ ] Configuration file support
- [ ] Real-time notifications
- [ ] Statistics and reporting

##  Disclaimer

This software is for educational and research purposes. Users are responsible for complying with applicable laws and regulations. The authors are not responsible for any misuse of this software.

##  Acknowledgments

- [Microsoft Detours](https://github.com/Microsoft/Detours) - API hooking library
- Windows API documentation
- Community contributors and testers

---

**Made with  for data protection and security research**

 2025 TinyDLP
