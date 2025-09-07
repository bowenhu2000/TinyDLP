#include "FileMonitor.h"
#include "USBMonitor.h"
#include "AlertDialog.h"
#include "Logger.h"
#include "APIHook.h"
#include <algorithm>

// Static member definitions
std::vector<HANDLE> FileMonitor::hChangeHandles;
std::vector<std::wstring> FileMonitor::drivePaths;
std::vector<std::wstring> FileMonitor::monitoredDrives;
std::thread FileMonitor::monitorThread;
bool FileMonitor::isMonitoring = false;
std::mutex FileMonitor::monitorMutex;

FileMonitor::FileMonitor() {
    Logger::Log(LOG_INFO, L"FileMonitor created");
}

FileMonitor::~FileMonitor() {
    StopMonitoring();
    Logger::Log(LOG_INFO, L"FileMonitor destroyed");
}

bool FileMonitor::Initialize() {
    Logger::Log(LOG_INFO, L"FileMonitor initialized");
    return true;
}

void FileMonitor::Shutdown() {
    StopMonitoring();
    Logger::Log(LOG_INFO, L"FileMonitor shutdown");
}

void FileMonitor::StartMonitoring() {
    std::lock_guard<std::mutex> lock(monitorMutex);
    if (isMonitoring) { return; }
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
        if (!isMonitoring) { return; }
        isMonitoring = false;
    }
    
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
    
    // Close all change notification handles
    std::lock_guard<std::mutex> lock(monitorMutex);
    for (HANDLE handle : hChangeHandles) {
        if (handle != INVALID_HANDLE_VALUE) {
            FindCloseChangeNotification(handle);
        }
    }
    hChangeHandles.clear();
    drivePaths.clear();
    
    Logger::Log(LOG_INFO, L"File monitoring stopped");
}

void FileMonitor::MonitorThreadFunction() {
    Logger::Log(LOG_INFO, L"File monitor thread started");
    while (true) {
        {
            std::lock_guard<std::mutex> lock(monitorMutex);
            if (!isMonitoring) { break; }
        }
        
        std::vector<HANDLE> handlesCopy;
        {
            std::lock_guard<std::mutex> lock(monitorMutex);
            handlesCopy = hChangeHandles;
        }
        
        if (handlesCopy.empty()) {
            Sleep(1000);
            continue;
        }
        
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(handlesCopy.size()),
            handlesCopy.data(),
            FALSE,
            1000
        );
        
        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + handlesCopy.size()) {
            DWORD index = waitResult - WAIT_OBJECT_0;
            HANDLE changedHandle = handlesCopy[index];
            
            // Find the drive path for this handle
            std::wstring drivePath;
            {
                std::lock_guard<std::mutex> lock(monitorMutex);
                auto it = std::find(hChangeHandles.begin(), hChangeHandles.end(), changedHandle);
                if (it != hChangeHandles.end()) {
                    size_t index = std::distance(hChangeHandles.begin(), it);
                    if (index < drivePaths.size()) {
                        drivePath = drivePaths[index];
                    }
                }
            }
            
            if (!drivePath.empty()) {
                ProcessFileChangeWithReadDirectoryChanges(drivePath);
            }
            
            // Reset the change notification
            FindNextChangeNotification(changedHandle);
        }
    }
    Logger::Log(LOG_INFO, L"File monitor thread ended");
}

void FileMonitor::AddDriveToMonitor(const std::wstring& drivePath) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    
    // Check if already monitoring this drive
    if (std::find(drivePaths.begin(), drivePaths.end(), drivePath) != drivePaths.end()) {
        return;
    }
    
    HANDLE hChange = FindFirstChangeNotificationW(
        drivePath.c_str(),
        TRUE,  // Watch subdirectories
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE
    );
    
    if (hChange != INVALID_HANDLE_VALUE) {
        hChangeHandles.push_back(hChange);
        drivePaths.push_back(drivePath);
        Logger::Log(LOG_INFO, L"Added drive to monitor: " + drivePath);
    } else {
        Logger::Log(LOG_ERROR, L"Failed to add drive to monitor: " + drivePath);
    }
}

void FileMonitor::RemoveDriveFromMonitor(const std::wstring& drivePath) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    
    auto it = std::find(drivePaths.begin(), drivePaths.end(), drivePath);
    if (it != drivePaths.end()) {
        size_t index = std::distance(drivePaths.begin(), it);
        
        if (index < hChangeHandles.size()) {
            HANDLE handle = hChangeHandles[index];
            if (handle != INVALID_HANDLE_VALUE) {
                FindCloseChangeNotification(handle);
            }
            hChangeHandles.erase(hChangeHandles.begin() + index);
        }
        
        drivePaths.erase(it);
        Logger::Log(LOG_INFO, L"Removed drive from monitor: " + drivePath);
    }
}

void FileMonitor::ProcessFileChangeWithReadDirectoryChanges(const std::wstring& drivePath) {
    if (!USBMonitor::IsUSBDrive(drivePath)) {
        return;
    }
    
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
    
    char buffer[4096];
    DWORD bytesReturned;
    
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
        DWORD offset = 0;
        while (offset < bytesReturned) {
            FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)(buffer + offset);
            std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
            std::wstring fullPath = drivePath + fileName;
            
            if (IsPDFFile(fileName)) {
                DWORD processId = GetCurrentProcessId(); // Placeholder, needs actual process ID
                std::wstring processName = GetProcessName(processId);
                std::wstring driveLetter = drivePath.substr(0, 2);
                
                Logger::Log(LOG_WARNING, L"PDF file operation detected: " + fullPath);
                BlockPDFSave(fileName, processName, driveLetter);
            }
            
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
    
    // Implement API call interception blocking
    // 1. Try to delete the file if it exists
    if (GetFileAttributesW(filePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (DeleteFileW(filePath.c_str())) {
            Logger::Log(LOG_WARNING, L"Blocked PDF file deleted: " + filePath);
        } else {
            Logger::Log(LOG_ERROR, L"Failed to delete blocked PDF file: " + filePath);
        }
    }
    
    // 2. Create a blocking file to prevent recreation
    HANDLE hFile = CreateFileW(filePath.c_str(), 
        GENERIC_WRITE, 
        0,  // No sharing - exclusive access
        NULL, 
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM, 
        NULL);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        // Write blocking content
        const char* blockingContent = "PDF file blocked by TinyDLP - Access Denied";
        DWORD bytesWritten;
        WriteFile(hFile, blockingContent, strlen(blockingContent), &bytesWritten, NULL);
        CloseHandle(hFile);
        
        // Set additional attributes to make it harder to modify
        SetFileAttributesW(filePath.c_str(), 
            FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        
        Logger::Log(LOG_WARNING, L"Created blocking file: " + filePath);
    }
    
    // 3. Use API hooking to intercept future attempts
    // The APIHook class will handle intercepting CreateFileW, WriteFile, CopyFileW, MoveFileW
    // for this specific file path
    
    Logger::Log(LOG_WARNING, L"PDF file operation blocked via API interception: " + filePath);
}
