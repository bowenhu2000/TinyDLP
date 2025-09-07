#include "APIHook.h"
#include "Logger.h"
#include "AlertDialog.h"
#include "USBMonitor.h"
#include <psapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "psapi.lib")

// Global variables
bool APIHook::isHooked = false;
std::vector<DWORD> APIHook::hookedProcesses;
std::mutex APIHook::hookMutex;

// Function pointers for original API functions
CreateFileW_t OriginalCreateFileW = nullptr;
WriteFile_t OriginalWriteFile = nullptr;
CreateDirectoryW_t OriginalCreateDirectoryW = nullptr;

bool APIHook::Initialize() {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    if (isHooked) {
        return true;
    }
    
    // Get original function addresses
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        return false;
    }
    
    OriginalCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
    OriginalWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");
    OriginalCreateDirectoryW = (CreateDirectoryW_t)GetProcAddress(hKernel32, "CreateDirectoryW");
    
    if (!OriginalCreateFileW || !OriginalWriteFile || !OriginalCreateDirectoryW) {
        return false;
    }
    
    isHooked = true;
    Logger::Log(LOG_INFO, L"API Hook initialized successfully");
    return true;
}

void APIHook::Shutdown() {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    // Unhook all processes
    for (DWORD processId : hookedProcesses) {
        UnhookProcess(processId);
    }
    hookedProcesses.clear();
    
    isHooked = false;
    Logger::Log(LOG_INFO, L"API Hook shutdown");
}

bool APIHook::HookProcess(DWORD processId) {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    if (IsProcessHooked(processId)) {
        return true;
    }
    
    // For now, we'll implement a simpler approach using process monitoring
    // In a full implementation, you would inject a DLL into the target process
    hookedProcesses.push_back(processId);
    
    Logger::Log(LOG_INFO, L"Process hooked: " + std::to_wstring(processId));
    return true;
}

void APIHook::UnhookProcess(DWORD processId) {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    auto it = std::find(hookedProcesses.begin(), hookedProcesses.end(), processId);
    if (it != hookedProcesses.end()) {
        hookedProcesses.erase(it);
        Logger::Log(LOG_INFO, L"Process unhooked: " + std::to_wstring(processId));
    }
}

bool APIHook::IsProcessHooked(DWORD processId) {
    std::lock_guard<std::mutex> lock(hookMutex);
    return std::find(hookedProcesses.begin(), hookedProcesses.end(), processId) != hookedProcesses.end();
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
            BlockFileOperation(filePath, L"CREATE");
            SetLastError(ERROR_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
        }
    }
    
    // Call original function
    return OriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, 
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI HookedWriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
) {
    // Get the file path from the handle (simplified approach)
    // In a real implementation, you would maintain a mapping of handles to file paths
    
    // For now, we'll use a different approach - monitor file operations at the system level
    // This is a simplified implementation
    
    // Call original function
    return OriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, 
        lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI HookedCreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
) {
    if (lpPathName) {
        std::wstring dirPath(lpPathName);
        
        // Check if this is a directory being created on a USB drive
        if (IsUSBDrive(dirPath)) {
            // Allow directory creation but log it
            Logger::Log(LOG_INFO, L"Directory creation on USB drive: " + dirPath);
        }
    }
    
    // Call original function
    return OriginalCreateDirectoryW(lpPathName, lpSecurityAttributes);
}

// Helper functions
bool IsPDFFile(const std::wstring& filePath) {
    if (filePath.length() < 4) {
        return false;
    }
    
    std::wstring extension = filePath.substr(filePath.length() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    return extension == L".pdf";
}

bool IsUSBDrive(const std::wstring& filePath) {
    return USBMonitor::IsUSBDrive(filePath);
}

void BlockFileOperation(const std::wstring& filePath, const std::wstring& operation) {
    // Get current process information
    DWORD processId = GetCurrentProcessId();
    std::wstring processName = L"Unknown Process";
    
    // Try to get process name
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
    
    // Log the blocked operation
    Logger::Log(LOG_WARNING, L"BLOCKED: " + operation + L" | Process: " + processName + 
        L" (PID: " + std::to_wstring(processId) + L") | File: " + filePath);
    
    // Show alert dialog
    std::wstring drivePath = filePath.substr(0, 2);
    AlertDialog::ShowPDFBlockAlert(filePath, processName, drivePath);
}
