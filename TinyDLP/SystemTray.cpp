#include "SystemTray.h"
#include "Logger.h"
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

// Static member definitions
NOTIFYICONDATAW SystemTray::nid = {};
bool SystemTray::isInitialized = false;
HMENU SystemTray::hContextMenu = NULL;
HWND SystemTray::hLogViewerWindow = NULL;
std::vector<std::wstring> SystemTray::logMessages;
std::mutex SystemTray::logMutex;

// Constants
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ABOUT 1001
#define ID_TRAY_LOGS 1002
#define ID_TRAY_EXIT 1003

bool SystemTray::Initialize(HWND hMainWnd) {
    if (isInitialized) {
        return true;
    }
    
    // Create context menu
    hContextMenu = CreatePopupMenu();
    if (!hContextMenu) {
        return false;
    }
    
    AppendMenuW(hContextMenu, MF_STRING, ID_TRAY_ABOUT, L"About");
    AppendMenuW(hContextMenu, MF_STRING, ID_TRAY_LOGS, L"Logs");
    AppendMenuW(hContextMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hContextMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    
    // Initialize system tray icon
    ZeroMemory(&nid, sizeof(NOTIFYICONDATAW));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"TinyDLP - Data Loss Prevention");
    
    isInitialized = true;
    return true;
}

void SystemTray::Shutdown() {
    if (hLogViewerWindow) {
        DestroyWindow(hLogViewerWindow);
        hLogViewerWindow = NULL;
    }
    
    if (hContextMenu) {
        DestroyMenu(hContextMenu);
        hContextMenu = NULL;
    }
    
    if (isInitialized) {
        Shell_NotifyIconW(NIM_DELETE, &nid);
        isInitialized = false;
    }
}

bool SystemTray::HandleTrayMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_TRAYICON) {
        switch (LOWORD(lParam)) {
            case WM_RBUTTONUP: {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hContextMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                PostMessage(hWnd, WM_NULL, 0, 0);
                return true;
            }
            case WM_LBUTTONDBLCLK:
                ShowLogViewer();
                return true;
        }
    } else if (uMsg == WM_COMMAND) {
        switch (LOWORD(wParam)) {
            case ID_TRAY_ABOUT:
                ShowAboutDialog();
                return true;
            case ID_TRAY_LOGS:
                ShowLogViewer();
                return true;
            case ID_TRAY_EXIT:
                PostQuitMessage(0);
                return true;
        }
    }
    return false;
}

void SystemTray::ShowTrayIcon() {
    if (isInitialized) {
        Shell_NotifyIconW(NIM_ADD, &nid);
    }
}

void SystemTray::HideTrayIcon() {
    if (isInitialized) {
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }
}

void SystemTray::UpdateTrayIcon(const std::wstring& tooltip) {
    if (isInitialized) {
        wcscpy_s(nid.szTip, tooltip.c_str());
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
}

void SystemTray::AddLogMessage(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    logMessages.push_back(message);
    
    // Keep only last 1000 messages to prevent memory issues
    if (logMessages.size() > 1000) {
        logMessages.erase(logMessages.begin());
    }
    
    // Update log viewer if it's open
    if (hLogViewerWindow && IsWindow(hLogViewerWindow)) {
        PostMessage(hLogViewerWindow, WM_USER + 1, 0, 0);
    }
}

void SystemTray::ShowAboutDialog() {
    MessageBoxW(NULL, 
        L"TinyDLP v1.0\n\n"
        L"Data Loss Prevention Tool\n"
        L"Monitors file operations and USB devices\n"
        L"to prevent unauthorized data exfiltration.\n\n"
        L" 2024 TinyDLP",
        L"About TinyDLP",
        MB_OK | MB_ICONINFORMATION);
}

void SystemTray::ShowLogViewer() {
    if (hLogViewerWindow && IsWindow(hLogViewerWindow)) {
        SetForegroundWindow(hLogViewerWindow);
        ShowWindow(hLogViewerWindow, SW_SHOW);
        return;
    }
    
    // Register log viewer window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = LogViewerWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TinyDLPLogViewer";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassExW(&wc);
    
    // Create log viewer window
    hLogViewerWindow = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"TinyDLPLogViewer",
        L"TinyDLP Log Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (hLogViewerWindow) {
        ShowWindow(hLogViewerWindow, SW_SHOW);
        UpdateWindow(hLogViewerWindow);
    }
}

LRESULT CALLBACK SystemTray::LogViewerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL;
    
    switch (uMsg) {
        case WM_CREATE: {
            hEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | 
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                0, 0, 0, 0,
                hWnd,
                (HMENU)1,
                GetModuleHandle(NULL),
                NULL
            );
            
            if (hEdit) {
                // Set font
                HFONT hFont = CreateFontW(
                    14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Consolas"
                );
                SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
            
            // Load existing log messages
            UpdateLogViewer();
            break;
        }
        
        case WM_SIZE: {
            if (hEdit) {
                RECT rect;
                GetClientRect(hWnd, &rect);
                SetWindowPos(hEdit, NULL, 0, 0, 
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;
        }
        
        case WM_USER + 1: // Update log messages
            UpdateLogViewer();
            break;
            
        case WM_CLOSE:
            ShowWindow(hWnd, SW_HIDE);
            return 0;
            
        case WM_DESTROY:
            hLogViewerWindow = NULL;
            break;
            
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    
    return 0;
}

void SystemTray::UpdateLogViewer() {
    if (!hLogViewerWindow || !IsWindow(hLogViewerWindow)) {
        return;
    }
    
    HWND hEdit = GetDlgItem(hLogViewerWindow, 1);
    if (!hEdit) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Clear current content
    SetWindowTextW(hEdit, L"");
    
    // Add all log messages
    std::wstring allLogs;
    for (const auto& message : logMessages) {
        allLogs += message + L"\r\n";
    }
    
    // Set text and scroll to bottom
    SetWindowTextW(hEdit, allLogs.c_str());
    ScrollToBottom();
}

void SystemTray::ScrollToBottom() {
    HWND hEdit = GetDlgItem(hLogViewerWindow, 1);
    if (hEdit) {
        int lineCount = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
        SendMessage(hEdit, EM_LINESCROLL, 0, lineCount);
    }
}
