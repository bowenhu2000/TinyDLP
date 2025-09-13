#pragma once

#include "Common.h"

class SystemTray {
private:
    static NOTIFYICONDATAW nid;
    static bool isInitialized;
    static HMENU hContextMenu;
    static HWND hLogViewerWindow;
    static std::vector<std::wstring> logMessages;
    static std::mutex logMutex;
    
    static LRESULT CALLBACK LogViewerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void ShowAboutDialog();
    static void ShowLogViewer();
    static void UpdateLogViewer();
    static void ScrollToBottom();
    
public:
    static bool Initialize(HWND hMainWnd);
    static void Shutdown();
    static bool HandleTrayMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void ShowTrayIcon();
    static void HideTrayIcon();
    static void UpdateTrayIcon(const std::wstring& tooltip);
    static void AddLogMessage(const std::wstring& message);
};
