#include "TinyDLP_Hook.h"
#include <algorithm>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

// Global variables
CreateFileW_t OriginalCreateFileW = nullptr;
WriteFile_t OriginalWriteFile = nullptr;
CopyFileW_t OriginalCopyFileW = nullptr;
MoveFileW_t OriginalMoveFileW = nullptr;

std::wofstream g_logFile;
bool g_isHooked = false;

// Handle to file path mapping for tracking
std::map<HANDLE, std::wstring> g_fileHandleMap;
CRITICAL_SECTION g_cs;

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        // Initialize critical section
        InitializeCriticalSection(&g_cs);
        
        // Open log file
        g_logFile.open(LOG_FILE_PATH, std::ios::app);
        if (g_logFile.is_open()) {
            g_logFile.imbue(std::locale("C"));
        }
        
        // Log DLL injection success with detailed information
        std::wstring processName = GetProcessName();
        DWORD processId = GetCurrentProcessId();
        std::wstring timestamp = GetCurrentTimestamp();
        
        LogMessage(LOG_INFO, L"=== TinyDLP Hook DLL Successfully Injected ===");
        LogMessage(LOG_INFO, L"Target Process: " + processName);
        LogMessage(LOG_INFO, L"Process ID: " + std::to_wstring(processId));
        LogMessage(LOG_INFO, L"Injection Time: " + timestamp);
        LogMessage(LOG_INFO, L"DLL Path: " + std::wstring(LOG_FILE_PATH));
        LogMessage(LOG_INFO, L"Ready to intercept file operations for PDF blocking");
        
        // Install hooks
        if (InstallHooks()) {
            g_isHooked = true;
            LogMessage(LOG_INFO, L"Hooks installed successfully");
        } else {
            LogMessage(LOG_ERROR, L"Failed to install hooks");
        }
        break;
    }
        
    case DLL_PROCESS_DETACH: {
        if (g_isHooked) {
            UninstallHooks();
            LogMessage(LOG_INFO, L"Hooks uninstalled");
        }
        
        if (g_logFile.is_open()) {
            g_logFile.close();
        }
        
        DeleteCriticalSection(&g_cs);
        LogMessage(LOG_INFO, L"DLL unloaded from process");
        break;
    }
    }
    return TRUE;
}


// Logging functions
void LogMessage(LogLevel level, const std::wstring& message) {
    if (!g_logFile.is_open()) return;
    
    std::wstring timestamp = GetCurrentTimestamp();
    std::wstring levelStr;
    
    switch (level) {
    case LOG_INFO: levelStr = L"INFO"; break;
    case LOG_WARNING: levelStr = L"WARN"; break;
    case LOG_ERROR: levelStr = L"ERROR"; break;
    }
    
    try {
        g_logFile << L"[" << timestamp << L"] [" << levelStr << L"] " << message << std::endl;
        g_logFile.flush();
    } catch (const std::exception&) {
        // If logging fails, continue silently
    }
}

std::wstring GetCurrentTimestamp() {
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

bool IsPDFFile(const std::wstring& filePath) {
    if (filePath.length() < 4) {
        return false;
    }
    
    std::wstring extension = filePath.substr(filePath.length() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    return extension == L".pdf";
}

bool IsUSBDrive(const std::wstring& filePath) {
    if (filePath.length() < 2) {
        return false;
    }
    
    // Extract drive letter (e.g., "C:" from "C:\path\to\file")
    std::wstring drive = filePath.substr(0, 2);
    
    // Check if it's a removable drive
    UINT driveType = GetDriveTypeW(drive.c_str());
    return (driveType == DRIVE_REMOVABLE);
}

std::wstring GetProcessName() {
    wchar_t processName[MAX_PATH];
    DWORD size = MAX_PATH;
    
    if (QueryFullProcessImageNameW(GetCurrentProcess(), 0, processName, &size)) {
        std::wstring fullPath(processName);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            return fullPath.substr(lastSlash + 1);
        }
        return fullPath;
    }
    
    return L"Unknown Process";
}

void BlockFileOperation(const std::wstring& filePath, const std::wstring& operation) {
    std::wstring processName = GetProcessName();
    DWORD processId = GetCurrentProcessId();
    
    LogMessage(LOG_WARNING, L"BLOCKED: " + operation + L" | Process: " + processName + 
        L" (PID: " + std::to_wstring(processId) + L") | File: " + filePath);
    
    // Show message box to user
    std::wstring message = L"TinyDLP has blocked an attempt to save a PDF file to a USB drive.\\n\\n";
    message += L"Process: " + processName + L"\\n";
    message += L"File: " + filePath + L"\\n";
    message += L"Operation: " + operation;
    
    MessageBoxW(NULL, message.c_str(), L"TinyDLP - PDF Save Blocked", 
        MB_ICONWARNING | MB_OK | MB_TOPMOST);
}

// Hook installation functions
bool InstallHooks() {
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        return false;
    }
    
    // Get original function addresses
    OriginalCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
    OriginalWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");
    OriginalCopyFileW = (CopyFileW_t)GetProcAddress(hKernel32, "CopyFileW");
    OriginalMoveFileW = (MoveFileW_t)GetProcAddress(hKernel32, "MoveFileW");
    
    if (!OriginalCreateFileW || !OriginalWriteFile || !OriginalCopyFileW || !OriginalMoveFileW) {
        return false;
    }
    
    // For this implementation, we'll use a simple approach
    // In a production system, you would use Microsoft Detours or similar library
    // to properly hook the functions
    
    return true;
}

void UninstallHooks() {
    // Clean up any resources
    EnterCriticalSection(&g_cs);
    g_fileHandleMap.clear();
    LeaveCriticalSection(&g_cs);
}

// Hooked function implementations
HANDLE WINAPI HookedCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
) {
    if (lpFileName) {
        std::wstring filePath(lpFileName);
        
        // Check if this is a PDF file being created/written to a USB drive
        if (IsPDFFile(filePath) && IsUSBDrive(filePath)) {
            // Block the operation
            BlockFileOperation(filePath, L"CREATE_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
        }
    }
    
    // Call original function
    HANDLE result = OriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, 
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    
    // If successful and it's a file we want to monitor, track the handle
    if (result != INVALID_HANDLE_VALUE && lpFileName) {
        std::wstring filePath(lpFileName);
        if (IsPDFFile(filePath)) {
            EnterCriticalSection(&g_cs);
            g_fileHandleMap[result] = filePath;
            LeaveCriticalSection(&g_cs);
        }
    }
    
    return result;
}

BOOL WINAPI HookedWriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
) {
    // Check if this handle is for a blocked file
    std::wstring filePath;
    EnterCriticalSection(&g_cs);
    auto it = g_fileHandleMap.find(hFile);
    if (it != g_fileHandleMap.end()) {
        filePath = it->second;
    }
    LeaveCriticalSection(&g_cs);
    
    if (!filePath.empty() && IsPDFFile(filePath) && IsUSBDrive(filePath)) {
        // Block the write operation
        BlockFileOperation(filePath, L"WRITE_FILE");
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    
    // Call original function
    return OriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, 
        lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI HookedCopyFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
) {
    if (lpNewFileName) {
        std::wstring filePath(lpNewFileName);
        
        // Check if this is a PDF file being copied to a USB drive
        if (IsPDFFile(filePath) && IsUSBDrive(filePath)) {
            // Block the operation
            BlockFileOperation(filePath, L"COPY_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    
    // Call original function
    return OriginalCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

BOOL WINAPI HookedMoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
) {
    if (lpNewFileName) {
        std::wstring filePath(lpNewFileName);
        
        // Check if this is a PDF file being moved to a USB drive
        if (IsPDFFile(filePath) && IsUSBDrive(filePath)) {
            // Block the operation
            BlockFileOperation(filePath, L"MOVE_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    
    // Call original function
    return OriginalMoveFileW(lpExistingFileName, lpNewFileName);
}




