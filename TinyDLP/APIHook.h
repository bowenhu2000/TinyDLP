#pragma once

#include "Common.h"

class APIHook {
private:
    static bool isHooked;
    static std::vector<DWORD> hookedProcesses;
    static std::mutex hookMutex;

public:
    static bool Initialize();
    static void Shutdown();
    static bool HookProcess(DWORD processId);
    static void UnhookProcess(DWORD processId);
    static bool IsProcessHooked(DWORD processId);
    
private:
    static bool InjectDLL(DWORD processId, const std::wstring& dllPath);
    static std::wstring GetCurrentModulePath();
};

// Function pointer types for original API functions
typedef HANDLE (WINAPI* CreateFileW_t)(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
);

typedef BOOL (WINAPI* WriteFile_t)(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
);

typedef BOOL (WINAPI* CreateDirectoryW_t)(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

// Global function pointers
extern CreateFileW_t OriginalCreateFileW;
extern WriteFile_t OriginalWriteFile;
extern CreateDirectoryW_t OriginalCreateDirectoryW;

// Hooked function implementations
HANDLE WINAPI HookedCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
);

BOOL WINAPI HookedWriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
);

BOOL WINAPI HookedCreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

// Helper functions
bool IsPDFFile(const std::wstring& filePath);
bool IsUSBDrive(const std::wstring& filePath);
void BlockFileOperation(const std::wstring& filePath, const std::wstring& operation);
