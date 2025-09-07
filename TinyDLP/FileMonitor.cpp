#include "FileMonitor.h"
#include "USBMonitor.h"
#include "AlertDialog.h"
#include "Logger.h"
#include <psapi.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

std::vector<HANDLE> FileMonitor::hChangeHandles;
std::vector<std::wstring> FileMonitor::monitoredDrives;
std::thread FileMonitor::monitorThread;
bool FileMonitor::isMonitoring = false;
std::mutex FileMonitor::monitorMutex;

bool FileMonitor::Initialize() {
    std::lock_guard<std::mutex> lock(monitorMutex);
    
    // Initialize with all removable drives
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            wchar_t driveLetter = L'A' + i;
            std::wstring drivePath = std::wstring(1, driveLetter) + L":\\";
            
            UINT driveType = GetDriveTypeW(drivePath.c_str());
            if (driveType == DRIVE_REMOVABLE) {
                AddDriveToMonitor(drivePath);
            }
        }
    }
    
    Logger::Log(LOG_INFO, L"File Monitor initialized");
    return true;
}

void FileMonitor::Shutdown() {
    StopMonitoring();
    
    std::lock_guard<std::mutex> lock(monitorMutex);
    
    // Close all change handles
    for (HANDLE hHandle : hChangeHandles) {
        if (hHandle != INVALID_HANDLE_VALUE) {
            FindCloseChangeNotification(hHandle);
        }
    }
    hChangeHandles.clear();
    monitoredDrives.clear();
    
    Logger::Log(LOG_INFO, L"File Monitor shutdown");
}

void FileMonitor::StartMonitoring() {
    std::lock_guard<std::mutex> lock(monitorMutex);
    
    if (isMonitoring) {
        return;
    }
    
    isMonitoring = true;
    try {
        monitorThread = std::thread(MonitorThreadFunction);
        Logger::Log(LOG_INFO, L"File monitoring started");
    } catch (const std::exception&) {
        isMonitoring = false;
        Logger::Log(LOG_ERROR, L"Failed to start file monitoring thread");
        return;
    }
}

void FileMonitor::StopMonitoring() {
    {
        std::lock_guard<std::mutex> lock(monitorMutex);
        isMonitoring = false;
    }
    
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
    
    Logger::Log(LOG_INFO, L"File monitoring stopped");
}

void FileMonitor::AddDriveToMonitor(const std::wstring& drivePath) {
    //std::lock_guard<std::mutex> lock(monitorMutex);
    
    // Check if already monitoring this drive
    if (std::find(monitoredDrives.begin(), monitoredDrives.end(), drivePath) != monitoredDrives.end()) {
        return;
    }
    
    HANDLE hChange = FindFirstChangeNotificationW(drivePath.c_str(), TRUE, 
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE);
    
    if (hChange != INVALID_HANDLE_VALUE) {
        hChangeHandles.push_back(hChange);
        monitoredDrives.push_back(drivePath);
        Logger::Log(LOG_INFO, L"Added drive to monitoring: " + drivePath);
    }
}

void FileMonitor::RemoveDriveFromMonitor(const std::wstring& drivePath) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    
    auto it = std::find(monitoredDrives.begin(), monitoredDrives.end(), drivePath);
    if (it != monitoredDrives.end()) {
        size_t index = std::distance(monitoredDrives.begin(), it);
        
        if (index < hChangeHandles.size()) {
            FindCloseChangeNotification(hChangeHandles[index]);
            hChangeHandles.erase(hChangeHandles.begin() + index);
        }
        
        monitoredDrives.erase(it);
        Logger::Log(LOG_INFO, L"Removed drive from monitoring: " + drivePath);
    }
}

void FileMonitor::MonitorThreadFunction() {
    Logger::Log(LOG_INFO, L"File monitor thread started");
    
    while (true) {
        // Check if we should stop monitoring
        {
            std::lock_guard<std::mutex> lock(monitorMutex);
            if (!isMonitoring) {
                break;
            }
        }
        
        // Get a copy of handles under lock to avoid race conditions
        std::vector<HANDLE> handlesCopy;
        {
            std::lock_guard<std::mutex> lock(monitorMutex);
            handlesCopy = hChangeHandles;
        }
        
        // If no handles, sleep and continue
        if (handlesCopy.empty()) {
            Sleep(1000);
            continue;
        }
        
        // Wait for any change notification
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(handlesCopy.size()),
            handlesCopy.data(),
            FALSE,
            1000  // 1 second timeout
        );
        
        // Handle timeout - this is normal, just continue
        if (waitResult == WAIT_TIMEOUT) {
            continue;
        }
        
        // Handle error
        if (waitResult == WAIT_FAILED) {
            DWORD error = GetLastError();
            Logger::Log(LOG_ERROR, L"WaitForMultipleObjects failed with error: " + std::to_wstring(error));
            Sleep(1000);
            continue;
        }
        
        // Handle successful wait
        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + handlesCopy.size()) {
            DWORD index = waitResult - WAIT_OBJECT_0;
            
            std::lock_guard<std::mutex> lock(monitorMutex);
            if (index < monitoredDrives.size()) {
                std::wstring drivePath = monitoredDrives[index];
                
                // Process the change with actual file detection
                ProcessFileChangeWithReadDirectoryChanges(drivePath);
                
                // Continue monitoring
                if (index < hChangeHandles.size()) {
                    FindNextChangeNotification(hChangeHandles[index]);
                }
            }
        }
    }
    
    Logger::Log(LOG_INFO, L"File monitor thread ended");
}

void FileMonitor::ProcessFileChangeWithReadDirectoryChanges(const std::wstring& drivePath) {
    // Check if this is a USB drive
    if (!USBMonitor::IsUSBDrive(drivePath)) {
        return;
    }
    
    // Use ReadDirectoryChangesW to get actual file information
    HANDLE hDir = CreateFileW(
        drivePath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );
    
    if (hDir == INVALID_HANDLE_VALUE) {
        return;
    }
    
    // Buffer for file change notifications
    char buffer[4096];
    DWORD bytesReturned;
    
    // Read directory changes
    if (ReadDirectoryChangesW(
        hDir,
        buffer,
        sizeof(buffer),
        TRUE,  // Watch subdirectories
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE,
        &bytesReturned,
        NULL,
        NULL
    )) {
        // Parse the notifications
        DWORD offset = 0;
        while (offset < bytesReturned) {
            FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)(buffer + offset);
            
            // Convert the filename to wstring
            std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
            std::wstring fullPath = drivePath + fileName;
            
            // Check if this is a PDF file operation
            if (IsPDFFile(fileName)) {
                DWORD processId = GetCurrentProcessId();  // In real implementation, get from file operation
                std::wstring processName = GetProcessName(processId);
                std::wstring driveLetter = drivePath.substr(0, 2);
                
                Logger::Log(LOG_WARNING, L"PDF file operation detected: " + fullPath);
                BlockPDFSave(fileName, processName, driveLetter);
            }
            
            // Move to next notification
            if (pNotify->NextEntryOffset == 0) {
                break;
            }
            offset += pNotify->NextEntryOffset;
        }
    }
    
    CloseHandle(hDir);
}

void FileMonitor::ProcessFileChange(const std::wstring& filePath, DWORD action) {
    // Legacy function - now calls the proper implementation
    ProcessFileChangeWithReadDirectoryChanges(filePath);
}

bool FileMonitor::IsPDFFile(const std::wstring& filePath) {
    if (filePath.length() < 4) {
        return false;
    }
    
    std::wstring extension = filePath.substr(filePath.length() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    return extension == L".pdf";
}

std::wstring FileMonitor::GetProcessName(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) {
        return L"Unknown Process";
    }
    
    wchar_t processName[MAX_PATH];
    DWORD size = MAX_PATH;
    
    if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
        CloseHandle(hProcess);
        
        // Extract just the filename from the full path
        std::wstring fullPath(processName);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            return fullPath.substr(lastSlash + 1);
        }
        return fullPath;
    }
    
    CloseHandle(hProcess);
    return L"Unknown Process";
}

std::wstring FileMonitor::GetDriveFromPath(const std::wstring& filePath) {
    if (filePath.length() >= 2 && filePath[1] == L':') {
        return filePath.substr(0, 2);
    }
    return L"";
}

void FileMonitor::BlockPDFSave(const std::wstring& filePath, const std::wstring& processName, const std::wstring& drivePath) {
    // Log the blocked operation
    FileOperationInfo fileOp;
    fileOp.filePath = filePath;
    fileOp.processName = processName;
    fileOp.processId = GetCurrentProcessId();
    fileOp.operation = FILE_WRITE;
    fileOp.timestamp = std::chrono::system_clock::now();
    
    Logger::LogFileOperation(fileOp);
    
    // Show alert dialog
    AlertDialog::ShowPDFBlockAlert(filePath, processName, drivePath);
    
    // In a real implementation, you would also block the actual file operation
    // This could be done through file system filtering or by intercepting the API call
}

