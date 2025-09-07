#pragma once

#include "Common.h"

class FileMonitor {
private:
    static std::vector<HANDLE> hChangeHandles;
    static std::vector<std::wstring> drivePaths;
    static std::vector<std::wstring> monitoredDrives;
    static std::thread monitorThread;
    static bool isMonitoring;
    static std::mutex monitorMutex;

public:
    FileMonitor();
    ~FileMonitor();
    static bool Initialize();
    static void Shutdown();
    static void StartMonitoring();
    static void StopMonitoring();
    static void AddDriveToMonitor(const std::wstring& drivePath);
    static void RemoveDriveFromMonitor(const std::wstring& drivePath);
    
private:
    static void MonitorThreadFunction();
    static void ProcessFileChange(const std::wstring& filePath, DWORD action);
    static void ProcessFileChangeWithReadDirectoryChanges(const std::wstring& drivePath);
    static bool IsPDFFile(const std::wstring& filePath);
    static std::wstring GetProcessName(DWORD processId);
    static std::wstring GetDriveFromPath(const std::wstring& filePath);
    static void BlockPDFSave(const std::wstring& filePath, const std::wstring& processName, const std::wstring& drivePath);
};
