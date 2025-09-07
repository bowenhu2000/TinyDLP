#pragma once

#include "Common.h"

class USBMonitor {
private:
    static std::vector<USBDeviceInfo> usbDevices;
    static std::mutex usbMutex;
    static bool isMonitoring;
    static HANDLE hDeviceNotify;
    static HWND hWnd;

public:
    static bool Initialize(HWND windowHandle);
    static void Shutdown();
    static void StartMonitoring();
    static void StopMonitoring();
    static std::vector<USBDeviceInfo> GetUSBDevices();
    static bool IsUSBDrive(const std::wstring& drivePath);
    static LRESULT HandleDeviceChange(WPARAM wParam, LPARAM lParam);
    
private:
    static void EnumerateUSBDevices();
    static void OnDeviceArrival(const std::wstring& devicePath);
    static void OnDeviceRemoval(const std::wstring& devicePath);
    static std::wstring GetDriveLetterFromDevicePath(const std::wstring& devicePath);
    static std::wstring GetFriendlyName(const std::wstring& devicePath);
    static bool IsRemovableDevice(const std::wstring& devicePath);
};
