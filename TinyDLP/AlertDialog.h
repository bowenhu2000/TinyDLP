#pragma once

#include "Common.h"

class AlertDialog {
public:
    static void ShowPDFBlockAlert(const std::wstring& fileName, const std::wstring& processName, const std::wstring& usbDrive);
    static void ShowErrorAlert(const std::wstring& title, const std::wstring& message);
    static void ShowInfoAlert(const std::wstring& title, const std::wstring& message);
    
private:
    static INT_PTR CALLBACK AlertDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static std::wstring FormatAlertMessage(const std::wstring& fileName, const std::wstring& processName, const std::wstring& usbDrive);
};
