# TinyDLP GUI Application

## Overview
TinyDLP has been updated with a modern GUI interface that runs in the system tray without showing a console window.

## New Features

### 1. System Tray Integration
- **No Console Window**: The application now runs silently without displaying a command-line window
- **System Tray Icon**: A TinyDLP icon appears in the bottom-right corner of the screen when the application is running
- **Context Menu**: Right-click the system tray icon to access the application menu

### 2. Context Menu Options
- **About**: Shows information about TinyDLP including version and description
- **Logs**: Opens a real-time log viewer window
- **Exit**: Closes the application

### 3. Real-Time Log Viewer
- **Scrollable Window**: View all log messages in a scrollable text window
- **Real-Time Updates**: Logs are updated in real-time as events occur
- **Auto-Scroll**: Automatically scrolls to show the latest log entries
- **Monospace Font**: Uses Consolas font for better readability of log data

## Usage Instructions

1. **Start the Application**:
   - Run `TinyDLP.exe` as Administrator
   - The application will start silently and show an icon in the system tray

2. **Access the Menu**:
   - Right-click the system tray icon
   - Select "About" to see application information
   - Select "Logs" to view real-time log messages
   - Select "Exit" to close the application

3. **View Logs**:
   - Click "Logs" from the context menu
   - The log viewer window will open showing all current log messages
   - The window automatically updates with new log entries
   - Close the window by clicking the X button (it will hide, not close completely)

## Technical Details

### Changes Made
- Converted from console application (`wmain`) to Windows application (`WinMain`)
- Added system tray functionality using Windows Shell API
- Created real-time log viewer with scrollable text control
- Integrated logger with system tray for real-time log display
- Updated project configuration to use Windows subsystem instead of Console

### Files Added
- `SystemTray.h` - Header file for system tray functionality
- `SystemTray.cpp` - Implementation of system tray and log viewer

### Files Modified
- `main.cpp` - Updated to use WinMain and integrate system tray
- `Logger.h` - Added callback function for system tray integration
- `Logger.cpp` - Added callback functionality to send logs to system tray
- `TinyDLP.vcxproj` - Updated to include new files and Windows subsystem

## Requirements
- Windows 10 or later
- Administrator privileges
- Visual Studio 2019 or later (for building from source)

## Building
Use the provided `build.bat` script to compile the application:
```
build.bat
```

The executable will be created in `x64\Release\TinyDLP.exe`.
