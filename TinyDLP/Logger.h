#pragma once

#include "Common.h"

class Logger {
private:
    static std::mutex logMutex;
    static std::wofstream logFile;
    static bool isInitialized;

public:
    static bool Initialize();
    static void Shutdown();
    static void Log(LogLevel level, const std::wstring& message);
    static void LogFileOperation(const FileOperationInfo& fileOp);
    static void LogUSBEvent(const std::wstring& event, const USBDeviceInfo& device);
    static std::wstring GetCurrentTimestamp();
    static std::wstring LogLevelToString(LogLevel level);
};
