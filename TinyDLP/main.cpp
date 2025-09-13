#include "Common.h"
#include "Logger.h"
#include "USBMonitor.h"
#include "FileMonitor.h"
#include "AlertDialog.h"
#include "DLLInjector.h"
#include "SystemTray.h"

// Global variables
HWND g_hWnd = NULL;
bool g_isRunning = false;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            g_hWnd = hWnd;
            return 0;
            
        case WM_DEVICECHANGE:
            return USBMonitor::HandleDeviceChange(wParam, lParam);
            
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            // Handle system tray messages
            if (SystemTray::HandleTrayMessage(hWnd, uMsg, wParam, lParam)) {
                return 0;
            }
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

// Create hidden window for device notifications
bool CreateHiddenWindow() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TinyDLPWindow";
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    g_hWnd = CreateWindowExW(
        0,
        L"TinyDLPWindow",
        L"TinyDLP",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    return g_hWnd != NULL;
}

// Check if running as administrator
bool IsRunningAsAdministrator() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
}

// Initialize all components
bool InitializeComponents() {
    // Initialize logger
    if (!Logger::Initialize()) {
        MessageBoxW(NULL, L"Failed to initialize logger", L"TinyDLP Error", MB_ICONERROR);
        return false;
    }
    
    // Create hidden window
    if (!CreateHiddenWindow()) {
        Logger::Log(LOG_ERROR, L"Failed to create hidden window");
        return false;
    }
    
    // Initialize system tray
    if (!SystemTray::Initialize(g_hWnd)) {
        Logger::Log(LOG_ERROR, L"Failed to initialize system tray");
        return false;
    }
    
    // Set up logger callback for system tray
    Logger::OnLogMessage = SystemTray::AddLogMessage;
    
    // Initialize USB monitor
    if (!USBMonitor::Initialize(g_hWnd)) {
        Logger::Log(LOG_ERROR, L"Failed to initialize USB monitor");
        return false;
    }
    
    // Initialize file monitor
    if (!FileMonitor::Initialize()) {
        Logger::Log(LOG_ERROR, L"Failed to initialize file monitor");
        return false;
    }
    
    // Initialize DLL injector
    if (!DLLInjector::Initialize()) {
        Logger::Log(LOG_ERROR, L"Failed to initialize DLL injector");
        return false;
    }
    
    return true;
}

// Shutdown all components
void ShutdownComponents() {
    DLLInjector::Shutdown();
    FileMonitor::Shutdown();
    USBMonitor::Shutdown();
    SystemTray::Shutdown();
    Logger::Shutdown();
    
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
}

// Main message loop
void RunMessageLoop() {
    MSG msg = {};
    g_isRunning = true;
    
    Logger::Log(LOG_INFO, L"TinyDLP started - monitoring processes and injecting DLL for PDF blocking");
    
    // Show system tray icon
    SystemTray::ShowTrayIcon();
    
    // Start process monitoring and DLL injection
    DLLInjector::StartProcessMonitoring();
    
    while (g_isRunning && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    Logger::Log(LOG_INFO, L"TinyDLP shutting down");
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Check if running as administrator
    if (!IsRunningAsAdministrator()) {
        MessageBoxW(NULL, 
            L"TinyDLP must be run as Administrator to monitor file operations.\n\n"
            L"Please right-click on TinyDLP.exe and select 'Run as administrator'.",
            L"Administrator Required", 
            MB_ICONWARNING | MB_OK);
        return 1;
    }
    
    // Initialize components
    if (!InitializeComponents()) {
        MessageBoxW(NULL, L"Failed to initialize TinyDLP components", L"TinyDLP Error", MB_ICONERROR);
        return 1;
    }
    
    // Run main message loop
    RunMessageLoop();
    
    // Shutdown components
    ShutdownComponents();
    
    return 0;
}
