#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>

// Detours library includes
#include "detours/detours.h"

// Common constants
#define LOG_FILE_PATH L"C:\\TinyDLP\\TinyDLP_Hook.log"
#define MAX_PATH_LENGTH 260

// Log levels
enum LogLevel { LOG_INFO, LOG_WARNING, LOG_ERROR };

// Function pointer types for original API functions
typedef HANDLE (WINAPI* CreateFileW_t)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef BOOL (WINAPI* WriteFile_t)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef BOOL (WINAPI* CopyFileW_t)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
typedef BOOL (WINAPI* MoveFileW_t)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
typedef BOOL (WINAPI* CopyFileExW_t)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags);
typedef BOOL (WINAPI* MoveFileExW_t)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);

// Global variables
extern CreateFileW_t OriginalCreateFileW;
extern WriteFile_t OriginalWriteFile;
extern CopyFileW_t OriginalCopyFileW;
extern MoveFileW_t OriginalMoveFileW;
extern CopyFileExW_t OriginalCopyFileExW;
extern MoveFileExW_t OriginalMoveFileExW;
extern std::wofstream g_logFile;
extern bool g_isHooked;
extern std::map<HANDLE, std::wstring> g_fileHandleMap;
extern CRITICAL_SECTION g_cs;

// Hooked function declarations
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
BOOL WINAPI HookedCopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
BOOL WINAPI HookedMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
BOOL WINAPI HookedCopyFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags);
BOOL WINAPI HookedMoveFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);

// Helper functions
void LogMessage(LogLevel level, const std::wstring& message);
std::wstring GetCurrentTimestamp();
bool IsPDFFile(const std::wstring& filePath);
bool IsUSBDrive(const std::wstring& filePath);
std::wstring GetProcessName();
void BlockFileOperation(const std::wstring& filePath, const std::wstring& operation);
bool InstallHooks();
void UninstallHooks();
