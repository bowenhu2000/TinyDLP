#include "Common.h"
#include "Logger.h"
#include "USBMonitor.h"
#include "FileMonitor.h"
#include "AlertDialog.h"

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
    
    if (!g_hWnd) {
        return false;
    }
    
    // Hide the window
    ShowWindow(g_hWnd, SW_HIDE);
    
    return true;
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
    
    return true;
}

// Shutdown all components
void ShutdownComponents() {
    FileMonitor::Shutdown();
    USBMonitor::Shutdown();
    Logger::Shutdown();
    
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
}

// Main message loop
void RunMessageLoop() {
    MSG msg;
    g_isRunning = true;
    
    // Start monitoring
    USBMonitor::StartMonitoring();
    FileMonitor::StartMonitoring();
    
    Logger::Log(LOG_INFO, L"TinyDLP started successfully");
    AlertDialog::ShowInfoAlert(L"TinyDLP", L"TinyDLP has started and is monitoring for PDF file saves to USB drives.");
    
    while (g_isRunning) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_isRunning = false;
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Small delay to prevent high CPU usage
        Sleep(100);
    }
    
    // Stop monitoring
    FileMonitor::StopMonitoring();
    USBMonitor::StopMonitoring();
    
    Logger::Log(LOG_INFO, L"TinyDLP stopped");
}

int wmain(int argc, wchar_t* argv[]) {
    // Check if running as administrator
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        
        CloseHandle(hToken);
    }
    
    if (!isAdmin) {
        MessageBoxW(NULL, 
            L"TinyDLP requires administrator privileges to monitor file operations.\n"
            L"Please run as administrator.",
            L"TinyDLP - Administrator Required",
            MB_ICONWARNING | MB_OK);
        return 1;
    }
    
    // Initialize components
    if (!InitializeComponents()) {
        MessageBoxW(NULL, L"Failed to initialize TinyDLP components", L"TinyDLP Error", MB_ICONERROR);
        return 1;
    }
    
    // Run the application
    RunMessageLoop();
    
    // Shutdown components
    ShutdownComponents();
    
    return 0;
}
