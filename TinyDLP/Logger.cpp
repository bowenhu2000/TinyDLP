#include "Logger.h"

std::mutex Logger::logMutex;
std::wofstream Logger::logFile;
bool Logger::isInitialized = false;

bool Logger::Initialize() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (isInitialized) {
        return true;
    }
    
    logFile.open(LOG_FILE_PATH, std::ios::app);
    if (!logFile.is_open()) {
        return false;
    }
    
    // Set locale safely to avoid exceptions
    try {
        logFile.imbue(std::locale("C"));
    } catch (...) {
        // Continue without locale if it fails
    }
    
    isInitialized = true;
    return true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (isInitialized) {
        logFile.close();
        isInitialized = false;
    }
}

void Logger::Log(LogLevel level, const std::wstring& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (!isInitialized || !logFile.is_open()) {
        return;
    }
    
    std::wstring timestamp = GetCurrentTimestamp();
    std::wstring levelStr = LogLevelToString(level);
    
    logFile << L"[" << timestamp << L"] [" << levelStr << L"] " << message << std::endl;
    logFile.flush();
}

void Logger::LogFileOperation(const FileOperationInfo& fileOp) {
    std::wstring operationStr;
    switch (fileOp.operation) {
        case FILE_CREATE: operationStr = L"CREATE"; break;
        case FILE_WRITE: operationStr = L"WRITE"; break;
        case FILE_RENAME: operationStr = L"RENAME"; break;
        case FILE_DELETE: operationStr = L"DELETE"; break;
    }
    
    std::wstringstream ss;
    ss << L"FILE_OPERATION: " << operationStr << L" | Process: " << fileOp.processName 
       << L" (PID: " << fileOp.processId << L") | File: " << fileOp.filePath;
    
    Log(LOG_WARNING, ss.str());
}

void Logger::LogUSBEvent(const std::wstring& event, const USBDeviceInfo& device) {
    std::wstringstream ss;
    ss << L"USB_EVENT: " << event << L" | Device: " << device.friendlyName 
       << L" | Drive: " << device.driveLetter << L" | Path: " << device.devicePath;
    
    Log(LOG_INFO, ss.str());
}

std::wstring Logger::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::wstringstream ss;
    struct tm timeinfo;
    if (localtime_s(&timeinfo, &time_t) == 0) {
        ss << std::put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S");
    } else {
        ss << L"1970-01-01 00:00:00";
    }
    ss << L"." << std::setfill(L'0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::wstring Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LOG_INFO: return L"INFO";
        case LOG_WARNING: return L"WARNING";
        case LOG_ERROR: return L"ERROR";
        default: return L"UNKNOWN";
    }
}
