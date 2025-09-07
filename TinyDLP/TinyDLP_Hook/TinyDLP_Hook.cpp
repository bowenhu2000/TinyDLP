#include "TinyDLP_Hook.h"
#include <algorithm>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "detours.lib")

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
        LogMessage(LOG_INFO, L"Ready to intercept file operations for PDF blocking");
        
        // Install hooks using Detours
        if (InstallHooks()) {
            g_isHooked = true;
            LogMessage(LOG_INFO, L"Detours hooks installed successfully - API interception is ACTIVE");
        } else {
            LogMessage(LOG_ERROR, L"Failed to install Detours hooks - API interception is NOT ACTIVE");
        }
        break;
    }
        
    case DLL_PROCESS_DETACH: {
        if (g_isHooked) {
            UninstallHooks();
            LogMessage(LOG_INFO, L"Detours hooks uninstalled");
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

void LogMessage(LogLevel level, const std::wstring& message) {
    if (!g_logFile.is_open()) {
        return;
    }
    
    std::wstring timestamp = GetCurrentTimestamp();
    std::wstring levelStr;
    
    switch (level) {
    case LOG_INFO:
        levelStr = L"INFO";
        break;
    case LOG_WARNING:
        levelStr = L"WARNING";
        break;
    case LOG_ERROR:
        levelStr = L"ERROR";
        break;
    }
    
    g_logFile << L"[" << timestamp << L"] [" << levelStr << L"] " << message << std::endl;
    g_logFile.flush();
}

std::wstring GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::wstringstream ss;
    std::tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S");
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
    
    LogMessage(LOG_WARNING, L"=== PDF SAVE TO USB BLOCKED ===");
    LogMessage(LOG_WARNING, L"Operation: " + operation);
    LogMessage(LOG_WARNING, L"Process: " + processName + L" (PID: " + std::to_wstring(processId) + L")");
    LogMessage(LOG_WARNING, L"File: " + filePath);
    LogMessage(LOG_WARNING, L"Reason: PDF file save to USB drive blocked by TinyDLP");
    
    // Show message box to user
    std::wstring message = L"TinyDLP has blocked an attempt to save a PDF file to a USB drive.\n\n";
    message += L"Process: " + processName + L"\n";
    message += L"File: " + filePath + L"\n";
    message += L"Operation: " + operation;
    
    MessageBoxW(NULL, message.c_str(), L"TinyDLP - PDF Save Blocked",
        MB_ICONWARNING | MB_OK | MB_TOPMOST);
}

bool InstallHooks() {
    LogMessage(LOG_INFO, L"Starting Detours hook installation...");
    
    // Initialize Detours
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // Get original function addresses
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        LogMessage(LOG_ERROR, L"Failed to get kernel32.dll module handle");
        DetourTransactionAbort();
        return false;
    }
    
    // Hook CreateFileW
    OriginalCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
    if (OriginalCreateFileW) {
        DetourAttach(&(PVOID&)OriginalCreateFileW, HookedCreateFileW);
        LogMessage(LOG_INFO, L"CreateFileW hook attached");
    } else {
        LogMessage(LOG_ERROR, L"Failed to get CreateFileW address");
    }
    
    // Hook WriteFile
    OriginalWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");
    if (OriginalWriteFile) {
        DetourAttach(&(PVOID&)OriginalWriteFile, HookedWriteFile);
        LogMessage(LOG_INFO, L"WriteFile hook attached");
    } else {
        LogMessage(LOG_ERROR, L"Failed to get WriteFile address");
    }
    
    // Hook CopyFileW
    OriginalCopyFileW = (CopyFileW_t)GetProcAddress(hKernel32, "CopyFileW");
    if (OriginalCopyFileW) {
        DetourAttach(&(PVOID&)OriginalCopyFileW, HookedCopyFileW);
        LogMessage(LOG_INFO, L"CopyFileW hook attached");
    } else {
        LogMessage(LOG_ERROR, L"Failed to get CopyFileW address");
    }
    
    // Hook MoveFileW
    OriginalMoveFileW = (MoveFileW_t)GetProcAddress(hKernel32, "MoveFileW");
    if (OriginalMoveFileW) {
        DetourAttach(&(PVOID&)OriginalMoveFileW, HookedMoveFileW);
        LogMessage(LOG_INFO, L"MoveFileW hook attached");
    } else {
        LogMessage(LOG_ERROR, L"Failed to get MoveFileW address");
    }
    
    // Commit the transaction
    LONG result = DetourTransactionCommit();
    if (result == NO_ERROR) {
        LogMessage(LOG_INFO, L"All Detours hooks installed successfully - API interception is ACTIVE");
        return true;
    } else {
        LogMessage(LOG_ERROR, L"Failed to commit Detours transaction. Error: " + std::to_wstring(result));
        DetourTransactionAbort();
        return false;
    }
}

void UninstallHooks() {
    LogMessage(LOG_INFO, L"Uninstalling Detours hooks...");
    
    // Begin transaction
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // Detach all hooks
    if (OriginalCreateFileW) {
        DetourDetach(&(PVOID&)OriginalCreateFileW, HookedCreateFileW);
    }
    if (OriginalWriteFile) {
        DetourDetach(&(PVOID&)OriginalWriteFile, HookedWriteFile);
    }
    if (OriginalCopyFileW) {
        DetourDetach(&(PVOID&)OriginalCopyFileW, HookedCopyFileW);
    }
    if (OriginalMoveFileW) {
        DetourDetach(&(PVOID&)OriginalMoveFileW, HookedMoveFileW);
    }
    
    // Commit the transaction
    DetourTransactionCommit();
    
    // Clean up any resources
    EnterCriticalSection(&g_cs);
    g_fileHandleMap.clear();
    LeaveCriticalSection(&g_cs);
    
    LogMessage(LOG_INFO, L"Detours hooks uninstalled");
}

// Hooked Functions Implementation
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
        LogMessage(LOG_INFO, L"CreateFileW called for: " + filePath);
        
        // Check if this is a PDF file being created on a USB drive
        if (IsPDFFile(filePath) && IsUSBDrive(filePath)) {
            LogMessage(LOG_WARNING, L"PDF file creation attempt detected on USB drive: " + filePath);
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
        LogMessage(LOG_WARNING, L"PDF file write attempt detected on USB drive: " + filePath);
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
        LogMessage(LOG_INFO, L"CopyFileW called for: " + filePath);
        
        if (IsPDFFile(filePath) && IsUSBDrive(filePath)) {
            LogMessage(LOG_WARNING, L"PDF file copy attempt detected on USB drive: " + filePath);
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
        LogMessage(LOG_INFO, L"MoveFileW called for: " + filePath);
        
        if (IsPDFFile(filePath) && IsUSBDrive(filePath)) {
            LogMessage(LOG_WARNING, L"PDF file move attempt detected on USB drive: " + filePath);
            BlockFileOperation(filePath, L"MOVE_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    
    // Call original function
    return OriginalMoveFileW(lpExistingFileName, lpNewFileName);
}
