#include "AlertDialog.h"
#include "Logger.h"

void AlertDialog::ShowPDFBlockAlert(const std::wstring& fileName, const std::wstring& processName, const std::wstring& usbDrive) {
    std::wstring message = FormatAlertMessage(fileName, processName, usbDrive);
    
    // Log the alert
    Logger::Log(LOG_WARNING, L"PDF Block Alert: " + message);
    
    // Show message box
    MessageBoxW(NULL, message.c_str(), L"TinyDLP - PDF Save Blocked", MB_ICONWARNING | MB_OK | MB_TOPMOST);
}

void AlertDialog::ShowErrorAlert(const std::wstring& title, const std::wstring& message) {
    Logger::Log(LOG_ERROR, L"Error Alert: " + message);
    MessageBoxW(NULL, message.c_str(), title.c_str(), MB_ICONERROR | MB_OK | MB_TOPMOST);
}

void AlertDialog::ShowInfoAlert(const std::wstring& title, const std::wstring& message) {
    Logger::Log(LOG_INFO, L"Info Alert: " + message);
    MessageBoxW(NULL, message.c_str(), title.c_str(), MB_ICONINFORMATION | MB_OK | MB_TOPMOST);
}

std::wstring AlertDialog::FormatAlertMessage(const std::wstring& fileName, const std::wstring& processName, const std::wstring& usbDrive) {
    std::wstringstream ss;
    ss << L"PDF file save attempt blocked!\n\n";
    ss << L"File: " << fileName << L"\n";
    ss << L"Process: " << processName << L"\n";
    ss << L"Target Drive: " << usbDrive << L"\n\n";
    ss << L"TinyDLP has prevented this PDF file from being saved to a USB drive for security reasons.";
    
    return ss.str();
}
