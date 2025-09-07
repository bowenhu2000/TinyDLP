#include "APIHook.h"
#include "Logger.h"
#include "AlertDialog.h"
#include "USBMonitor.h"
#include <psapi.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

// Static member definitions
bool APIHook::isInitialized = false;
std::mutex APIHook::hookMutex;
std::map<HANDLE, std::wstring> APIHook::fileHandleMap;

// Original function pointers
CreateFileW_t APIHook::OriginalCreateFileW = nullptr;
WriteFile_t APIHook::OriginalWriteFile = nullptr;
CopyFileW_t APIHook::OriginalCopyFileW = nullptr;
MoveFileW_t APIHook::OriginalMoveFileW = nullptr;

bool APIHook::Initialize() {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    if (isInitialized) {
        return true;
    }
    
    // Get original function addresses
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        Logger::Log(LOG_ERROR, L"Failed to get kernel32.dll handle");
        return false;
    }
    
    OriginalCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
    OriginalWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");
    OriginalCopyFileW = (CopyFileW_t)GetProcAddress(hKernel32, "CopyFileW");
    OriginalMoveFileW = (MoveFileW_t)GetProcAddress(hKernel32, "MoveFileW");
    
    if (!OriginalCreateFileW || !OriginalWriteFile || !OriginalCopyFileW || !OriginalMoveFileW) {
        Logger::Log(LOG_ERROR, L"Failed to get original API function addresses");
        return false;
    }
    
    isInitialized = true;
    Logger::Log(LOG_INFO, L"API Hook initialized successfully");
    return true;
}

void APIHook::Shutdown() {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    if (!isInitialized) {
        return;
    }
    
    fileHandleMap.clear();
    isInitialized = false;
    Logger::Log(LOG_INFO, L"API Hook shutdown");
}

bool APIHook::IsFileBlocked(const std::wstring& filePath) {
    return IsPDFFile(filePath) && IsUSBDrive(filePath);
}

void APIHook::BlockFileOperation(const std::wstring& filePath, const std::wstring& operation) {
    DWORD processId = GetCurrentProcessId();
    std::wstring processName = GetProcessName(processId);
    
    // Log the blocked operation
    Logger::Log(LOG_WARNING, L"BLOCKED: " + operation + L" | Process: " + processName + 
        L" (PID: " + std::to_wstring(processId) + L") | File: " + filePath);
    
    // Show alert dialog
    std::wstring drivePath = filePath.substr(0, 2);
    AlertDialog::ShowPDFBlockAlert(filePath, processName, drivePath);
}

std::wstring APIHook::GetProcessName(DWORD processId) {
    std::wstring processName = L"Unknown Process";
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processNameBuffer[MAX_PATH];
        DWORD size = MAX_PATH;
        
        if (QueryFullProcessImageNameW(hProcess, 0, processNameBuffer, &size)) {
            std::wstring fullPath(processNameBuffer);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                processName = fullPath.substr(lastSlash + 1);
            } else {
                processName = fullPath;
            }
        }
        
        CloseHandle(hProcess);
    }
    
    return processName;
}

bool APIHook::IsPDFFile(const std::wstring& filePath) {
    if (filePath.length() < 4) {
        return false;
    }
    
    std::wstring extension = filePath.substr(filePath.length() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    return extension == L".pdf";
}

bool APIHook::IsUSBDrive(const std::wstring& filePath) {
    return USBMonitor::IsUSBDrive(filePath);
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
        if (APIHook::IsFileBlocked(filePath)) {
            // Block the operation
            APIHook::BlockFileOperation(filePath, L"CREATE_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
        }
    }
    
    // Call original function
    HANDLE result = APIHook::OriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, 
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    
    // If successful and it's a file we want to monitor, track the handle
    if (result != INVALID_HANDLE_VALUE && lpFileName) {
        std::wstring filePath(lpFileName);
        if (APIHook::IsPDFFile(filePath)) {
            std::lock_guard<std::mutex> lock(APIHook::hookMutex);
            APIHook::fileHandleMap[result] = filePath;
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
    {
        std::lock_guard<std::mutex> lock(APIHook::hookMutex);
        auto it = APIHook::fileHandleMap.find(hFile);
        if (it != APIHook::fileHandleMap.end()) {
            filePath = it->second;
        }
    }
    
    if (!filePath.empty() && APIHook::IsFileBlocked(filePath)) {
        // Block the write operation
        APIHook::BlockFileOperation(filePath, L"WRITE_FILE");
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    
    // Call original function
    return APIHook::OriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, 
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
        if (APIHook::IsFileBlocked(filePath)) {
            // Block the operation
            APIHook::BlockFileOperation(filePath, L"COPY_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    
    // Call original function
    return APIHook::OriginalCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

BOOL WINAPI HookedMoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
) {
    if (lpNewFileName) {
        std::wstring filePath(lpNewFileName);
        
        // Check if this is a PDF file being moved to a USB drive
        if (APIHook::IsFileBlocked(filePath)) {
            // Block the operation
            APIHook::BlockFileOperation(filePath, L"MOVE_FILE");
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    
    // Call original function
    return APIHook::OriginalMoveFileW(lpExistingFileName, lpNewFileName);
}

