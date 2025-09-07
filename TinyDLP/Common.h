#pragma once

#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

// Common constants
#define LOG_FILE_PATH L"TinyDLP.log"
#define MAX_PATH_LENGTH 260
#define USB_DEVICE_CLASS_GUID L"{4d36e967-e325-11ce-bfc1-08002be10318}"

// Log levels
enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// File operation types
enum FileOperation {
    FILE_CREATE,
    FILE_WRITE,
    FILE_RENAME,
    FILE_DELETE
};

// USB device information structure
struct USBDeviceInfo {
    std::wstring devicePath;
    std::wstring friendlyName;
    std::wstring driveLetter;
    bool isRemovable;
};

// File operation information
struct FileOperationInfo {
    std::wstring filePath;
    std::wstring processName;
    DWORD processId;
    FileOperation operation;
    std::chrono::system_clock::time_point timestamp;
};
