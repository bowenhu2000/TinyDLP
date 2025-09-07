#include "USBMonitor.h"
#include "Logger.h"
#include <dbt.h>

std::vector<USBDeviceInfo> USBMonitor::usbDevices;
std::mutex USBMonitor::usbMutex;
bool USBMonitor::isMonitoring = false;
HANDLE USBMonitor::hDeviceNotify = NULL;
HWND USBMonitor::hWnd = NULL;

bool USBMonitor::Initialize(HWND windowHandle) {
    hWnd = windowHandle;
    
    // Register for device notifications
    DEV_BROADCAST_DEVICEINTERFACE_W dbdi = {};
    dbdi.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_W);
    dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dbdi.dbcc_classguid = GUID_DEVINTERFACE_DISK;
    
    hDeviceNotify = RegisterDeviceNotificationW(hWnd, &dbdi, DEVICE_NOTIFY_WINDOW_HANDLE);
    
    if (hDeviceNotify == NULL) {
        Logger::Log(LOG_ERROR, L"Failed to register for device notifications");
        return false;
    }
    
    // Enumerate existing USB devices
    EnumerateUSBDevices();
    
    Logger::Log(LOG_INFO, L"USB Monitor initialized successfully");
    return true;
}

void USBMonitor::Shutdown() {
    StopMonitoring();
    
    if (hDeviceNotify != NULL) {
        UnregisterDeviceNotification(hDeviceNotify);
        hDeviceNotify = NULL;
    }
    
    std::lock_guard<std::mutex> lock(usbMutex);
    usbDevices.clear();
    
    Logger::Log(LOG_INFO, L"USB Monitor shutdown");
}

void USBMonitor::StartMonitoring() {
    std::lock_guard<std::mutex> lock(usbMutex);
    isMonitoring = true;
    Logger::Log(LOG_INFO, L"USB monitoring started");
}

void USBMonitor::StopMonitoring() {
    std::lock_guard<std::mutex> lock(usbMutex);
    isMonitoring = false;
    Logger::Log(LOG_INFO, L"USB monitoring stopped");
}

std::vector<USBDeviceInfo> USBMonitor::GetUSBDevices() {
    std::lock_guard<std::mutex> lock(usbMutex);
    return usbDevices;
}

bool USBMonitor::IsUSBDrive(const std::wstring& drivePath) {
    std::lock_guard<std::mutex> lock(usbMutex);
    
    for (const auto& device : usbDevices) {
        // Compare drive letters properly (K: vs K:\)
        std::wstring deviceDrive = device.driveLetter.substr(0, 2);
        std::wstring inputDrive = drivePath.substr(0, 2);
        if (deviceDrive == inputDrive) {
            return true;
        }
    }
    return false;
}

LRESULT USBMonitor::HandleDeviceChange(WPARAM wParam, LPARAM lParam) {
    if (!isMonitoring) {
        return 0;
    }
    
    switch (wParam) {
        case DBT_DEVICEARRIVAL: {
            DEV_BROADCAST_HDR* pdbh = (DEV_BROADCAST_HDR*)lParam;
            if (pdbh->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                DEV_BROADCAST_VOLUME* pdbv = (DEV_BROADCAST_VOLUME*)lParam;
                DWORD driveMask = pdbv->dbcv_unitmask;
                
                for (int i = 0; i < 26; i++) {
                    if (driveMask & (1 << i)) {
                        wchar_t driveLetter = L'A' + i;
                        std::wstring drivePath = std::wstring(1, driveLetter) + L":\\";
                        
                        // Check if this is a removable drive
                        UINT driveType = GetDriveTypeW(drivePath.c_str());
                        if (driveType == DRIVE_REMOVABLE) {
                            USBDeviceInfo device;
                            device.driveLetter = drivePath;
                            device.devicePath = L"";
                            device.friendlyName = L"USB Drive " + std::wstring(1, driveLetter);
                            device.isRemovable = true;
                            
                            OnDeviceArrival(device.devicePath);
                        }
                    }
                }
            }
            break;
        }
        case DBT_DEVICEREMOVECOMPLETE: {
            DEV_BROADCAST_HDR* pdbh = (DEV_BROADCAST_HDR*)lParam;
            if (pdbh->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                DEV_BROADCAST_VOLUME* pdbv = (DEV_BROADCAST_VOLUME*)lParam;
                DWORD driveMask = pdbv->dbcv_unitmask;
                
                for (int i = 0; i < 26; i++) {
                    if (driveMask & (1 << i)) {
                        wchar_t driveLetter = L'A' + i;
                        std::wstring drivePath = std::wstring(1, driveLetter) + L":\\";
                        OnDeviceRemoval(drivePath);
                    }
                }
            }
            break;
        }
    }
    
    return 0;
}

void USBMonitor::EnumerateUSBDevices() {
    std::lock_guard<std::mutex> lock(usbMutex);
    usbDevices.clear();
    
    // Enumerate all drives
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            wchar_t driveLetter = L'A' + i;
            std::wstring drivePath = std::wstring(1, driveLetter) + L":\\";
            
            UINT driveType = GetDriveTypeW(drivePath.c_str());
            if (driveType == DRIVE_REMOVABLE) {
                USBDeviceInfo device;
                device.driveLetter = drivePath;
                device.devicePath = L"";
                device.friendlyName = L"USB Drive " + std::wstring(1, driveLetter);
                device.isRemovable = true;
                
                usbDevices.push_back(device);
                Logger::LogUSBEvent(L"ENUMERATED", device);
            }
        }
    }
}

void USBMonitor::OnDeviceArrival(const std::wstring& devicePath) {
    std::lock_guard<std::mutex> lock(usbMutex);
    
    // Find the device in our list or add it
    bool found = false;
    for (auto& device : usbDevices) {
        if (device.devicePath == devicePath) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        USBDeviceInfo device;
        device.devicePath = devicePath;
        device.friendlyName = GetFriendlyName(devicePath);
        device.driveLetter = GetDriveLetterFromDevicePath(devicePath);
        device.isRemovable = IsRemovableDevice(devicePath);
        
        usbDevices.push_back(device);
        Logger::LogUSBEvent(L"ARRIVAL", device);
    }
}

void USBMonitor::OnDeviceRemoval(const std::wstring& devicePath) {
    std::lock_guard<std::mutex> lock(usbMutex);
    
    auto it = std::find_if(usbDevices.begin(), usbDevices.end(),
        [&devicePath](const USBDeviceInfo& device) {
            return device.driveLetter == devicePath;
        });
    
    if (it != usbDevices.end()) {
        Logger::LogUSBEvent(L"REMOVAL", *it);
        usbDevices.erase(it);
    }
}

std::wstring USBMonitor::GetDriveLetterFromDevicePath(const std::wstring& devicePath) {
    // This is a simplified implementation
    // In a real implementation, you would parse the device path to get the drive letter
    return L"";
}

std::wstring USBMonitor::GetFriendlyName(const std::wstring& devicePath) {
    // This is a simplified implementation
    // In a real implementation, you would query the device for its friendly name
    return L"USB Device";
}

bool USBMonitor::IsRemovableDevice(const std::wstring& devicePath) {
    // This is a simplified implementation
    // In a real implementation, you would check the device properties
    return true;
}
