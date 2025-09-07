# TinyDLP - Data Loss Prevention Tool

TinyDLP is a Windows application that monitors and blocks PDF file saves to USB drives for data loss prevention.

## Features

- **USB Device Monitoring**: Detects when USB drives are connected or disconnected
- **File System Monitoring**: Monitors file operations on USB drives
- **PDF File Blocking**: Prevents PDF files from being saved to USB drives
- **Alert System**: Shows warning dialogs when blocking attempts
- **Comprehensive Logging**: Logs all events to TinyDLP.log file
- **Administrator Privileges**: Requires admin rights for system-level monitoring

## Requirements

- Windows 10 or later
- Visual Studio 2019 or later
- Administrator privileges to run the application
- C++17 compatible compiler

## Building the Project

### Using Visual Studio 2019

1. Open TinyDLP.sln in Visual Studio 2019
2. Select the appropriate configuration (Debug or Release)
3. Select x64 platform
4. Build the solution (Ctrl+Shift+B)

### Using Command Line

`cmd
# Navigate to the project directory
cd TinyDLP

# Build using MSBuild
msbuild TinyDLP.sln /p:Configuration=Release /p:Platform=x64
`

## Usage

1. **Run as Administrator**: The application must be run with administrator privileges
2. **Start Monitoring**: The application will automatically start monitoring USB drives
3. **PDF Blocking**: When a PDF file save attempt is detected on a USB drive:
   - An alert dialog will be shown
   - The operation will be logged
   - The save operation will be blocked

## Architecture

The application consists of several key components:

- **Logger**: Handles all logging operations with timestamps and log levels
- **USBMonitor**: Monitors USB device insertion/removal using Windows device notifications
- **FileMonitor**: Monitors file system changes on USB drives
- **AlertDialog**: Shows user-friendly alert dialogs for blocked operations
- **Main Application**: Coordinates all components and handles the message loop

## Logging

All events are logged to TinyDLP.log with the following information:
- Timestamp with millisecond precision
- Log level (INFO, WARNING, ERROR)
- Detailed event information

## Security Considerations

- The application requires administrator privileges to monitor system-level file operations
- File blocking is implemented at the application level
- For production use, consider implementing kernel-level file system filtering

## Limitations

- This is a demonstration implementation
- File blocking is simulated (actual blocking would require kernel-level drivers)
- USB device detection is simplified
- File monitoring uses basic Windows APIs

## Future Enhancements

- Kernel-level file system filtering for true file blocking
- Configuration file for customizable rules
- Network monitoring capabilities
- Advanced USB device identification
- Real-time file content analysis

## License

This project is for educational and demonstration purposes.
